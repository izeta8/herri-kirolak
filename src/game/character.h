#pragma once
// Personaje "tipo Mii": cabeza grande, cuerpo cápsula, manos flotantes sin brazos.
#include "engine/mesh.h"
#include "engine/save.h"

struct Appearance {
    u8 color = 0, skin = 0, hair = 0, txapela = 0;
    static Appearance from(const Profile& p) { return {p.color, p.skin, p.hair, p.txapela}; }
    bool operator==(const Appearance& o) const {
        return color == o.color && skin == o.skin && hair == o.hair && txapela == o.txapela;
    }
};

struct CharacterModel {
    Appearance look;
    Mesh head, headBlink, torso, legs, hand;
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
