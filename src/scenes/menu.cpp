// Menú principal, récords
#include <stdio.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/common.h"
#include "scenes/scenes.h"

namespace {

class MainMenu : public Scene {
public:
    explicit MainMenu(int sel) {
        menu_.count = 4;
        menu_.sel = sel;
    }

    void update(float dt) override {
        t_ += dt;
        int r = menu_.update(dt, input::allDown());
        if (r == 0) scenes::go(makeSetupScene());
        else if (r == 1) scenes::go(makeProfilesScene());
        else if (r == 2) scenes::go(makeRecordsScene());
        else if (r == 3) scenes::openExitDialog();  // el mismo diálogo que HOME
        else if (r == -2) scenes::go(makeTitleScene());
    }

    void draw() override {
        gfx::begin2D();
        ui::background(t_);
        const float W = gfx::screenW();
        drawCharacterPreview(Appearance::from(save::data().profiles[0]), 0.95f * W / 854.0f, sinf(t_ * 0.6f) * 0.5f, t_,
                             0);
        gfx::begin2D();
        ui::header(txt::GAME_TITLE);
        const char* items[4] = {txt::PLAY, txt::PLAYERS, txt::RECORDS, txt::EXIT};
        float cx = W * 0.36f;
        for (int i = 0; i < 4; i++) {
            float in = easeOutBack(clampv((t_ - i * 0.06f) / 0.35f, 0.0f, 1.0f));
            ui::menuButton(menu_, i, cx - (1 - in) * 300, 140 + i * 84, 360, 68, items[i]);
        }
        ui::hints(txt::SELECT, txt::BACK);
    }

private:
    ui::ListMenu menu_;
    float t_ = 0;
};

class RecordsScene : public Scene {
public:
    void update(float dt) override {
        t_ += dt;
        u32 down = input::allDown();
        // Puntero: A sobre una flecha cambia de página; A en otro sitio (o B) vuelve al menú
        ui::usePointer();
        if (down & WPAD_BUTTON_A) {
            const float W = gfx::screenW();
            if (ui::pointerOver(W / 2 - 170, 60, 50, 50) || ui::pointerOver(W / 2 + 120, 60, 50, 50))
                down = (down & ~WPAD_BUTTON_A) | WPAD_BUTTON_RIGHT;
        }
        if (down & (WPAD_BUTTON_B | WPAD_BUTTON_A)) scenes::go(makeMainMenu(2));
        if (down & (WPAD_BUTTON_LEFT | WPAD_BUTTON_RIGHT)) {
            page_ = 1 - page_;
            audio::play(SFX_MOVE, 0.6f);
        }
    }

    void draw() override {
        gfx::begin2D();
        ui::background(t_);
        ui::header(txt::RECORDS);
        const float W = gfx::screenW();
        float pw = 600, px = W / 2 - pw / 2;
        ui::panel(px, 100, pw, 300);
        char buf[64], tbuf[32];
        // Página: ← aizkolaritza / pala →
        font::draw(page_ == 0 ? txt::AIZKOLARITZA : txt::PALA, W / 2, 66, 30, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER);
        float pulse = 4 * sinf(t_ * 6);
        gfx::triangle(W / 2 - 150 - pulse, 84, W / 2 - 134 - pulse, 72, W / 2 - 134 - pulse, 96, UI_BLUE);
        gfx::triangle(W / 2 + 150 + pulse, 84, W / 2 + 134 + pulse, 72, W / 2 + 134 + pulse, 96, UI_BLUE);
        if (page_ == 1) {
            const PalaRecords& r = save::data().pala;
            const char* names[2] = {txt::REC_PALA_STREAK, txt::REC_PALA_RALLY};
            int vals[2] = {r.bestStreak, r.bestRally};
            const char* who[2] = {r.streakName, r.rallyName};
            for (int i = 0; i < 2; i++) {
                float y = 140 + i * 100;
                font::plain(names[i], px + 30, y, 26, UI_TEXT);
                if (vals[i]) {
                    snprintf(tbuf, sizeof(tbuf), txt::REC_HITS, vals[i]);
                    font::plain(tbuf, px + pw - 30, y + 36, 34, UI_BLUE_DARK, ALIGN_RIGHT);
                    font::plain(who[i], px + 30, y + 42, 22, UI_TEXT_LIGHT);
                } else {
                    font::plain(txt::NO_RECORD, px + pw - 30, y + 42, 22, UI_TEXT_LIGHT, ALIGN_RIGHT);
                }
                if (i == 0) gfx::rect(px + 24, y + 84, pw - 48, 2, rgba(0xE0, 0xE8, 0xF0));
            }
        }
        for (int i = 0; i < REC_COUNT && page_ == 0; i++) {
            float y = 124 + i * 66;
            if (i == REC_TRAINING) snprintf(buf, sizeof(buf), "%s", txt::REC_TRAINING);
            else snprintf(buf, sizeof(buf), txt::REC_COMP, i);
            font::plain(buf, px + 30, y, 26, UI_TEXT);
            const Record& r = save::data().records[i];
            if (r.timeMs) {
                ui::formatTime(r.timeMs, tbuf, sizeof(tbuf));
                font::plain(tbuf, px + pw - 30, y - 4, 34, UI_BLUE_DARK, ALIGN_RIGHT);
                font::plain(r.name, px + pw - 170, y + 2, 22, UI_TEXT_LIGHT, ALIGN_RIGHT);
            } else {
                font::plain(txt::NO_RECORD, px + pw - 30, y + 2, 22, UI_TEXT_LIGHT, ALIGN_RIGHT);
            }
            if (i < REC_COUNT - 1) gfx::rect(px + 24, y + 48, pw - 48, 2, rgba(0xE0, 0xE8, 0xF0));
        }
        if (!save::available()) font::plain(txt::NO_SAVE, W / 2, 408, 18, UI_TEXT_LIGHT, ALIGN_CENTER);
        ui::hints(nullptr, txt::BACK);
    }

private:
    float t_ = 0;
    int page_ = 0;
};

}  // namespace

Scene* makeMainMenu(int sel) { return new MainMenu(sel); }
Scene* makeRecordsScene() { return new RecordsScene(); }
