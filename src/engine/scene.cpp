#include "engine/scene.h"
#include <stdio.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"

namespace scenes {

static Scene* s_cur = nullptr;
static Scene* s_next = nullptr;
static float s_fade = 1.0f;  // 1 = negro
static bool s_fadingOut = false;
static bool s_homeOpen = false;
static ui::ListMenu s_homeMenu;
static float s_homeAnim = 0;
static bool s_exit = false;

void start(Scene* first) {
    s_cur = first;
    s_fade = 1.0f;
}

void go(Scene* next) {
    if (s_next) delete s_next;
    s_next = next;
    s_fadingOut = true;
}

bool transitioning() { return s_fadingOut || s_next; }

static void drawHome() {
    float w = gfx::screenW();
    float a = s_homeAnim;
    gfx::begin2D();
    ui::fade(0.55f * a);
    float pw = 520, ph = 230;
    float py = 240 - ph / 2 + (1 - easeOutCubic(a)) * 60;
    ui::panel(w / 2 - pw / 2, py, pw, ph);
    font::plain(txt::EXIT_QUESTION, w / 2, py + 34, 32, UI_TEXT, ALIGN_CENTER);
    ui::menuButton(s_homeMenu, 0, w / 2 - 120, py + 150, 200, 70, txt::YES);
    ui::menuButton(s_homeMenu, 1, w / 2 + 120, py + 150, 200, 70, txt::NO);
    // Como el menú HOME de la Wii: las pilas de cada mando conectado
    int n = 0;
    for (int i = 0; i < input::MAX_PADS; i++) n += input::pad(i).connected;
    if (n > 0) {
        float bw = n * 110 + 20, bx = w / 2 - bw / 2, by = py + ph + 14;
        ui::panel(bx, by, bw, 46);
        float x = bx + 20;
        for (int i = 0; i < input::MAX_PADS; i++) {
            if (!input::pad(i).connected) continue;
            char num[4];
            snprintf(num, sizeof(num), "%d", i + 1);
            ui::colorDot(x + 10, by + 23, 10, rgba(0x2C, 0x9C, 0xE8));
            font::plain(num, x + 10, by + 12, 17, UI_WHITE, ALIGN_CENTER);
            ui::battery(x + 30, by + 14, input::batteryBars(i));
            x += 110;
        }
    }
}

// Aviso de pilas bajas: una vez cuando un mando baja del 20 % (y otra si se cambian y vuelven a bajar)
static bool s_lowShown[input::MAX_PADS];
static float s_lowT = 0;
static int s_lowPad = 0;
static void updateBatteryToast(float dt) {
    s_lowT = fmaxf(0.0f, s_lowT - dt);
    for (int i = 0; i < input::MAX_PADS; i++) {
        bool low = input::batteryLow(i);
        if (low && !s_lowShown[i]) {
            s_lowShown[i] = true;
            s_lowT = 5.0f;
            s_lowPad = i;
        } else if (!low && input::pad(i).connected) {
            s_lowShown[i] = false;
        }
    }
}

static void drawBatteryToast() {
    if (s_lowT <= 0) return;
    char buf[64];
    snprintf(buf, sizeof(buf), txt::BATTERY_LOW, s_lowPad + 1);
    float w = gfx::screenW(), tw = font::width(buf, 20) + 80;
    float a = clampv(s_lowT * 2, 0.0f, 1.0f), y = 12 - (1 - a) * 20;
    gfx::begin2D();
    gfx::roundRect(w / 2 - tw / 2, y, tw, 36, 18, withAlpha(rgba(255, 255, 255, 235), a));
    ui::battery(w / 2 - tw / 2 + 14, y + 9, 0);
    font::plain(buf, w / 2 + 22, y + 7, 20, withAlpha(UI_RED, a), ALIGN_CENTER);
}

static bool s_openRequest = false;
void openExitDialog() { s_openRequest = true; }

bool frame(float dt) {
    ui::updatePointer();
    updateBatteryToast(dt);
    // Diálogo de HOME: congela la escena actual
    bool justOpened = false;
    if (!s_homeOpen && !transitioning() && (s_openRequest || input::anyDown(WPAD_BUTTON_HOME) >= 0)) {
        s_openRequest = false;
        justOpened = true;
        s_homeOpen = true;
        s_homeMenu = ui::ListMenu();
        s_homeMenu.count = 2;
        s_homeMenu.horizontal = true;
        s_homeMenu.sel = 1;
        input::stopRumble();
        audio::play(SFX_OK, 0.6f);
    }
    if (justOpened) {
    } else if (s_homeOpen) {
        s_homeAnim = approach(s_homeAnim, 1.0f, 12.0f, dt);
        u32 down = input::allDown();
        int r = s_homeMenu.update(dt, down);
        if (r == 0) s_exit = true;
        if (r == 1 || r == -2 || (down & WPAD_BUTTON_HOME)) {
            s_homeOpen = false;
            s_homeAnim = 0;
        }
    } else if (s_fadingOut) {
        s_fade = fminf(1.0f, s_fade + dt * 4.0f);
        if (s_fade >= 1.0f) {
            s_fadingOut = false;
            if (s_next) {
                delete s_cur;
                s_cur = s_next;
                s_next = nullptr;
            }
        }
    } else {
        s_fade = fmaxf(0.0f, s_fade - dt * 4.0f);
        if (s_cur) s_cur->update(dt);
    }

    gfx::beginFrame();
    if (s_cur) s_cur->draw();
    gfx::begin2D();
    ui::fade(s_fade);
    if (s_homeOpen) drawHome();
    drawBatteryToast();
    ui::drawPointers();  // la mano de cada mando, si la escena (o el diálogo de HOME) usa el puntero
    gfx::endFrame();
    return !s_exit;
}

}  // namespace scenes
