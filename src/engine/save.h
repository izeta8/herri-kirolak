#pragma once
// Datos guardados: personajes y récords. Se guardan en save.dat junto a boot.dol (USB o SD).
#include <gctypes.h>

constexpr int MAX_PROFILES = 8;
constexpr int NAME_LEN = 11;  // + '\0'

struct Profile {
    char name[NAME_LEN + 1];
    u8 used;
    u8 color;    // índice en la paleta de colores favoritos
    u8 skin;     // tono de piel
    u8 hair;     // color de pelo
    u8 txapela;  // 1 = con txapela
    u8 hairStyle;  // peinado (0 en partidas guardadas antes de existir)
    u8 pad[2];
};

enum RecordSlot { REC_TRAINING, REC_COMP_1, REC_COMP_2, REC_COMP_3, REC_COUNT };

struct Record {
    u32 timeMs;  // 0 = sin récord
    char name[NAME_LEN + 1];
};

struct Settings {
    u8 tiltInverted;  // 1 = cambiar izquierda/derecha
    u8 sensitivity;   // 0 suave, 1 normal, 2 fuerte
    u8 raiseInverted; // 1 = "arriba" es -Y en vez de +Y
    u8 pad[13];
};

struct SaveData {
    u32 magic;
    u16 version;
    u16 pad;
    Profile profiles[MAX_PROFILES];
    Record records[REC_COUNT];
    Settings settings;  // desde la versión 2
};

namespace save {
void init(const char* argv0);
SaveData& data();
bool write();
bool available();
const char* location();  // para mostrarlo en pantalla
const char* directory(); // carpeta del juego en USB/SD ("" si no hay)
void applySettings();    // pasa los ajustes al detector de golpes
}  // namespace save
