#include "engine/updater.h"
#include <asndlib.h>
#include <errno.h>
#include <fat.h>
#include <gccore.h>
#include <malloc.h>
#include <network.h>
#include <ogc/machine/processor.h>
#include <stdio.h>
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
static s32 s_server = -1, s_client = -1;
static u8 s_header[8];
static u8* s_buf = nullptr;
static u32 s_size = 0, s_got = 0;
static bool s_inBody = false;

// Comprobación básica de que lo recibido parece un DOL
static bool looksLikeDol(const u8* d, u32 size) {
    if (size < 0x100) return false;
    const u32* h = (const u32*)d;
    u32 entry = h[56];
    return entry >= 0x80000000 && entry < 0x81800000;
}

void open() {
    s_server = telemetry::netListen(PORT);
    char msg[48];
    snprintf(msg, sizeof(msg), "eguneratzea: portua %d -> %d", PORT, (int)s_server);
    telemetry::note(msg);
}

static void dropClient() {
    if (s_client >= 0) net_close(s_client);
    s_client = -1;
    if (!s_ready) {
        free(s_buf);
        s_buf = nullptr;
    }
}

void poll() {
    if (s_server < 0 || s_ready) return;
    if (s_client < 0) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        s32 c = net_accept(s_server, (struct sockaddr*)&client, &len);  // no bloqueante
        if (c < 0) return;  // nadie quiere conectar
        telemetry::setBlocking(c);
        char msg[48];
        snprintf(msg, sizeof(msg), "eguneratzea: konexioa %d", (int)c);
        telemetry::note(msg);
        s_client = c;
        s_got = 0;
        s_inBody = false;
    }
    // Lee lo que haya llegado, sin quedarse esperando
    for (int chunk = 0; chunk < 64; chunk++) {
        if (!telemetry::readable(s_client)) return;
        u8* dst;
        u32 want;
        if (!s_inBody) {
            dst = s_header + s_got;
            want = 8 - s_got;
        } else {
            dst = s_buf + s_got;
            want = s_size - s_got;
            if (want > 32768) want = 32768;
        }
        s32 n = net_recv(s_client, dst, want, 0);
        if (n <= 0) {
            dropClient();
            return;
        }
        s_got += n;
        if (!s_inBody && s_got == 8) {
            s_size = (s_header[4] << 24) | (s_header[5] << 16) | (s_header[6] << 8) | s_header[7];
            if (memcmp(s_header, "HKUP", 4) != 0 || s_size > MAX_DOL || !(s_buf = (u8*)memalign(32, s_size))) {
                dropClient();
                return;
            }
            s_inBody = true;
            s_got = 0;
        } else if (s_inBody && s_got == s_size) {
            telemetry::note("eguneratzea: DOL osoa jasota");
            if (looksLikeDol(s_buf, s_size)) {
                net_send(s_client, "OK\n", 3, 0);
                s_ready = true;
            } else {
                net_send(s_client, "ERR\n", 4, 0);
            }
            usleep(50000);
            dropClient();
            return;
        }
    }
}

bool ready() { return s_ready; }

void launch() {
    ASND_End();
    for (int i = 0; i < 4; i++) WPAD_Rumble(i, 0);
    fatUnmount("sd:/");
    fatUnmount("usb:/");
    telemetry::stop();
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
