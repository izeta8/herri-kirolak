// Aukerak: ajustes del juego. Cada fila cambia su valor con la cruceta (←/→) o con A (también con el puntero: A sobre
// la fila o sobre sus flechas). B guarda y vuelve al menú. Abajo se ve dónde se guarda la partida.
#include <stdio.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/scenes.h"

namespace {

enum Row { R_RUMBLE, R_LANDING, R_SENS, R_LR, R_UD, R_COUNT };

class SettingsScene : public Scene {
public:
    SettingsScene() {
        menu_.count = R_COUNT;
        before_ = save::data().settings;
    }

    void update(float dt) override {
        t_ += dt;
        u32 down = input::allDown();
        Settings& st = save::data().settings;
        int dir = (down & WPAD_BUTTON_RIGHT) ? 1 : (down & WPAD_BUTTON_LEFT) ? -1 : 0;
        // Con el puntero, A sobre la flecha izquierda cambia hacia atrás; A en el resto de la fila, hacia delante
        if ((down & WPAD_BUTTON_A) && menu_.hovered() >= 0) {
            int i = menu_.hovered();
            dir = ui::pointerOver(menu_.rx[i] + menu_.rw[i] - 220, menu_.ry[i], 60, menu_.rh[i]) ? -1 : 1;
            menu_.sel = i;
            down &= ~WPAD_BUTTON_A;
        } else if (down & WPAD_BUTTON_A) {
            dir = 1;
            down &= ~WPAD_BUTTON_A;
        }
        // En esta pantalla izquierda/derecha cambian el valor, no la fila
        int r = menu_.update(dt, down & ~(WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT));
        if (dir) {
            switch (menu_.sel) {
                case R_RUMBLE: st.rumbleOff = !st.rumbleOff; break;
                case R_LANDING: st.hideLanding = !st.hideLanding; break;
                case R_SENS: st.sensitivity = (u8)((st.sensitivity + dir + 3) % 3); break;
                case R_LR: st.tiltInverted = !st.tiltInverted; break;
                case R_UD: st.raiseInverted = !st.raiseInverted; break;
            }
            save::applySettings();
            audio::play(SFX_MOVE, 0.6f);
            if (menu_.sel == R_RUMBLE && !st.rumbleOff) input::rumble(ui::pointer().valid ? ui::pointer().pad : 0, 0.15f);
            flash_[menu_.sel] = 1.0f;
        }
        for (float& f : flash_) f = fmaxf(0.0f, f - dt * 3);
        if (r == -2) {
            // Guardar solo si algo ha cambiado
            const Settings& a = before_;
            if (a.rumbleOff != st.rumbleOff || a.hideLanding != st.hideLanding || a.sensitivity != st.sensitivity ||
                a.tiltInverted != st.tiltInverted || a.raiseInverted != st.raiseInverted)
                save::write();
            scenes::go(makeMainMenu(3));
        }
    }

    void draw() override {
        const float W = gfx::screenW();
        char buf[160];
        gfx::begin2D();
        ui::background(t_);
        ui::header(txt::SETTINGS);
        const Settings& st = save::data().settings;
        const char* labels[R_COUNT] = {txt::SET_RUMBLE, txt::SET_LANDING, txt::SET_AIZKO_SENS, txt::SET_AIZKO_LR,
                                       txt::SET_AIZKO_UD};
        const char* sens[3] = {txt::SENS_SOFT, txt::SENS_NORMAL, txt::SENS_HARD};
        float pw = fminf(640.0f, W - 60), px = W / 2 - pw / 2;
        for (int i = 0; i < R_COUNT; i++) {
            float y = 104 + i * 56, a = menu_.anim[i];
            menu_.place(i, px, y, pw, 46);
            ui::row(px, y, pw, 46, a);
            font::plain(labels[i], px + 26, y + 10, 24, UI_TEXT);
            const char* v = nullptr;
            switch (i) {
                case R_RUMBLE: v = st.rumbleOff ? txt::NO : txt::YES; break;
                case R_LANDING: v = st.hideLanding ? txt::NO : txt::YES; break;
                case R_SENS: v = sens[st.sensitivity % 3]; break;
                case R_LR: v = st.tiltInverted ? txt::YES : txt::NO; break;
                case R_UD: v = st.raiseInverted ? txt::YES : txt::NO; break;
            }
            float vx = px + pw - 110, s = 1.0f + 0.15f * flash_[i];
            font::draw(v, vx, y + 8 - 2 * flash_[i], 26 * s, UI_TEAL, UI_WHITE, ALIGN_CENTER);
            if (a > 0.5f) {
                gfx::triangle(vx - 80, y + 23, vx - 68, y + 14, vx - 68, y + 32, UI_TEAL);
                gfx::triangle(vx + 80, y + 23, vx + 68, y + 14, vx + 68, y + 32, UI_TEAL);
            }
        }
        if (save::available()) snprintf(buf, sizeof(buf), txt::SAVE_WHERE, save::location());
        else snprintf(buf, sizeof(buf), "%s", txt::NO_SAVE);
        float tw = font::width(buf, 17) + 36;
        gfx::roundRect(W / 2 - tw / 2, 380, tw, 28, 14, rgba(255, 255, 255, 200));
        font::plain(buf, W / 2, 384, 17, save::available() ? UI_TEXT_LIGHT : UI_RED, ALIGN_CENTER);
        ui::hints(txt::EDIT, txt::BACK);
    }

private:
    ui::ListMenu menu_;
    Settings before_;
    float t_ = 0;
    float flash_[R_COUNT] = {};
};

}  // namespace

Scene* makeSettingsScene() { return new SettingsScene(); }
