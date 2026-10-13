// Kolpe proba: muestra en vivo los 3 ejes del acelerómetro, congela cada golpe para compararlos,
// dibuja un Wii Remote 3D con la orientación detectada, permite ajustar la sensibilidad y el lado,
// y graba los golpes en el USB (golpeak.csv) para afinar tuning.h con datos reales.
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
#include "engine/mesh.h"
#include "engine/save.h"
#include "engine/telemetry.h"
#include "engine/tuning.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/scenes.h"

namespace {

enum Label { LBL_LEFT, LBL_STRAIGHT, LBL_RIGHT };
const char* labelName(int l) { return l == LBL_LEFT ? txt::LEFT : l == LBL_RIGHT ? txt::RIGHT : txt::STRAIGHT; }

const int HIST = 300;          // 3 s en directo
const int PRE = 70, POST = 60; // ventana del golpe congelado (lecturas antes/después del disparo)
const int MAX_REC = 30000;     // ~5 min a 100 lecturas/s

struct RecSample {
    input::Sample s;
    u8 label;
};

struct Stroke {
    Vec3 a[PRE + POST];
    int n = 0;
    Vec3 gStart, gEnd;
    float peak = 0, lat = 0;
    bool chop = false;
};

// Estado compartido con el callback de lecturas
Vec3 g_hist[HIST];
int g_histPos = 0;
int g_pad = 0;
RecSample* g_rec = nullptr;
int g_recCount = 0;
bool g_recording = false;
int g_label = LBL_STRAIGHT;
int g_recChops = 0;
// captura del golpe
Vec3 g_pre[PRE];
Vec3 g_preG[PRE];
int g_prePos = 0;
bool g_capturing = false;
int g_postLeft = 0;
Stroke g_building, g_stroke;
bool g_hasStroke = false;
int g_strokeCount = 0;

void onSample(int chan, const input::Sample& s) {
    if (chan != g_pad) return;
    Vec3 a(s.ax, s.ay, s.az), g(s.gx, s.gy, s.gz);
    g_hist[g_histPos] = a;
    g_histPos = (g_histPos + 1) % HIST;
    float m = length(a);

    if (!g_capturing && m > tuning::SWING_START_G) {
        // Arranca la captura con lo que había justo antes
        g_capturing = true;
        g_postLeft = POST;
        g_building = Stroke();
        for (int i = 0; i < PRE; i++) g_building.a[g_building.n++] = g_pre[(g_prePos + i) % PRE];
        g_building.gStart = g_preG[g_prePos];  // la gravedad más antigua de la ventana (antes de moverse)
    }
    if (g_capturing) {
        g_building.a[g_building.n++] = a;
        g_building.peak = fmaxf(g_building.peak, m);
        g_building.lat += (a.x - g_building.gStart.x) * 0.01f;
        if (s.chop) g_building.chop = true;
        if (--g_postLeft <= 0 || g_building.n >= PRE + POST) {
            g_building.gEnd = g;
            g_stroke = g_building;
            g_hasStroke = true;
            g_strokeCount++;
            g_capturing = false;
        }
    }
    g_pre[g_prePos] = a;
    g_preG[g_prePos] = g;
    g_prePos = (g_prePos + 1) % PRE;

    if (g_recording && g_rec && g_recCount < MAX_REC) {
        g_rec[g_recCount].s = s;
        g_rec[g_recCount].label = (u8)g_label;
        g_recCount++;
        if (s.chop) g_recChops++;
    }
}

const Color CX = rgba(0xE8, 0x3A, 0x3A), CY = rgba(0x2E, 0xA8, 0x44), CZ = rgba(0x2C, 0x7C, 0xE8);

// Gráfica de los 3 ejes. `get(i)` devuelve la lectura i (0 = la más antigua)
template <typename F>
void axisGraph(float x, float y, float w, float h, int n, F get, float range) {
    gfx::roundRect(x, y, w, h, 10, rgba(0xF6, 0xFA, 0xFE));
    float mid = y + h / 2;
    for (int gl = -3; gl <= 3; gl++) {
        if (fabsf((float)gl) > range) continue;
        gfx::rect(x + 4, mid - gl / range * (h / 2 - 4), w - 8, gl == 0 ? 2 : 1,
                  gl == 0 ? rgba(0xA0, 0xA8, 0xB0) : rgba(0xDE, 0xE4, 0xEA));
    }
    if (n < 2) return;
    auto yOf = [&](float v) { return mid - clampv(v / range, -1.0f, 1.0f) * (h / 2 - 4); };
    const Color cols[3] = {CX, CY, CZ};
    for (int axis = 0; axis < 3; axis++) {
        GX_Begin(GX_LINESTRIP, GX_VTXFMT1, n);
        for (int i = 0; i < n; i++) {
            Vec3 v = get(i);
            float val = axis == 0 ? v.x : axis == 1 ? v.y : v.z;
            GX_Position3f32(x + 4 + (w - 8) * i / (n - 1), yOf(val), 0);
            GX_Color1u32(cols[axis]);
        }
        GX_End();
    }
}

Mesh s_remote;

void buildRemote() {
    if (s_remote.valid()) return;
    MeshBuilder b;
    // Ejes del mando: X a lo ancho, botones en +Z, la punta (sensor IR) en -Y
    const float L = 0.75f, W = 0.18f, T = 0.14f;
    const Color btn = rgba(0xC8, 0xCC, 0xD4);
    b.box(Vec3(0, 0, 0), Vec3(W, L, T), rgba(0xF4, 0xF4, 0xF6));
    b.box(Vec3(0, -L - 0.01f, 0), Vec3(W * 0.85f, 0.02f, T * 0.8f), rgba(0x20, 0x22, 0x2A));  // sensor IR
    b.box(Vec3(0, -L * 0.62f, T + 0.01f), Vec3(0.08f, 0.025f, 0.02f), rgba(0x50, 0x54, 0x5C));  // cruceta
    b.box(Vec3(0, -L * 0.62f, T + 0.01f), Vec3(0.025f, 0.08f, 0.02f), rgba(0x50, 0x54, 0x5C));
    b.sphere(Vec3(0, -L * 0.32f, T + 0.01f), Vec3(0.055f, 0.055f, 0.03f), btn, 12, 6);  // A
    b.box(Vec3(0, L * 0.55f, T + 0.005f), Vec3(0.03f, 0.03f, 0.015f), btn);             // 1
    b.box(Vec3(0, L * 0.72f, T + 0.005f), Vec3(0.03f, 0.03f, 0.015f), btn);             // 2
    for (int i = 0; i < 4; i++)
        b.box(Vec3(-0.09f + i * 0.06f, L * 0.88f, T + 0.003f), Vec3(0.012f, 0.012f, 0.01f), rgba(0x30, 0x90, 0xE8));
    // Flechas de los ejes (X rojo, Y verde hacia la punta... ojo: +Y va hacia atrás, Z azul hacia los botones)
    b.box(Vec3(W + 0.15f, 0, 0), Vec3(0.15f, 0.015f, 0.015f), CX);
    b.box(Vec3(0, L + 0.2f, 0), Vec3(0.015f, 0.18f, 0.015f), CY);
    b.box(Vec3(0, 0, T + 0.15f), Vec3(0.015f, 0.015f, 0.15f), CZ);
    b.build(s_remote);
}

// Dibuja el mando con la orientación que da la gravedad `g` (medida en ejes del mando)
void drawRemote3D(const Vec3& g, float screenX, float screenY) {
    buildRemote();
    Camera cam;
    float W = gfx::screenW();
    // Desplazar la cámara para que el mando aparezca en (screenX, screenY)
    float ox = (screenX / W - 0.5f) * 6.0f, oy = (screenY / 480.0f - 0.5f) * -3.3f;
    cam.eye = Vec3(-ox, -oy + 0.8f, 5.6f);
    cam.target = Vec3(-ox, -oy, 0);
    cam.fov = 45;
    gfx::begin3D(cam);
    // En un mando real, con la punta hacia arriba la gravedad sale en -Y: la lectura = "arriba" en ejes del mando
    // salvo el eje Y, que va al revés. Usamos la lectura tal cual y la punta en -Y.
    Vec3 up = length(g) > 0.3f ? normalize(g) : Vec3(0, 0, 1);  // "arriba" del mundo, en ejes del mando
    Vec3 tip(0, -1, 0);                                         // la punta del mando
    Vec3 fwd = tip - up * dot(tip, up);
    if (length(fwd) < 0.2f) fwd = Vec3(0, 0, 1) - up * up.z;
    fwd = normalize(fwd);
    Vec3 Zw = -fwd;  // la punta mirando "hacia la tele" (lejos de la cámara)
    Vec3 Xw = normalize(cross(up, Zw));
    // Columnas = imagen de cada eje del mando en el mundo
    Xf xf = Xf::fromBasis(Vec3(0, 0, 0), Vec3(Xw.x, up.x, Zw.x), Vec3(Xw.y, up.y, Zw.y), Vec3(Xw.z, up.z, Zw.z));
    gfx::drawMesh(s_remote, xf);
}

class SwingTestScene : public Scene {
public:
    SwingTestScene() {
        memset(g_hist, 0, sizeof(g_hist));
        g_histPos = 0;
        g_recording = false;
        g_recCount = 0;
        g_capturing = false;
        g_hasStroke = false;
        g_strokeCount = 0;
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
        if (down & WPAD_BUTTON_1) toggleRecording();
#endif
        // Marcas de gesto para la telemetría: + = empieza, − = termina
        if (down & WPAD_BUTTON_PLUS) {
            marking_ = true;
            markT_ = 0;
            audio::play(SFX_BEEP, 0.5f);
        }
        if ((down & WPAD_BUTTON_MINUS) && marking_) {
            marking_ = false;
            marks_++;
            lastMarkT_ = markT_;
            audio::play(SFX_OK, 0.6f);
        }
        if (marking_) markT_ += dt;
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
    bool marking_ = false;
    float markT_ = 0, lastMarkT_ = 0;
    int marks_ = 0;
    int chops_ = 0;
    input::Chop last_;
    const char* msg_ = "";
    char msgBuf_[96];
};

void SwingTestScene::draw() {
    const float W = gfx::screenW();
    char buf[112];
    gfx::begin2D();
    ui::background(t_);
    const input::Pad& p = input::pad(g_pad);
    const Settings& st = save::data().settings;

    // Mando 3D con la orientación actual (a la derecha)
    float rx = W - 120, ry = 190;
    ui::panel(rx - 100, 92, 200, 340);
    drawRemote3D(p.grav, rx, ry);
    gfx::begin2D();
    ui::header(txt::SWING_TEST);

    // Estado del gesto
    const char* stateText = txt::RAISE_REMOTE;
    Color sc = UI_WHITE;
    if (p.state == input::SW_RAISING) stateText = "...";
    else if (p.state == input::SW_ARMED) { stateText = txt::ARMED; sc = UI_GREEN; }
    else if (p.state == input::SW_SWING || flash_ > 0.3f) { stateText = txt::HIT; sc = UI_RED; }
    font::draw(stateText, W - 40, 26, 30, sc, UI_BLUE_DARK, ALIGN_RIGHT);

    snprintf(buf, sizeof(buf), "x %+.2f  y %+.2f  z %+.2f", p.grav.x, p.grav.y, p.grav.z);
    font::plain(buf, rx, 290, 16, UI_TEXT, ALIGN_CENTER);
    snprintf(buf, sizeof(buf), "%s: %d", txt::ANGLE, (int)p.raiseDeg);
    font::plain(buf, rx, 270, 18, p.state == input::SW_ARMED ? UI_GREEN : UI_TEXT, ALIGN_CENTER);

    // En directo: 3 ejes
    float gx = 30, gw = rx - 100 - 30 - 14;
    ui::panel(gx - 6, 92, gw + 12, 120);
    font::plain(txt::LIVE, gx + 6, 96, 18, UI_TEXT);
    font::plain("X", gx + gw - 66, 96, 18, CX);
    font::plain("Y", gx + gw - 44, 96, 18, CY);
    font::plain("Z", gx + gw - 22, 96, 18, CZ);
    axisGraph(gx, 118, gw, 88, HIST, [](int i) { return g_hist[(g_histPos + i) % HIST]; }, 4.0f);

    // Último golpe congelado
    ui::panel(gx - 6, 220, gw + 12, 212, mixColor(UI_WHITE, rgba(0xFF, 0xF0, 0xB8), flash_));
    snprintf(buf, sizeof(buf), "%s (%d)", txt::LAST_STROKE, g_strokeCount);
    font::plain(buf, gx + 6, 224, 18, UI_TEXT);
    if (g_hasStroke) {
        const Stroke& s = g_stroke;
        float graphW = gw * 0.62f;
        axisGraph(gx, 248, graphW, 176, s.n, [&](int i) { return s.a[i]; }, 5.5f);
        float tx = gx + graphW + 12;
        snprintf(buf, sizeof(buf), "%s %.1fg", txt::PEAK, s.peak);
        font::plain(buf, tx, 252, 18, s.chop ? UI_RED : UI_TEXT);
        snprintf(buf, sizeof(buf), "%s %+.2f", txt::LATERAL, s.lat);
        font::plain(buf, tx, 276, 18, UI_TEXT);
        font::plain(txt::START, tx, 304, 16, UI_TEXT_LIGHT);
        snprintf(buf, sizeof(buf), "%+.1f %+.1f %+.1f", s.gStart.x, s.gStart.y, s.gStart.z);
        font::plain(buf, tx, 322, 18, UI_TEXT);
        font::plain(txt::END, tx, 348, 16, UI_TEXT_LIGHT);
        snprintf(buf, sizeof(buf), "%+.1f %+.1f %+.1f", s.gEnd.x, s.gEnd.y, s.gEnd.z);
        font::plain(buf, tx, 366, 18, UI_TEXT);
        if (s.chop && chops_) {
            const char* side = last_.tilt < -tuning::TILT_STRAIGHT ? txt::LEFT
                             : last_.tilt > tuning::TILT_STRAIGHT  ? txt::RIGHT
                                                                    : txt::STRAIGHT;
            snprintf(buf, sizeof(buf), "%s %d%% · %s", txt::HIT, (int)(last_.strength * 100), side);
            font::plain(buf, tx, 396, 18, UI_RED);
        }
    } else {
        font::plain(txt::DO_A_STROKE, gx + gw / 2, 310, 20, UI_TEXT_LIGHT, ALIGN_CENTER);
    }

    // Ajustes y grabación (bajo el mando 3D)
    const char* sens[3] = {txt::SENS_SOFT, txt::SENS_NORMAL, txt::SENS_HARD};
    float sx = rx - 90, sy = 318;
    (void)sens;
    if (marking_) {
        snprintf(buf, sizeof(buf), "%s %.1fs", txt::GESTURE, markT_);
        font::draw(buf, sx, sy - 4, 22, UI_RED, UI_WHITE);
    } else {
        snprintf(buf, sizeof(buf), "+/-  %s: %d (%.1fs)", txt::GESTURES, marks_, lastMarkT_);
        font::plain(buf, sx, sy, 17, UI_TEXT);
    }
    snprintf(buf, sizeof(buf), "2:  %s %s", txt::LR_SHORT, st.tiltInverted ? "<->" : "ok");
    font::plain(buf, sx, sy + 22, 17, UI_TEXT);
    snprintf(buf, sizeof(buf), "+:  %s %s", txt::UD_SHORT, st.raiseInverted ? "<->" : "ok");
    buf[0] = '^';
    font::plain(buf, sx, sy + 44, 17, UI_TEXT);
    if (g_recording) {
        if (fmodf(t_, 1.0f) < 0.6f) gfx::circle(sx + 6, sy + 77, 6, UI_RED, 12);
        snprintf(buf, sizeof(buf), "%s %d", txt::RECORDING, g_recChops);
        font::plain(buf, sx + 16, sy + 68, 17, UI_RED);
    } else {
        snprintf(buf, sizeof(buf), "1:  %s", txt::RECORD);
        font::plain(buf, sx, sy + 68, 17, UI_TEXT);
    }
    snprintf(buf, sizeof(buf), "</>:  %s", labelName(g_label));
    font::plain(buf, sx, sy + 90, 17, UI_TEXT);

    if (msgT_ > 0) {
        float a = clampv(msgT_ * 2, 0.0f, 1.0f);
        float tw = font::width(msg_, 22) + 40;
        gfx::roundRect(W / 2 - tw / 2, 200, tw, 40, 20, withAlpha(UI_BLUE_DARK, 0.9f * a));
        font::plain(msg_, W / 2, 208, 22, UI_WHITE, ALIGN_CENTER, a);
    }
    const char* net[3] = {"WiFi: - %s", "WiFi: %s", "WiFi: %s  (PC konektatuta)"};
    snprintf(buf, sizeof(buf), net[clampv(telemetry::status(), 0, 2)], telemetry::ip());
    font::plain(buf, 40, 446, 20, telemetry::status() == 2 ? UI_GREEN : UI_TEXT);
    ui::hints(nullptr, txt::BACK);
}

}  // namespace

Scene* makeSwingTestScene() { return new SwingTestScene(); }
