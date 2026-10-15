#pragma once
// Cuerpos de Mii de Wii Sports Resort, leídos de hk_body.bin: lo genera tools/mii_body.py a partir del disco del usuario y
// NO va en el repositorio (ver docs/WII-NOTAS.md). Los de los bolos: A6 (hombre) y B6 (mujer), sin brazos y con manos
// sueltas, y C1 (público, torso de bala). Se dibujan con su material de Resort: la ropa sale de su textura (camiseta del
// color favorito, piel) y la luz de las lightmaps de game/mii/light. Cada hueso es una malla rígida.
#include <gccore.h>
#include "engine/gfx.h"

namespace miibody {

enum Kind { PLAYER_M = 0, PLAYER_F = 1, CROWD = 2, HAND = 3, NUM_KINDS };

bool init();             // carga hk_body.bin (false si no está: se usan los cuerpos propios)
bool available(int kind);
inline int playerKind(bool female) { return female ? PLAYER_F : PLAYER_M; }

// Un cuerpo concreto: la geometría es común a todos los Miis (Resort no cambia las proporciones con la altura ni la
// complexión); aquí van sus colores
struct Body {
    int kind = -1;
    GXColor shirt = {255, 255, 255, 255}, skin = {255, 255, 255, 255};
    bool lightShirt = false;  // camiseta clara: ropa con los dibujos oscuros
    bool valid() const { return kind >= 0; }
    // favColor: color favorito del Mii (0..11) si la camiseta es ese color; -1 si es otro (se mira lo clara que es)
    void build(int kind, Color shirt, Color skin, int favColor);
    void release() { kind = -1; }
};

// Postura en coordenadas de mundo (la misma que usa CharacterPose)
struct Pose {
    Xf root;               // pies, con la orientación del personaje
    float lean = 0, side = 0, headPitch = 0, squash = 0;
    Vec3 handL, handR;     // centros de las manos en el mundo (izquierda de la pantalla = -X del personaje)
    bool handsVisible = true;
};

// Dibuja el cuerpo y devuelve dónde va la cabeza (origen en la barbilla, sin escala) en `head` y la cintura (para el
// gerriko; en metros, sin escala) en `waist`
void draw(const Body& b, const Pose& p, Xf* head, Xf* waist = nullptr);

// Ancho de la cabeza de Mii (m) para este cuerpo, para que guarde las proporciones de Resort
float headWidth();

}  // namespace miibody
