// Kolpe proba: muestra en vivo lo que detecta el Wii Remote, permite ajustar la sensibilidad y
// el lado, y graba los golpes en el USB (golpeak.csv) para afinar tuning.h con datos reales.
//
// Botones: 1 = empezar/parar grabación · cruceta ← / → = etiqueta (ezkerra / zuzen / eskuina)
//          cruceta ↑/↓ = invertir arriba/abajo · 2 = invertir izquierda/derecha · + / − = sensibilidad
//          B = salir (guarda ajustes)
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/tuning.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/scenes.h"

namespace {

enum Label { LBL_LEFT, LBL_STRAIGHT, LBL_RIGHT };
const char* labelName(int l) { return l == LBL_LEFT ? txt::LEFT : l == LBL_RIGHT ? txt::RIGHT : txt::STRAIGHT; }

const int HIST = 300;
const int MAX_REC = 30000;  // ~5 min a 100 lecturas/s

struct RecSample {
    input::Sample s;
    u8 label;
};

// Estado compartido con el callback de lecturas
float g_hist[HIST];
int g_histPos = 0;
int g_pad = 0;
RecSample* g_rec = nullptr;
int g_recCount = 0;
bool g_recording = false;
int g_label = LBL_STRAIGHT;
int g_recChops = 0;

void onSample(int chan, const input::Sample& s) {
    if (chan != g_pad) return;
    g_hist[g_histPos] = sqrtf(s.ax * s.ax + s.ay * s.ay + s.az * s.az);
    g_histPos = (g_histPos + 1) % HIST;
    if (g_recording && g_rec && g_recCount < MAX_REC) {
        g_rec[g_recCount].s = s;
        g_rec[g_recCount].label = (u8)g_label;
        g_recCount++;
        if (s.chop) g_recChops++;
    }
}

class SwingTestScene : public Scene {
public:
    SwingTestScene() {
        memset(g_hist, 0, sizeof(g_hist));
        g_histPos = 0;
        g_recording = false;
        g_recCount = 0;
        g_rec = (RecSample*)malloc(sizeof(RecSample) * MAX_REC);
        input::setSampleSink(onSample);
    }
    ~SwingTestScene() override {
        input::setSampleSink(nullptr);
        g_recording = false;
        free(g_rec);
        g_rec = nullptr;
    }

    void update(float dt) override {
        t_ += dt;
        for (int i = 0; i < input::MAX_PADS; i++)
            if (input::pad(i).down & WPAD_BUTTON_A) g_pad = i;
        const input::Pad& p = input::pad(g_pad);
        Settings& st = save::data().settings;
        u32 down = p.down;

        if (p.chop.happened) {
            last_ = p.chop;
            chops_++;
            flash_ = 1;
            audio::play(p.chop.strength > 0.5f ? SFX_CHOP : SFX_CHOP_WEAK, 0.6f);
            input::rumble(g_pad, 0.08f);
        }
        flash_ = fmaxf(0.0f, flash_ - dt * 3);
        msgT_ = fmaxf(0.0f, msgT_ - dt);

        if (down & WPAD_BUTTON_LEFT) g_label = g_label == LBL_RIGHT ? LBL_STRAIGHT : LBL_LEFT;
        if (down & WPAD_BUTTON_RIGHT) g_label = g_label == LBL_LEFT ? LBL_STRAIGHT : LBL_RIGHT;
        if (down & (WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT)) audio::play(SFX_MOVE, 0.5f);
        if (down & (WPAD_BUTTON_UP | WPAD_BUTTON_DOWN)) {
            st.raiseInverted = !st.raiseInverted;
            save::applySettings();
            audio::play(SFX_OK, 0.6f);
        }
#ifndef DEBUG_INPUT  // en las versiones de prueba 1, 2 y − simulan golpes
        if (down & WPAD_BUTTON_2) {
            st.tiltInverted = !st.tiltInverted;
            save::applySettings();
            audio::play(SFX_OK, 0.6f);
        }
        if (down & WPAD_BUTTON_MINUS) {
            if (st.sensitivity > 0) st.sensitivity--;
            save::applySettings();
            audio::play(SFX_MOVE, 0.6f);
        }
        if (down & WPAD_BUTTON_1) toggleRecording();
#endif
        if (down & WPAD_BUTTON_PLUS) {
            if (st.sensitivity < 2) st.sensitivity++;
            save::applySettings();
            audio::play(SFX_MOVE, 0.6f);
        }
        if (g_recording && g_recCount >= MAX_REC) toggleRecording();
        if (input::allDown() & WPAD_BUTTON_B) {
            if (g_recording) toggleRecording();
            save::write();
            audio::play(SFX_BACK, 0.7f);
            scenes::go(makeSetupScene());
        }
    }

    void toggleRecording() {
        if (!g_recording) {
            if (!save::available()) {
                showMsg(txt::NO_SAVE);
                audio::play(SFX_BAD, 0.6f);
                return;
            }
            g_recCount = 0;
            g_recChops = 0;
            g_recording = true;
            audio::play(SFX_BEEP, 0.6f);
            return;
        }
        g_recording = false;
        audio::play(SFX_OK, 0.6f);
        writeCsv();
    }

    void writeCsv() {
        char path[300];
        snprintf(path, sizeof(path), "%sgolpeak.csv", save::directory());
        FILE* f = fopen(path, "r");
        bool exists = f != nullptr;
        if (f) fclose(f);
        f = fopen(path, "a");
        if (!f) {
            showMsg(txt::NO_SAVE);
            return;
        }
        if (!exists) fprintf(f, "saioa,ms,etiketa,ax,ay,az,gx,gy,gz,egoera,kolpea,sentikortasuna,alderantziz,goraalderantziz\n");
        session_++;
        const Settings& st = save::data().settings;
        u32 sessionId = (u32)(t_ * 1000) ^ ((u32)g_recCount << 8);
        for (int i = 0; i < g_recCount; i++) {
            const RecSample& r = g_rec[i];
            fprintf(f, "%u,%u,%c,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%u,%u,%u,%u,%u\n", sessionId, r.s.ms, "LZR"[r.label], r.s.ax,
                    r.s.ay, r.s.az, r.s.gx, r.s.gy, r.s.gz, r.s.state, r.s.chop, st.sensitivity, st.tiltInverted,
                    st.raiseInverted);
        }
        fclose(f);
        snprintf(msgBuf_, sizeof(msgBuf_), txt::REC_SAVED, g_recChops, g_recCount);
        showMsg(msgBuf_);
    }

    void showMsg(const char* m) {
        msg_ = m;
        msgT_ = 3.5f;
    }

    void draw() override;

private:
    float t_ = 0, flash_ = 0, msgT_ = 0;
    int chops_ = 0, session_ = 0;
    input::Chop last_;
    const char* msg_ = "";
    char msgBuf_[96];
};

void SwingTestScene::draw() {
    const float W = gfx::screenW();
    char buf[96];
    gfx::begin2D();
    ui::background(t_);
    ui::header(txt::SWING_TEST);
    const input::Pad& p = input::pad(g_pad);
    const Settings& st = save::data().settings;

    // Estado del gesto
    const char* stateText = txt::RAISE_REMOTE;
    Color sc = UI_TEXT;
    if (p.state == input::SW_RAISING) { stateText = "..."; }
    else if (p.state == input::SW_ARMED) { stateText = txt::ARMED; sc = UI_GREEN; }
    else if (p.state == input::SW_SWING || flash_ > 0.3f) { stateText = txt::HIT; sc = UI_RED; }
    font::draw(stateText, W - 40, 26, 30, sc == UI_TEXT ? UI_WHITE : sc, UI_BLUE_DARK, ALIGN_RIGHT);

    // Gráfica de |a| con los umbrales
    float gx = 50, gy = 96, gw = W - 100, gh = 100;
    ui::panel(gx - 10, gy - 10, gw + 20, gh + 20);
    const float maxG = 6.0f;
    auto yOf = [&](float g) { return gy + gh - clampv(g / maxG, 0.0f, 1.0f) * gh; };
    float minG = tuning::CHOP_MIN_G[st.sensitivity % 3];
    for (int i = 0; i < HIST; i++) {
        float g = g_hist[(g_histPos + i) % HIST];
        float x = gx + gw * i / HIST;
        float y = yOf(g);
        gfx::rect(x, y, gw / HIST + 0.6f, gy + gh - y, g >= minG ? UI_RED : g >= tuning::SWING_START_G ? UI_GOLD : UI_BLUE);
    }
    gfx::rect(gx, yOf(minG), gw, 2, withAlpha(UI_RED, 0.7f));
    gfx::rect(gx, yOf(1.0f), gw, 1, rgba(0x90, 0x98, 0xA0));
    snprintf(buf, sizeof(buf), "|a| %.2f g", length(p.accel));
    font::plain(buf, gx + 8, gy + 2, 18, UI_TEXT);

    // Subida e inclinación en vivo
    float by = 238, colW = W / 2 - 60;
    ui::panel(40, by, colW, 128);
    auto bar = [&](float y, const char* label, float v, bool centered, Color c) {
        font::plain(label, 60, y, 20, UI_TEXT);
        float bx = 60 + 150, bw = colW - 190;
        gfx::roundRect(bx, y + 4, bw, 16, 8, rgba(0xE0, 0xE8, 0xF0));
        if (centered) {
            gfx::rect(bx + bw / 2 - 1, y, 2, 24, UI_TEXT_LIGHT);
            gfx::circle(bx + bw / 2 + clampv(v, -1.0f, 1.0f) * bw / 2, y + 12, 10, c, 18);
        } else {
            float f = clampv(v, 0.0f, 1.0f);
            if (f > 0.02f) gfx::roundRect(bx, y + 4, bw * f, 16, 8, c);
        }
    };
    bar(by + 14, txt::RAISE_LABEL, (p.pitchUp + 1) * 0.5f, false, p.state == input::SW_ARMED ? UI_GREEN : UI_BLUE);
    bar(by + 48, txt::TILT_LABEL, p.tiltLive, true, UI_BLUE);
    snprintf(buf, sizeof(buf), "x %.2f  y %.2f  z %.2f", p.grav.x, p.grav.y, p.grav.z);
    font::plain(buf, 60, by + 88, 18, UI_TEXT_LIGHT);

    // Último golpe
    float cx = W / 2 + 20;
    ui::panel(cx, by, colW, 128, mixColor(UI_WHITE, rgba(0xFF, 0xF0, 0xB8), flash_));
    snprintf(buf, sizeof(buf), "%s: %d", txt::HITS, chops_);
    font::plain(buf, cx + 20, by + 10, 22, UI_TEXT);
    if (chops_) {
        const char* side = last_.tilt < -tuning::TILT_STRAIGHT ? txt::LEFT : last_.tilt > tuning::TILT_STRAIGHT ? txt::RIGHT : txt::STRAIGHT;
        snprintf(buf, sizeof(buf), "%s %d%%  ·  %s", txt::STRENGTH, (int)(last_.strength * 100), side);
        font::plain(buf, cx + 20, by + 44, 22, UI_TEXT);
        snprintf(buf, sizeof(buf), "pikoa %.1fg  okertu %+.2f  alboz %+.2f", last_.peakG, last_.rollPart, last_.latPart);
        font::plain(buf, cx + 20, by + 80, 16, UI_TEXT_LIGHT);
    }

    // Ajustes y grabación
    float sy = 380;
    const char* sens[3] = {txt::SENS_SOFT, txt::SENS_NORMAL, txt::SENS_HARD};
    snprintf(buf, sizeof(buf), "%s: %s", txt::SENSITIVITY, sens[st.sensitivity % 3]);
    ui::buttonIcon(52, sy + 12, "-", 12, rgba(0x80, 0x88, 0x94));
    ui::buttonIcon(80, sy + 12, "+", 12, rgba(0x80, 0x88, 0x94));
    font::plain(buf, 98, sy, 20, UI_TEXT);
    ui::buttonIcon(52, sy + 42, "2", 12, rgba(0x80, 0x88, 0x94));
    snprintf(buf, sizeof(buf), "%s: %s", txt::INVERT_LR, st.tiltInverted ? txt::YES : txt::NO);
    font::plain(buf, 72, sy + 30, 20, UI_TEXT);
    snprintf(buf, sizeof(buf), "%s: %s", txt::INVERT_UD, st.raiseInverted ? txt::YES : txt::NO);
    font::plain(buf, 50, by - 26, 18, UI_TEXT_LIGHT);

    float rx = W / 2 + 40;
    ui::buttonIcon(rx, sy + 12, "1", 12, g_recording ? UI_RED : rgba(0x80, 0x88, 0x94));
    if (g_recording) {
        if (fmodf(t_, 1.0f) < 0.6f) gfx::circle(rx + 24, sy + 12, 7, UI_RED, 14);
        snprintf(buf, sizeof(buf), "%s  %d", txt::RECORDING, g_recChops);
        font::plain(buf, rx + 38, sy, 20, UI_RED);
    } else {
        font::plain(txt::RECORD, rx + 20, sy, 20, UI_TEXT);
    }
    snprintf(buf, sizeof(buf), "%s: %s", txt::LABEL, labelName(g_label));
    font::plain(buf, rx + 20, sy + 30, 20, UI_TEXT);

    if (msgT_ > 0) {
        float a = clampv(msgT_ * 2, 0.0f, 1.0f);
        float tw = font::width(msg_, 22) + 40;
        gfx::roundRect(W / 2 - tw / 2, 200, tw, 40, 20, withAlpha(UI_BLUE_DARK, 0.9f * a));
        font::plain(msg_, W / 2, 208, 22, UI_WHITE, ALIGN_CENTER, a);
    }
    ui::hints(nullptr, txt::BACK);
}

}  // namespace

Scene* makeSwingTestScene() { return new SwingTestScene(); }
