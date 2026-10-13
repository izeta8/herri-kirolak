#include "engine/telemetry.h"
#include "engine/updater.h"
#include <errno.h>
#include <fcntl.h>
#include <gccore.h>
#include <network.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace telemetry {

struct Entry {
    s32 chan;
    input::Sample s;
};

static const int RING = 4096;
static Entry s_ring[RING];
static volatile int s_head = 0, s_tail = 0;  // un productor (juego) y un consumidor (hilo de red)
static volatile int s_status = 0;
static char s_ip[24] = "";
static lwp_t s_thread;
static volatile s32 s_server = -1, s_client = -1;
static u8 s_stack[32 * 1024] ATTRIBUTE_ALIGN(32);

// Abre un socket de escucha en `port`. Reintenta: tras una actualización en caliente el puerto
// de la versión anterior puede tardar en liberarse.
s32 netListen(u16 port) {
    for (int tries = 0; tries < 40; tries++) {
        s32 s = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (s >= 0) {
            int one = 1;
            net_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = INADDR_ANY;
            if (net_bind(s, (struct sockaddr*)&addr, sizeof(addr)) >= 0 && net_listen(s, 4) >= 0) return s;
            net_close(s);
        }
        usleep(250000);
    }
    return -1;
}

// En la Wii real el modo no bloqueante de IOS da problemas (reenvía datos viejos, el puerto deja de
// aceptar conexiones): todo es bloqueante y antes de leer/aceptar se pregunta con net_poll
// Bits de poll de IOS: POLLRDNORM 0x01, POLLRDBAND 0x02, POLLERR 0x20, POLLHUP 0x40
static const u32 IOS_POLLIN = 0x0003, IOS_POLLERR_HUP = 0x0060;
bool readable(s32 s) {
    struct pollsd p = {s, IOS_POLLIN, 0};
    return net_poll(&p, 1, 0) > 0 && (p.revents & (IOS_POLLIN | IOS_POLLERR_HUP));
}

static char s_notes[8][80];
static volatile int s_noteHead = 0, s_noteTail = 0;
void note(const char* text) {
    int next = (s_noteHead + 1) % 8;
    if (next == s_noteTail) return;
    snprintf(s_notes[s_noteHead], sizeof(s_notes[0]), "# %s\n", text);
    s_noteHead = next;
}

void stop() {
    if (s_client >= 0) net_close(s_client);
    if (s_server >= 0) net_close(s_server);
    s_client = s_server = -1;
}

void push(int chan, const input::Sample& s) {
    if (s_status != 2) return;
    int next = (s_head + 1) % RING;
    if (next == s_tail) return;  // lleno: se pierde la lectura
    s_ring[s_head].chan = chan;
    s_ring[s_head].s = s;
    s_head = next;
}

int status() { return s_status; }
const char* ip() { return s_ip; }

// Envía todo (el socket del PC es bloqueante; el PC lee continuamente)
static bool sendAll(s32 sock, const char* buf, int len) {
    while (len > 0) {
        s32 n = net_send(sock, buf, len, 0);
        if (n == -EAGAIN) {
            usleep(2000);
            continue;
        }
        if (n <= 0) return false;
        buf += n;
        len -= n;
    }
    return true;
}

// Único hilo de red: telemetría (puerto 4300) y actualizaciones (4301), sin llamadas que bloqueen
static void* run(void*) {
    // net_init suele devolver -EAGAIN varias veces mientras el módulo WiFi arranca
    s32 r = -1;
    for (int tries = 0; tries < 400 && r < 0; tries++) {
        r = net_init();
        if (r < 0) usleep(r == -EAGAIN ? 50000 : 500000);
    }
    if (r < 0) {
        snprintf(s_ip, sizeof(s_ip), "err %d", (int)r);
        return nullptr;
    }
    u32 ip = net_gethostip();
    snprintf(s_ip, sizeof(s_ip), "%u.%u.%u.%u", (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);

    updater::open();
    s32 server = netListen(PORT);
    if (server < 0) snprintf(s_ip + strlen(s_ip), sizeof(s_ip) - strlen(s_ip), " (bind?)");
    s_server = server;
    s_status = 1;

    static char buf[8192];
    while (true) {
        updater::poll();
        if (updater::ready()) {
            usleep(100000);
            continue;
        }
        if (s_server >= 0 && s_client < 0 && readable(s_server)) {
            struct sockaddr_in client;
            socklen_t len = sizeof(client);
            s32 sock = net_accept(s_server, (struct sockaddr*)&client, &len);
            if (sock >= 0) {
                s_client = sock;
                s_tail = s_head;  // empezar con el búfer vacío
                s_status = 2;
                const char* hello = "# herri-kirolak telemetria: mando,ms,ax,ay,az,gx,gy,gz,egoera,kolpea,botoiak,etiketa\n";
                if (!sendAll(sock, hello, strlen(hello))) {
                    net_close(sock);
                    s_client = -1;
                    s_status = 1;
                }
            }
        }
        if (s_client >= 0) {
            while (s_noteTail != s_noteHead) {
                if (!sendAll(s_client, s_notes[s_noteTail], strlen(s_notes[s_noteTail]))) break;
                s_noteTail = (s_noteTail + 1) % 8;
            }
            int n = 0;
            while (s_tail != s_head && n < (int)sizeof(buf) - 128) {
                const Entry& e = s_ring[s_tail];
                n += snprintf(buf + n, sizeof(buf) - n, "%d,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%04x,%c\n", (int)e.chan,
                              e.s.ms, e.s.ax, e.s.ay, e.s.az, e.s.gx, e.s.gy, e.s.gz, e.s.state, e.s.chop, e.s.btns,
                              e.s.label ? e.s.label : '-');
                s_tail = (s_tail + 1) % RING;
            }
            if (n > 0 && !sendAll(s_client, buf, n)) {
                net_close(s_client);
                s_client = -1;
                s_status = 1;
            }
        }
        usleep(10000);
    }
    return nullptr;
}

void init() { LWP_CreateThread(&s_thread, run, nullptr, s_stack, sizeof(s_stack), 40); }

}  // namespace telemetry
