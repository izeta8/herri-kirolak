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

// Latigazo (palas): un golpe rápido con el Wii Remote, sin subirlo antes. Se resuelve en el pico, que es
// el momento del golpe; la fuerza sale del pico de aceleración.
struct Whip {
    bool happened = false;
    float strength = 0;  // 0..1
    float peakG = 0;
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
    bool swingStart = false;  // evento: empieza la bajada (para animar sin esperar al final del golpe)
    bool swingAbort = false;  // evento: la bajada no llegó a ser hachazo
    float rawDeg = 0;         // ángulo de subida con la lectura directa (rápido, para animar)
    float swingTilt = 0;      // durante la bajada: lado estimado hasta ahora (-1 ezkerra .. +1 eskuina)
    SwingState state = SW_IDLE;
    // Latigazo (palas)
    Whip whip;               // evento de este frame
    bool whipStart = false;  // evento: empieza un latigazo (para animar)
    // Puntero IR (coordenadas 2D virtuales de la pantalla, ver setPointerArea) y pilas
    bool irValid = false;
    float irX = 0, irY = 0;
    u8 battery = 0;  // pilas en % (0..100): libogc ya lo da así (comprobado en Dolphin: 100 con pilas llenas)
    // Nunchuk
    bool nunchuk = false;
    float stickX = 0, stickY = 0;  // -1..1 (derecha +X, arriba +Y), con zona muerta

    // internos del detector
    float timer = 0, peak = 0, lat = 0, rollTop = 0, gravXTop = 0;
    float swingX[64];  // aceleración X de la bajada (para decidir el lado)
    int swingN = 0;
    float rumbleT = 0;
    int whipSt = 0;
    float whipT = 0, whipPeak = 0;
};

// Una lectura cruda (para grabar golpes en Kolpe proba)
struct Sample {
    u32 ms;
    float ax, ay, az, gx, gy, gz;
    u8 state;
    u8 chop;
    u16 btns;  // botones mantenidos (marcas de inicio/fin de gesto en Kolpe proba)
    char label;  // etiqueta de la grabación guiada ('R', 'L' o 0)
};
typedef void (*SampleSink)(int chan, const Sample& s);

void init();
// Tamaño de la pantalla 2D virtual (854×480 o 640×480): el puntero IR se da en esas coordenadas
void setPointerArea(int w, int h);
void update(float dt);
int batteryBars(int chan);  // 0..4 rayas de pila (−1 si el mando no está)
bool batteryLow(int chan);  // menos del 20 %
const Pad& pad(int chan);
int anyDown(u32 mask);
u32 allDown();
void rumble(int chan, float seconds);
void setRumbleEnabled(bool on);  // ajuste "Bibrazioa"
void stopRumble();
int connectedCount();
// Ajustes del jugador (se guardan en save.dat)
void setTiltInverted(bool inv);
void setSensitivity(int level);  // 0 = suave (golpes flojos cuentan), 1 = normal, 2 = hay que dar fuerte
void setRaiseInverted(bool inv);  // "arriba" = -Y en vez de +Y
void setSampleSink(SampleSink sink);
void setLabel(char label);  // etiqueta que se añade a cada lectura (grabación guiada)

}  // namespace input
