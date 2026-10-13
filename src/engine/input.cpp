#include "engine/input.h"
#include "engine/tuning.h"

namespace input {

static Pad s_pads[MAX_PADS];

void init() {
    WPAD_Init();
    WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC);
    WPAD_SetIdleTimeout(300);
}

static void detectChop(Pad& p, float dt) {
    float m = length(p.accel);
    p.motion = m - 1.0f;
    switch (p.state) {
        case 0:  // preparado
            if (m > tuning::SWING_START_G) {
                p.state = 1;
                p.timer = 0;
                p.peak = m;
                p.sumX = p.accel.x;
                p.samples = 1;
                p.tiltStart = p.tiltLive;
            }
            break;
        case 1:  // en pleno golpe
            p.timer += dt;
            p.peak = fmaxf(p.peak, m);
            p.sumX += p.accel.x;
            p.samples++;
            if (m < tuning::SWING_END_G || p.timer > tuning::SWING_MAX_TIME) {
                if (p.peak >= tuning::CHOP_MIN_G) {
                    p.chop.happened = true;
                    p.chop.strength = clampv((p.peak - tuning::CHOP_MIN_G) / (tuning::CHOP_FULL_G - tuning::CHOP_MIN_G) * 0.75f + 0.25f, 0.0f, 1.0f);
                    float swingX = clampv(p.sumX / p.samples / tuning::TILT_FULL_G, -1.0f, 1.0f) * tuning::TILT_SIGN;
                    p.chop.tilt = clampv(p.tiltStart * 0.5f + swingX * 0.5f, -1.0f, 1.0f);
                }
                p.state = 2;
                p.timer = tuning::CHOP_COOLDOWN;
            }
            break;
        case 2:  // enfriamiento
            p.timer -= dt;
            if (p.timer <= 0 && m < tuning::SWING_END_G) p.state = 0;
            break;
    }
}

void update(float dt) {
    WPAD_ScanPads();
    for (int i = 0; i < MAX_PADS; i++) {
        Pad& p = s_pads[i];
        p.chop = Chop();
        u32 type;
        p.connected = WPAD_Probe(i, &type) == WPAD_ERR_NONE;
        if (!p.connected) {
            p.held = p.down = 0;
            p.state = 0;
            continue;
        }
        p.held = WPAD_ButtonsHeld(i);
        p.down = WPAD_ButtonsDown(i);
        WPADData* d = WPAD_Data(i);
        if (d) p.accel = Vec3(d->gforce.x, d->gforce.y, d->gforce.z);
        float m = length(p.accel);
        if (fabsf(m - 1.0f) < 0.25f) p.grav = approachv(p.grav, p.accel, 6.0f, dt);
        p.tiltLive = clampv(p.grav.x / tuning::TILT_FULL_G, -1.0f, 1.0f) * tuning::TILT_SIGN;
        detectChop(p, dt);

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
