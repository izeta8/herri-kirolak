#pragma once
// Personaje calcado a los Miis de la Wii: cabeza de huevo con la cara de línea, pelo de casco, cuerpo estrecho y brazos.
#include "engine/mesh.h"
#include "engine/save.h"

// Peinados: 0 corto con raya, 1 de punta, 2 melena larga, 3 media melena
constexpr int NUM_HAIR_STYLES = 4;

struct Appearance {
    u8 color = 0, skin = 0, hair = 0, txapela = 0;
    u8 casual = 0;  // 1 = público: ropa de calle (pantalón gris de Mii, sin gerriko) y menos polígonos
    u8 style = 0;   // peinado
    s8 user = -1;   // Mii de la consola (índice en el Canal Mii); -1 = ninguno
    static Appearance from(const Profile& p) {
        return {p.color, p.skin, p.hair, p.txapela, 0, p.hairStyle, (s8)(p.mii ? p.mii - 1 : -1)};
    }
    bool operator==(const Appearance& o) const {
        return color == o.color && skin == o.skin && hair == o.hair && txapela == o.txapela && casual == o.casual &&
               style == o.style && user == o.user;
    }
};

class MiiHead;

struct CharacterModel {
    Appearance look;
    MiiHead* mii = nullptr;  // cabeza de Mii de verdad (si la consola tiene las piezas); si no, la nuestra
    Mesh beret;              // txapela para ponerla encima de la cabeza Mii
    Mesh head, headBlink, torso, legUnit, pants, hand, sleeve, forearm, shoulder;
    bool built = false;
    void build(const Appearance& a);
    void release();
};

// Postura en coordenadas de mundo
struct CharacterPose {
    Vec3 pos;              // pies
    float yaw = 0;         // 0 = mira hacia +Z
    float lean = 0;        // inclinación del torso hacia delante (rad)
    float side = 0;        // inclinación lateral (rad)
    float headPitch = 0;   // cabeza mirando abajo (+) / arriba (-)
    float squash = 0;      // aplastamiento al saltar/aterrizar
    bool blink = false;
    Vec3 handL, handR;     // posiciones absolutas de las manos
    bool handsVisible = true;
};

// Altura de los hombros sobre los pies (para colocar manos)
constexpr float CHAR_SHOULDER_Y = 0.86f;

void drawCharacter(const CharacterModel& m, const CharacterPose& p);
// Transformación del torso (para calcular dónde caen hombros, etc.)
Xf characterTorsoXf(const CharacterPose& p);
// Una caché sencilla de modelos para no reconstruirlos
CharacterModel* characterCache(const Appearance& a);
