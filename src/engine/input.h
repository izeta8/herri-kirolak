#pragma once
// Wiimotes: botones, acelerómetro y detector del gesto de hachazo.
#include <wiiuse/wpad.h>
#include "engine/math.h"

namespace input {

constexpr int MAX_PADS = 4;

struct Chop {
    bool happened = false;
    float strength = 0;  // 0..1
    float tilt = 0;      // -1 (ezkerra) .. +1 (eskuina); cerca de 0 = golpe recto
};

struct Pad {
    bool connected = false;
    u32 held = 0, down = 0;
    Vec3 accel;        // fuerza g instantánea
    Vec3 grav{0, 0, 1};  // gravedad filtrada (orientación en reposo)
    float tiltLive = 0;  // inclinación de muñeca actual, -1..1
    float motion = 0;    // |a| - 1, para animaciones
    Chop chop;           // evento de este frame

    // estado interno del detector
    int state = 0;
    float timer = 0, peak = 0, sumX = 0, tiltStart = 0;
    int samples = 0;
    float rumbleT = 0;
};

void init();
void update(float dt);
const Pad& pad(int chan);
// ¿Algún mando ha pulsado alguno de estos botones este frame? (devuelve el canal o -1)
int anyDown(u32 mask);
// Botones pulsados por cualquier mando (para menús que maneja cualquiera)
u32 allDown();
void rumble(int chan, float seconds);
void stopRumble();
int connectedCount();

}  // namespace input
