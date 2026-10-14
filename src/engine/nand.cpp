#include "engine/nand.h"
#include <gccore.h>
#include <malloc.h>
#include <string.h>

namespace nand {

static bool s_init = false;
static s32 s_err = 0;
s32 lastError() { return s_err; }
static mutex_t s_lock = LWP_MUTEX_NULL;  // la leen el hilo principal y el de red

u8* readFile(const char* path, u32* size) {
    if (s_lock == LWP_MUTEX_NULL) LWP_MutexInit(&s_lock, false);
    LWP_MutexLock(s_lock);
    u8* result = nullptr;
    do {
        if (!s_init) {
            if ((s_err = ISFS_Initialize()) < 0) break;
            s_init = true;
        }
        static char name[64] ATTRIBUTE_ALIGN(32);
        strncpy(name, path, sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;
        s32 fd = ISFS_Open(name, ISFS_OPEN_READ);
        if (fd < 0) {
            s_err = fd;
            break;
        }
        static fstats st ATTRIBUTE_ALIGN(32);
        if (ISFS_GetFileStats(fd, &st) < 0 || st.file_length == 0) {
            ISFS_Close(fd);
            break;
        }
        u32 len = st.file_length;
        u8* buf = (u8*)memalign(32, (len + 31) & ~31u);
        if (!buf) {
            ISFS_Close(fd);
            break;
        }
        // A trozos: IOS lleva mejor lecturas pequeñas
        u32 got = 0;
        while (got < len) {
            u32 want = len - got > 0x10000 ? 0x10000 : len - got;
            s32 r = ISFS_Read(fd, buf + got, want);
            if (r <= 0) {
                s_err = r;
                break;
            }
            got += r;
        }
        ISFS_Close(fd);
        if (got != len) {
            free(buf);
            break;
        }
        DCFlushRange(buf, (len + 31) & ~31u);
        *size = len;
        result = buf;
    } while (false);
    LWP_MutexUnlock(s_lock);
    return result;
}

// Lista un directorio de la NAND (nombres separados por salto de línea). Solo para diagnóstico desde el PC.
u8* listDir(const char* path, u32* size) {
    if (s_lock == LWP_MUTEX_NULL) LWP_MutexInit(&s_lock, false);
    LWP_MutexLock(s_lock);
    u8* result = nullptr;
    do {
        if (!s_init) {
            if ((s_err = ISFS_Initialize()) < 0) break;
            s_init = true;
        }
        static char name[64] ATTRIBUTE_ALIGN(32);
        strncpy(name, path, sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;
        u32 nl = strlen(name);
        if (nl > 1 && name[nl - 1] == '/') name[nl - 1] = 0;
        u32 cnt = 128;
        char* names = (char*)memalign(32, (cnt * 13 + 31) & ~31u);
        if (!names) break;
        memset(names, 0, cnt * 13);
        if ((s_err = ISFS_ReadDir(name, names, &cnt)) < 0) {
            free(names);
            break;
        }
        u8* buf = (u8*)memalign(32, (cnt * 13 + 32) & ~31u);
        if (!buf) {
            free(names);
            break;
        }
        u32 len = 0;
        const char* p = names;
        for (u32 i = 0; i < cnt; i++) {
            u32 l = strlen(p);
            memcpy(buf + len, p, l);
            len += l;
            buf[len++] = 0x0A;
            p += l + 1;
        }
        free(names);
        *size = len;
        result = buf;
    } while (false);
    LWP_MutexUnlock(s_lock);
    return result;
}

}  // namespace nand
