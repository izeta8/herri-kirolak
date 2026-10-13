#include "engine/updater.h"
#include <asndlib.h>
#include <fat.h>
#include <gccore.h>
#include <malloc.h>
#include <network.h>
#include <ogc/machine/processor.h>
#include <string.h>
#include <unistd.h>
#include <wiiuse/wpad.h>
#include "booter_bin.h"
#include "engine/telemetry.h"

namespace updater {

static u8* const DOL_ADDR = (u8*)0x92000000;     // donde el cargador espera el DOL
static u8* const BOOTER_ADDR = (u8*)0x93000000;  // donde vive el cargador (tools/booter)
static const u32 MAX_DOL = 12 * 1024 * 1024;

static volatile bool s_ready = false;
static u8* s_buf = nullptr;
static u32 s_size = 0;
static lwp_t s_thread;
static volatile s32 s_server = -1;
static u8 s_stack[16 * 1024] ATTRIBUTE_ALIGN(32);

static bool recvAll(s32 sock, u8* dst, u32 len) {
    while (len > 0) {
        s32 n = net_recv(sock, dst, len > 32768 ? 32768 : len, 0);
        if (n <= 0) return false;
        dst += n;
        len -= n;
    }
    return true;
}

// Comprobación básica de que lo recibido parece un DOL
static bool looksLikeDol(const u8* d, u32 size) {
    if (size < 0x100) return false;
    const u32* h = (const u32*)d;
    u32 entry = h[56];
    return entry >= 0x80000000 && entry < 0x81800000;
}

static void* run(void*) {
    s32 server = telemetry::netListen(PORT);
    if (server < 0) return nullptr;
    s_server = server;
    while (!s_ready) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        s32 sock = net_accept(server, (struct sockaddr*)&client, &len);
        if (sock < 0) {
            usleep(200000);
            continue;
        }
        u8 header[8];
        if (recvAll(sock, header, 8) && memcmp(header, "HKUP", 4) == 0) {
            u32 size = (header[4] << 24) | (header[5] << 16) | (header[6] << 8) | header[7];
            u8* buf = size <= MAX_DOL ? (u8*)memalign(32, size) : nullptr;
            if (buf && recvAll(sock, buf, size) && looksLikeDol(buf, size)) {
                net_send(sock, "OK\n", 3, 0);
                s_buf = buf;
                s_size = size;
                s_ready = true;
            } else {
                net_send(sock, "ERR\n", 4, 0);
                free(buf);
            }
        }
        net_close(sock);
    }
    return nullptr;
}

void startServer() { LWP_CreateThread(&s_thread, run, nullptr, s_stack, sizeof(s_stack), 40); }
bool ready() { return s_ready; }

void launch() {
    ASND_End();
    for (int i = 0; i < 4; i++) WPAD_Rumble(i, 0);
    fatUnmount("sd:/");
    fatUnmount("usb:/");
    telemetry::closeSockets();
    if (s_server >= 0) net_close(s_server);
    usleep(100000);
    net_deinit();

    memmove(DOL_ADDR, s_buf, s_size);
    DCFlushRange(DOL_ADDR, s_size);
    memcpy(BOOTER_ADDR, booter_bin, booter_bin_size);
    DCFlushRange(BOOTER_ADDR, booter_bin_size);
    ICInvalidateRange(BOOTER_ADDR, booter_bin_size);

    VIDEO_SetBlack(true);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
    u32 level;
    _CPU_ISR_Disable(level);
    (void)level;
    ((void (*)(void))BOOTER_ADDR)();
}

}  // namespace updater
