#include "engine/save.h"
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

namespace save {

static const u32 MAGIC = 0x484B5331;  // "HKS1"
static const u16 VERSION = 1;

static SaveData s_data;
static char s_path[256] = "";
static bool s_ok = false;

static void setDefaults() {
    memset(&s_data, 0, sizeof(s_data));
    s_data.magic = MAGIC;
    s_data.version = VERSION;
    struct Def { const char* name; u8 color, skin, hair, txapela; };
    const Def defs[4] = {
        {"Iñaki", 0, 0, 0, 1},
        {"Ane", 5, 1, 1, 0},
        {"Mikel", 4, 0, 2, 1},
        {"Maite", 2, 2, 0, 0},
    };
    for (int i = 0; i < 4; i++) {
        Profile& p = s_data.profiles[i];
        strncpy(p.name, defs[i].name, NAME_LEN);
        p.used = 1;
        p.color = defs[i].color;
        p.skin = defs[i].skin;
        p.hair = defs[i].hair;
        p.txapela = defs[i].txapela;
    }
}

static bool dirExists(const char* d) {
    struct stat st;
    return stat(d, &st) == 0 && S_ISDIR(st.st_mode);
}

void init(const char* argv0) {
    setDefaults();
    if (!fatInitDefault()) return;

    // Carpeta desde la que se lanzó el juego (p.ej. "usb:/apps/herri-kirolak/boot.dol")
    char dir[256] = "";
    if (argv0 && strchr(argv0, ':')) {
        strncpy(dir, argv0, sizeof(dir) - 1);
        char* slash = strrchr(dir, '/');
        if (slash) slash[1] = '\0';
        else dir[0] = '\0';
    }
    if (!dir[0] || !dirExists(dir)) {
        const char* candidates[] = {"usb:/apps/herri-kirolak/", "sd:/apps/herri-kirolak/"};
        dir[0] = '\0';
        for (const char* c : candidates)
            if (dirExists(c)) { strcpy(dir, c); break; }
    }
    if (!dir[0]) return;
    snprintf(s_path, sizeof(s_path), "%ssave.dat", dir);
    s_ok = true;

    FILE* f = fopen(s_path, "rb");
    if (!f) return;
    SaveData tmp;
    if (fread(&tmp, sizeof(tmp), 1, f) == 1 && tmp.magic == MAGIC && tmp.version == VERSION) s_data = tmp;
    fclose(f);
}

SaveData& data() { return s_data; }
bool available() { return s_ok; }
const char* location() { return s_ok ? s_path : ""; }

bool write() {
    if (!s_ok) return false;
    FILE* f = fopen(s_path, "wb");
    if (!f) return false;
    bool ok = fwrite(&s_data, sizeof(s_data), 1, f) == 1;
    fclose(f);
    return ok;
}

}  // namespace save
