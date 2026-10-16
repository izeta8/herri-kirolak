#include "engine/save.h"
#include <fat.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "engine/input.h"

namespace save {

static const u32 MAGIC = 0x484B5331;  // "HKS1"
static const u16 VERSION = 4;

static SaveData s_data;
static char s_path[256] = "";
static char s_dir[256] = "";
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
    s_data.settings.sensitivity = 1;
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
    strcpy(s_dir, dir);
    snprintf(s_path, sizeof(s_path), "%ssave.dat", dir);
    s_ok = true;

    FILE* f = fopen(s_path, "rb");
    if (!f) return;
    SaveData tmp;
    memset(&tmp, 0, sizeof(tmp));
    size_t got = fread(&tmp, 1, sizeof(tmp), f);
    fclose(f);
    const size_t v3Size = sizeof(SaveData) - sizeof(PalaRecords);  // v2 y v3: sin récords de la pala
    const size_t v1Size = v3Size - sizeof(Settings);
    if (tmp.magic != MAGIC) return;
    if (tmp.version == VERSION && got == sizeof(tmp)) s_data = tmp;
    else if ((tmp.version == 3 || tmp.version == 2) && got >= v3Size) {
        memcpy(&s_data, &tmp, v3Size);  // los récords de la pala, a cero
        if (tmp.version == 2) s_data.settings.raiseInverted = 0;  // en la v2 "arriba" estaba invertido por defecto
        s_data.version = VERSION;
    }
    else if (tmp.version == 1 && got >= v1Size) {
        memcpy(&s_data, &tmp, v1Size);  // la versión 1 no tenía ajustes
        s_data.version = VERSION;
        s_data.settings.sensitivity = 1;
    }
}

SaveData& data() { return s_data; }
bool available() { return s_ok; }
const char* location() { return s_ok ? s_path : ""; }
const char* directory() { return s_ok ? s_dir : ""; }

void applySettings() {
    input::setTiltInverted(s_data.settings.tiltInverted != 0);
    input::setSensitivity(s_data.settings.sensitivity);
    input::setRaiseInverted(s_data.settings.raiseInverted != 0);
    input::setRumbleEnabled(s_data.settings.rumbleOff == 0);
}

bool write() {
    if (!s_ok) return false;
    FILE* f = fopen(s_path, "wb");
    if (!f) return false;
    bool ok = fwrite(&s_data, sizeof(s_data), 1, f) == 1;
    fclose(f);
    return ok;
}

}  // namespace save
