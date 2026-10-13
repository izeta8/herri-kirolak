#include "engine/input.h"
#include <ogc/lwp_watchdog.h>
#include "engine/telemetry.h"
#include "engine/tuning.h"

namespace input {

static Pad s_pads[MAX_PADS];
static float s_tiltSign = 1.0f;
static float s_raiseSign = 1.0f;
static float s_chopMinG = tuning::CHOP_MIN_G[1];
static SampleSink s_sink = nullptr;
static u64 s_t0 = 0;

// Lecturas recibidas desde el último frame (el mando manda ~100 por segundo)
static const int QUEUE = 48;
static Vec3 s_queue[MAX_PADS][QUEUE];
static int s_queued[MAX_PADS];

static void onData(s32 chan, const WPADData* d) {
    if (chan < 0 || chan >= MAX_PADS || !d) return;
    if (s_queued[chan] < QUEUE) s_queue[chan][s_queued[chan]++] = Vec3(d->gforce.x, d->gforce.y, d->gforce.z);
}

void init() {
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC);
    WPAD_SetIdleTimeout(300);
    s_t0 = gettime();
}

void setTiltInverted(bool inv) { s_tiltSign = inv ? -1.0f : 1.0f; }
void setSensitivity(int level) { s_chopMinG = tuning::CHOP_MIN_G[clampv(level, 0, 2)]; }
void setSampleSink(SampleSink sink) { s_sink = sink; }
void setRaiseInverted(bool inv) { s_raiseSign = inv ? -1.0f : 1.0f; }

static float rollOf(const Vec3& g) {
    return clampv(g.x / tuning::TILT_FULL_G, -1.0f, 1.0f) * tuning::TILT_SIGN * s_tiltSign;
}

// Procesa una lectura del acelerómetro
static void step(Pad& p, const Vec3& a, float dt) {
    using namespace tuning;
    p.accel = a;
    float m = length(a);
    // La gravedad solo se actualiza cuando el mando va despacio (si no, mide el movimiento)
    if (fabsf(m - 1.0f) < 0.3f) p.grav = approachv(p.grav, a, 10.0f, dt);
    float gl = fmaxf(length(p.grav), 0.5f);
    p.pitchUp = clampv(p.grav.y / gl * RAISE_SIGN * s_raiseSign, -1.0f, 1.0f);
    // Ángulo de subida (giro sobre el eje X): 0° = apuntando a la tele, 90° = punta arriba,
    // ~180° = por encima del hombro con los botones hacia abajo (así se sube un hacha)
    float deg = atan2f(p.grav.y * RAISE_SIGN * s_raiseSign, p.grav.z) / DEG;
    if (deg < -90.0f) deg += 360.0f;
    p.raiseDeg = deg;
    p.tiltLive = rollOf(p.grav);

    switch (p.state) {
        case SW_IDLE:
            if (p.raiseDeg > RAISE_ON_DEG) {
                p.state = SW_RAISING;
                p.timer = 0;
            }
            break;
        case SW_RAISING:  // tiene que mantenerse arriba un momento
            p.timer += dt;
            if (p.raiseDeg < RAISE_ON_DEG - 15.0f) p.state = SW_IDLE;
            else if (p.timer >= RAISE_HOLD) p.state = SW_ARMED;
            break;
        case SW_ARMED:
            if (m > SWING_START_G) {
                p.state = SW_SWING;
                p.timer = 0;
                p.peak = m;
                p.lat = 0;
                p.rollTop = p.tiltLive;
                p.gravXTop = p.grav.x;
            } else if (p.raiseDeg < RAISE_OFF_DEG) {
                p.state = SW_IDLE;  // lo ha bajado despacio: no es un golpe
            }
            break;
        case SW_SWING:
            p.timer += dt;
            p.peak = fmaxf(p.peak, m);
            p.lat += (a.x - p.gravXTop) * dt;  // empuje lateral acumulado (g·s)
            if ((m < SWING_END_G && p.timer > 0.04f) || p.timer > SWING_MAX_TIME) {
                if (p.peak >= s_chopMinG) {
                    Chop& c = p.chop;
                    c.happened = true;
                    c.peakG = p.peak;
                    c.strength = clampv((p.peak - s_chopMinG) / (CHOP_FULL_G - s_chopMinG) * 0.75f + 0.25f, 0.0f, 1.0f);
                    c.rollPart = p.rollTop;
                    c.latPart = clampv(p.lat / LAT_FULL_GS, -1.0f, 1.0f) * TILT_SIGN * s_tiltSign * LAT_SIGN;
                    c.tilt = clampv(c.rollPart * ROLL_WEIGHT + c.latPart * (1 - ROLL_WEIGHT), -1.0f, 1.0f);
                    p.state = SW_COOLDOWN;
                    p.timer = CHOP_COOLDOWN;
                } else {
                    p.state = SW_ARMED;  // amago: sigue armado
                }
            }
            break;
        case SW_COOLDOWN:
            p.timer -= dt;
            if (p.timer <= 0 && m < SWING_END_G) p.state = SW_IDLE;
            break;
    }
    // Altura del hacha para la animación
    float target = (p.state == SW_ARMED || p.state == SW_RAISING) ? 1.0f : clampv(p.raiseDeg / RAISE_ON_DEG, 0.0f, 1.0f);
    if (p.state == SW_SWING || p.state == SW_COOLDOWN) target = 0;
    p.raise = approach(p.raise, target, 14.0f, dt);
}

void update(float dt) {
    for (int i = 0; i < MAX_PADS; i++) s_queued[i] = 0;
    WPAD_ReadPending(WPAD_CHAN_ALL, onData);
    u32 ms = (u32)ticks_to_millisecs(gettime() - s_t0);
    for (int i = 0; i < MAX_PADS; i++) {
        Pad& p = s_pads[i];
        p.chop = Chop();
        u32 type;
        p.connected = WPAD_Probe(i, &type) == WPAD_ERR_NONE;
        if (!p.connected) {
            p.held = p.down = 0;
            p.state = SW_IDLE;
            continue;
        }
        p.held = WPAD_ButtonsHeld(i);
        p.down = WPAD_ButtonsDown(i);
        int n = s_queued[i];
        if (n == 0) {
            WPADData* d = WPAD_Data(i);
            if (d) {
                s_queue[i][0] = Vec3(d->gforce.x, d->gforce.y, d->gforce.z);
                n = 1;
            }
        }
        for (int k = 0; k < n; k++) {
            bool before = p.chop.happened;
            step(p, s_queue[i][k], dt / n);
            const Vec3& a = s_queue[i][k];
            Sample s = {ms - (u32)((n - 1 - k) * dt * 1000 / n), a.x, a.y, a.z, p.grav.x, p.grav.y, p.grav.z,
                        (u8)p.state, (u8)(p.chop.happened && !before), (u16)(p.held & 0xFFFF)};
            if (s_sink) s_sink(i, s);
            telemetry::push(i, s);
        }

#ifdef DEBUG_INPUT
        // Golpes simulados para probar en Dolphin: 1 = ezkerra, 2 = eskuina, − = recto
        if (p.down & (WPAD_BUTTON_1 | WPAD_BUTTON_2 | WPAD_BUTTON_MINUS)) {
            p.chop.happened = true;
            p.chop.strength = 0.9f;
            p.chop.tilt = (p.down & WPAD_BUTTON_1) ? -1.0f : (p.down & WPAD_BUTTON_2) ? 1.0f : 0.0f;
        }
#endif
        if (p.rumbleT > 0) {
            p.rumbleT -= dt;
            if (p.rumbleT <= 0) WPAD_Rumble(i, 0);
        }
    }
}

const Pad& pad(int chan) { return s_pads[clampv(chan, 0, MAX_PADS - 1)]; }

int anyDown(u32 mask) {
    for (int i = 0; i < MAX_PADS; i++)
        if (s_pads[i].down & mask) return i;
    return -1;
}

u32 allDown() {
    u32 r = 0;
    for (int i = 0; i < MAX_PADS; i++) r |= s_pads[i].down;
    return r;
}

void rumble(int chan, float seconds) {
    if (chan < 0 || chan >= MAX_PADS || !s_pads[chan].connected) return;
    s_pads[chan].rumbleT = seconds;
    WPAD_Rumble(chan, 1);
}

void stopRumble() {
    for (int i = 0; i < MAX_PADS; i++) {
        s_pads[i].rumbleT = 0;
        WPAD_Rumble(i, 0);
    }
}

int connectedCount() {
    int n = 0;
    for (int i = 0; i < MAX_PADS; i++) n += s_pads[i].connected;
    return n;
}

}  // namespace input
