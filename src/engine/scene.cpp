#include "engine/scene.h"
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
    ui::button(w / 2 - 120, py + 150, 200, 70, txt::YES, s_homeMenu.anim[0]);
    ui::button(w / 2 + 120, py + 150, 200, 70, txt::NO, s_homeMenu.anim[1]);
}

static bool s_openRequest = false;
void openExitDialog() { s_openRequest = true; }

bool frame(float dt) {
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
    gfx::endFrame();
    return !s_exit;
}

}  // namespace scenes
