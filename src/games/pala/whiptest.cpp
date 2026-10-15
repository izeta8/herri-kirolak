// Pala kolpe proba: el latigazo en directo (gráfica de |a| y de los 3 ejes), pico y fuerza de cada golpe, y
// grabación guiada para calibrar con datos: la pantalla pide golpes suaves, fuertes y de revés por turnos y la
// etiqueta (S / F / B) viaja con cada lectura por la telemetría (ver docs/GESTOS.md).
//
// Botones: A = usar este mando · 1 = empezar/parar la grabación guiada · B = salir
#include <stdio.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/telemetry.h"
#include "engine/tuning.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/scenes.h"

namespace {

const int HIST = 300;  // 3 s en directo
const int NUM_GUIDE = 3;
const char GUIDE_LABEL[NUM_GUIDE] = {'S', 'F', 'B'};  // suave, fuerte, revés

Vec3 g_hist[HIST];
int g_histPos = 0;
int g_pad = 0;
bool g_recording = false;
int g_guide = 0;
int g_guideCount[NUM_GUIDE];
bool g_inSwing = false;
int g_calm = 0;
float g_guideFlash = 0;

void onSample(int chan, const input::Sample& s) {
    if (chan != g_pad) return;
    Vec3 a(s.ax, s.ay, s.az);
    g_hist[g_histPos] = a;
    g_histPos = (g_histPos + 1) % HIST;
    if (!g_recording) return;
    // Un golpe = pasar de WHIP_START_G y luego 0,3 s de calma; entonces se pide el siguiente
    float m = length(a);
    if (!g_inSwing && m > tuning::WHIP_START_G) {
        g_inSwing = true;
        g_calm = 0;
    } else if (g_inSwing) {
        g_calm = m < 1.4f ? g_calm + 1 : 0;
        if (g_calm >= 30) {
            g_inSwing = false;
            g_guideCount[g_guide]++;
            g_guide = (g_guide + 1) % NUM_GUIDE;
            input::setLabel(GUIDE_LABEL[g_guide]);
            g_guideFlash = 1;
        }
    }
}

const Color CX = rgba(0xE8, 0x3A, 0x3A), CY = rgba(0x2E, 0xA8, 0x44), CZ = rgba(0x2C, 0x7C, 0xE8);
const Color CM = rgba(0x40, 0x44, 0x4C);

// Gráfica: los 3 ejes y |a| (más gruesa), con las líneas de inicio y de golpe mínimo
void graph(float x, float y, float w, float h, float range) {
    gfx::roundRect(x, y, w, h, 10, rgba(0xF6, 0xFA, 0xFE));
    float mid = y + h / 2;
    auto yOf = [&](float v) { return mid - clampv(v / range, -1.0f, 1.0f) * (h / 2 - 4); };
    gfx::rect(x + 4, mid, w - 8, 2, rgba(0xA0, 0xA8, 0xB0));
    gfx::rect(x + 4, yOf(tuning::WHIP_START_G), w - 8, 1, rgba(0xF0, 0xB0, 0x40));
    gfx::rect(x + 4, yOf(tuning::WHIP_MIN_G), w - 8, 1, UI_RED);
    const Color cols[4] = {CX, CY, CZ, CM};
    for (int axis = 0; axis < 4; axis++) {
        GX_Begin(GX_LINESTRIP, GX_VTXFMT1, HIST);
        for (int i = 0; i < HIST; i++) {
            Vec3 v = g_hist[(g_histPos + i) % HIST];
            float val = axis == 0 ? v.x : axis == 1 ? v.y : axis == 2 ? v.z : length(v);
            GX_Position3f32(x + 4 + (w - 8) * i / (HIST - 1), yOf(val), 0);
            GX_Color1u32(cols[axis]);
        }
        GX_End();
    }
}

class WhipTestScene : public Scene {
public:
    WhipTestScene() {
        for (Vec3& v : g_hist) v = Vec3();
        g_histPos = 0;
        g_recording = false;
        input::setSampleSink(onSample);
    }
    ~WhipTestScene() override {
        input::setSampleSink(nullptr);
        input::setLabel(0);
        g_recording = false;
    }

    void update(float dt) override {
        t_ += dt;
        for (int i = 0; i < input::MAX_PADS; i++)
            if (input::pad(i).down & WPAD_BUTTON_A) g_pad = i;
        const input::Pad& p = input::pad(g_pad);
        if (p.whip.happened) {
            last_ = p.whip;
            whips_++;
            flash_ = 1;
            audio::play(SFX_PALA, 0.5f + 0.5f * p.whip.strength);
            input::rumble(g_pad, 0.04f + 0.08f * p.whip.strength);
        }
        flash_ = fmaxf(0.0f, flash_ - dt * 2.5f);
        if (g_guideFlash > 0) {
            if (g_guideFlash >= 1.0f) audio::play(SFX_BEEP, 0.5f);
            g_guideFlash = fmaxf(0.0f, g_guideFlash - dt * 2 - 0.001f);
        }
#ifndef DEBUG_INPUT  // en las versiones de prueba 1 y 2 simulan latigazos
        if (p.down & WPAD_BUTTON_1) {
            g_recording = !g_recording;
            if (g_recording) {
                g_guide = 0;
                for (int& c : g_guideCount) c = 0;
                g_inSwing = false;
                input::setLabel(GUIDE_LABEL[0]);
                audio::play(SFX_BEEP, 0.6f);
            } else {
                input::setLabel(0);
                audio::play(SFX_OK, 0.6f);
            }
        }
#endif
        if (input::allDown() & WPAD_BUTTON_B) {
            audio::play(SFX_BACK, 0.7f);
            scenes::go(makeSetupScene());
        }
    }

    void draw() override {
        const float W = gfx::screenW();
        char buf[112];
        gfx::begin2D();
        ui::background(t_);
        ui::header(txt::WHIP_TEST);
        const input::Pad& p = input::pad(g_pad);

        float gx = 30, gw = W - 60 - 220;
        ui::panel(gx - 6, 92, gw + 12, 250);
        font::plain(txt::LIVE, gx + 6, 96, 18, UI_TEXT);
        font::plain("X", gx + gw - 92, 96, 18, CX);
        font::plain("Y", gx + gw - 70, 96, 18, CY);
        font::plain("Z", gx + gw - 48, 96, 18, CZ);
        font::plain("|a|", gx + gw - 4, 96, 18, CM, ALIGN_RIGHT);
        graph(gx, 118, gw, 218, 6.0f);

        // Último latigazo: fuerza grande y pico
        float px = W - 30 - 210;
        ui::panel(px, 92, 210, 250, mixColor(UI_WHITE, rgba(0xFF, 0xF0, 0xB8), flash_));
        snprintf(buf, sizeof(buf), "%s (%d)", txt::LAST_STROKE, whips_);
        font::plain(buf, px + 105, 100, 18, UI_TEXT, ALIGN_CENTER);
        if (whips_ > 0) {
            float mh = 150, my = 132, mx = px + 30;
            gfx::roundRect(mx, my, 30, mh, 15, rgba(255, 255, 255, 170));
            float f = clampv(last_.strength, 0.0f, 1.0f);
            if (f > 0.04f) gfx::roundRect(mx, my + mh * (1 - f), 30, mh * f, 15, mixColor(UI_GOLD, UI_RED, f));
            font::plain(txt::STRENGTH, mx + 15, my + mh + 6, 16, UI_TEXT, ALIGN_CENTER);
            snprintf(buf, sizeof(buf), "%d%%", (int)(last_.strength * 100));
            font::draw(buf, px + 140, 150, 44, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER);
            snprintf(buf, sizeof(buf), "%s %.1fg", txt::PEAK, last_.peakG);
            font::plain(buf, px + 140, 214, 20, UI_TEXT, ALIGN_CENTER);
        } else {
            font::plain(txt::DO_A_STROKE, px + 105, 200, 18, UI_TEXT_LIGHT, ALIGN_CENTER);
        }

        // Estado del detector y grabación
        bool swinging = p.whipSt == 1;
        font::draw(swinging || flash_ > 0.5f ? txt::HIT : "", W - 40, 26, 30, UI_RED, UI_BLUE_DARK, ALIGN_RIGHT);
        if (g_recording) {
            float bw = 460, bh = 74, bx = W / 2 - bw / 2, by = 352;
            float pop = 1.0f + 0.15f * g_guideFlash;
            const Color gc[NUM_GUIDE] = {rgba(0x3C, 0xB8, 0x48), UI_RED, rgba(0xE8, 0x7A, 0x1E)};
            gfx::roundRect(bx - 4, by - 4, bw + 8, bh + 8, 26, rgba(0, 0, 0, 60));
            gfx::roundRect(bx, by, bw, bh, 24, gc[g_guide]);
            font::draw(txt::WHIP_GUIDE[g_guide], W / 2, by + 10, 40 * pop, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_CENTER);
            snprintf(buf, sizeof(buf), "%s %d · %s %d · %s %d", txt::WHIP_GUIDE_SHORT[0], g_guideCount[0],
                     txt::WHIP_GUIDE_SHORT[1], g_guideCount[1], txt::WHIP_GUIDE_SHORT[2], g_guideCount[2]);
            font::plain(buf, W / 2, by + bh + 4, 18, UI_TEXT, ALIGN_CENTER);
            if (fmodf(t_, 1.0f) < 0.6f) gfx::circle(bx + 24, by + bh / 2, 8, UI_WHITE, 12);
        } else {
            snprintf(buf, sizeof(buf), "1:  %s", txt::RECORD);
            font::plain(buf, W / 2, 370, 22, UI_TEXT, ALIGN_CENTER);
        }
        const char* net[3] = {"WiFi: - %s", "WiFi: %s", "WiFi: %s  (PC konektatuta)"};
        snprintf(buf, sizeof(buf), net[clampv(telemetry::status(), 0, 2)], telemetry::ip());
        font::plain(buf, 40, 446, 20, telemetry::status() == 2 ? UI_GREEN : UI_TEXT);
        ui::hints(nullptr, txt::BACK);
    }

private:
    float t_ = 0, flash_ = 0;
    int whips_ = 0;
    input::Whip last_;
};

}  // namespace

Scene* makeWhipTestScene() { return new WhipTestScene(); }
