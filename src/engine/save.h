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
    u8 pad[3];
};

enum RecordSlot { REC_TRAINING, REC_COMP_1, REC_COMP_2, REC_COMP_3, REC_COUNT };

struct Record {
    u32 timeMs;  // 0 = sin récord
    char name[NAME_LEN + 1];
};

struct SaveData {
    u32 magic;
    u16 version;
    u16 pad;
    Profile profiles[MAX_PROFILES];
    Record records[REC_COUNT];
};

namespace save {
void init(const char* argv0);
SaveData& data();
bool write();
bool available();
const char* location();  // para mostrarlo en pantalla
}  // namespace save
