#include "engine/telemetry.h"
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
static char s_ip[16] = "";
static lwp_t s_thread;
static u8 s_stack[32 * 1024] ATTRIBUTE_ALIGN(32);

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
    u32 ip = net_gethostip();
    snprintf(s_ip, sizeof(s_ip), "%u.%u.%u.%u", (ip >> 24) & 255, (ip >> 16) & 255, (ip >> 8) & 255, ip & 255);

    s32 server = net_socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server < 0) {
        snprintf(s_ip, sizeof(s_ip), "socket %d", (int)server);
        return nullptr;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (net_bind(server, (struct sockaddr*)&addr, sizeof(addr)) < 0 || net_listen(server, 1) < 0) {
        snprintf(s_ip, sizeof(s_ip), "bind err");
        net_close(server);
        return nullptr;
    }
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
        s_tail = s_head;  // empezar con el búfer vacío
        s_status = 2;
        const char* hello = "# herri-kirolak telemetria: mando,ms,ax,ay,az,gx,gy,gz,egoera,kolpea,botoiak\n";
        bool ok = sendAll(sock, hello, strlen(hello));
        while (ok) {
            int n = 0;
            while (s_tail != s_head && n < (int)sizeof(buf) - 128) {
                const Entry& e = s_ring[s_tail];
                n += snprintf(buf + n, sizeof(buf) - n, "%d,%u,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%04x\n", (int)e.chan,
                              e.s.ms, e.s.ax, e.s.ay, e.s.az, e.s.gx, e.s.gy, e.s.gz, e.s.state, e.s.chop, e.s.btns);
                s_tail = (s_tail + 1) % RING;
            }
            if (n > 0) ok = sendAll(sock, buf, n);
            else usleep(15000);
        }
        net_close(sock);
        s_status = 1;
    }
    return nullptr;
}

void init() { LWP_CreateThread(&s_thread, run, nullptr, s_stack, sizeof(s_stack), 40); }

}  // namespace telemetry
