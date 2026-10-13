#include "engine/telemetry.h"
#include "engine/updater.h"
#include <gccore.h>
#include <network.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
            if (net_bind(s, (struct sockaddr*)&addr, sizeof(addr)) >= 0 && net_listen(s, 1) >= 0) return s;
            net_close(s);
        }
        usleep(250000);
    }
    return -1;
}

void closeSockets() {
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

static bool sendAll(s32 sock, const char* buf, int len) {
    while (len > 0) {
        s32 n = net_send(sock, buf, len, 0);
        if (n <= 0) return false;
        buf += n;
        len -= n;
    }
    return true;
}

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
    updater::startServer();
    u32 ip = net_gethostip();
    snprintf(s_ip, sizeof(s_ip), "%u.%u.%u.%u", (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);

    s32 server = netListen(PORT);
    if (server < 0) {
        snprintf(s_ip + strlen(s_ip), sizeof(s_ip) - strlen(s_ip), " (bind?)");
        return nullptr;
    }
    s_server = server;
    s_status = 1;

    static char buf[8192];
    while (true) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        s32 sock = net_accept(server, (struct sockaddr*)&client, &len);
        if (sock < 0) {
            usleep(200000);
            continue;
        }
        s_client = sock;
        s_tail = s_head;  // empezar con el búfer vacío
        s_status = 2;
        const char* hello = "# herri-kirolak telemetria: mando,ms,ax,ay,az,gx,gy,gz,egoera,kolpea,botoiak,etiketa\n";
        bool ok = sendAll(sock, hello, strlen(hello));
        while (ok) {
            int n = 0;
            while (s_tail != s_head && n < (int)sizeof(buf) - 128) {
                const Entry& e = s_ring[s_tail];
                n += snprintf(buf + n, sizeof(buf) - n, "%d,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%04x,%c\n", (int)e.chan,
                              e.s.ms, e.s.ax, e.s.ay, e.s.az, e.s.gx, e.s.gy, e.s.gz, e.s.state, e.s.chop, e.s.btns,
                              e.s.label ? e.s.label : '-');
                s_tail = (s_tail + 1) % RING;
            }
            if (n > 0) ok = sendAll(sock, buf, n);
            else usleep(15000);
        }
        net_close(sock);
        s_client = -1;
        s_status = 1;
    }
    return nullptr;
}

void init() { LWP_CreateThread(&s_thread, run, nullptr, s_stack, sizeof(s_stack), 40); }

}  // namespace telemetry
