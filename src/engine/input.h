#pragma once
// Wiimotes: botones, acelerómetro y detector del gesto de hachazo.
//
// Gesto: subir el Wii Remote (la punta mirando hacia arriba, como un hacha en alto) → queda ARMADO;
// desde ahí, bajarlo con fuerza → HACHAZO. Sin subirlo antes no hay golpe.
// El lado de la V sale de dónde empieza el golpe (inclinación del mando arriba, p.ej. arriba a la
// derecha) y del empuje lateral durante la bajada.
#include <wiiuse/wpad.h>
#include "engine/math.h"

namespace input {

constexpr int MAX_PADS = 4;

struct Chop {
    bool happened = false;
    float strength = 0;  // 0..1
    float tilt = 0;      // -1 (ezkerra) .. +1 (eskuina); cerca de 0 = golpe recto
    float peakG = 0;     // para depurar / Kolpe proba
    float rollPart = 0, latPart = 0;
};

enum SwingState { SW_IDLE, SW_RAISING, SW_ARMED, SW_SWING, SW_COOLDOWN };

struct Pad {
    bool connected = false;
    u32 held = 0, down = 0;
    Vec3 accel;            // fuerza g de la última lectura
    Vec3 grav{0, 0, 1};    // gravedad filtrada (orientación cuando el mando va despacio)
    float pitchUp = 0;     // -1..1: 1 = la punta apunta hacia arriba
    float raiseDeg = 0;    // 0° = apunta a la tele, 90° = punta arriba, 180° = por encima del hombro
    float raise = 0;       // 0..1 cuánto está subida el hacha (para animar)
    float tiltLive = 0;    // inclinación lateral actual, -1..1 (con signo de ajustes)
    Chop chop;             // evento de este frame
    SwingState state = SW_IDLE;

    // internos del detector
    float timer = 0, peak = 0, lat = 0, rollTop = 0, gravXTop = 0;
    float rumbleT = 0;
};

// Una lectura cruda (para grabar golpes en Kolpe proba)
struct Sample {
    u32 ms;
    float ax, ay, az, gx, gy, gz;
    u8 state;
    u8 chop;
    u16 btns;  // botones mantenidos (marcas de inicio/fin de gesto en Kolpe proba)
};
typedef void (*SampleSink)(int chan, const Sample& s);

void init();
void update(float dt);
const Pad& pad(int chan);
int anyDown(u32 mask);
u32 allDown();
void rumble(int chan, float seconds);
void stopRumble();
int connectedCount();
// Ajustes del jugador (se guardan en save.dat)
void setTiltInverted(bool inv);
void setSensitivity(int level);  // 0 = suave (golpes flojos cuentan), 1 = normal, 2 = hay que dar fuerte
void setRaiseInverted(bool inv);  // "arriba" = -Y en vez de +Y
void setSampleSink(SampleSink sink);

}  // namespace input
