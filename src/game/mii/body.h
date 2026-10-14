#pragma once
// Cuerpos de Mii de Wii Sports Resort (con brazos, dedos y esqueleto), leídos de hk_body.bin: lo genera
// tools/mii_body.py a partir del disco del usuario y NO va en el repositorio (ver docs/WII-NOTAS.md).
// Cada hueso se dibuja como una malla rígida; la pose se calcula aquí (inclinación, cabeza e IK de los brazos).
#include <vector>
#include "engine/mesh.h"

namespace miibody {

enum Kind { PLAYER = 0, CROWD = 1, NUM_KINDS };
enum Zone { SKIN = 0, SHIRT = 1, PANTS = 2, SHOE = 3 };

bool init();             // carga hk_body.bin (false si no está: se usan los cuerpos propios)
bool available(int kind);

// Cuerpo de un Mii ya coloreado (una malla por hueso)
struct Body {
    int kind = PLAYER;
    std::vector<Mesh> parts;  // índice = hueso
    float sy = 1, sxz = 1;    // proporciones según la altura y la complexión del Mii
    bool valid() const { return !parts.empty(); }
    void build(int kind, const Color zoneColors[4], u8 height, u8 build);
    void release();
};

// Postura en coordenadas de mundo (la misma que usa CharacterPose)
struct Pose {
    Xf root;               // pies, con la orientación del personaje
    float lean = 0, side = 0, headPitch = 0, squash = 0;
    Vec3 handL, handR;     // mano izquierda de la pantalla (-X del personaje) y derecha, en el mundo
    bool handsVisible = true;
};

// Dibuja el cuerpo y devuelve dónde va la cabeza (origen en la barbilla, sin escala) en `head` y la cintura (para el
// gerriko; en metros, sin escala) en `waist`
void draw(const Body& b, const Pose& p, Xf* head, Xf* waist = nullptr);

// Ancho de la cabeza de Mii (m) para este cuerpo, para que guarde las proporciones de Resort
float headWidth();

}  // namespace miibody
