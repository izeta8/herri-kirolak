// Preparar partida: kirola → modua → jokalari kopurua → enbor kopurua → aizkolariak
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

enum Step { STEP_SPORT, STEP_MODE, STEP_PLAYERS, STEP_LOGS, STEP_CHARS };

class SetupScene : public Scene {
public:
    SetupScene() { enter(STEP_SPORT); }

    void update(float dt) override {
        t_ += dt;
        stepT_ += dt;
        u32 down = input::allDown();
        if (step_ == STEP_CHARS) {
            updateChars(dt, down);
            return;
        }
        int r = menu_.update(dt, down);
        if (r == -2) {
            back();
            return;
        }
        if (r < 0) return;
        switch (step_) {
            case STEP_SPORT:
                enter(STEP_MODE);
                break;
            case STEP_MODE:
                if (r == 2) {
                    scenes::go(makeSwingTestScene());
                    break;
                }
                cfg_.training = r == 0;
                if (cfg_.training) {
                    cfg_.numPlayers = 1;
                    cfg_.numLogs = 1;
                    enter(STEP_CHARS);
                } else {
                    enter(STEP_PLAYERS);
                }
                break;
            case STEP_PLAYERS:
                cfg_.numPlayers = r + 1;
                enter(STEP_LOGS);
                break;
            case STEP_LOGS:
                cfg_.numLogs = r + 1;
                enter(STEP_CHARS);
                break;
            default:
                break;
        }
    }

    void draw() override;

private:
    void enter(Step s) {
        step_ = s;
        stepT_ = 0;
        menu_ = ui::ListMenu();
        switch (s) {
            case STEP_SPORT:
                menu_.count = 4;
                for (int i = 1; i < 4; i++) menu_.enabled[i] = false;
                break;
            case STEP_MODE:
                menu_.count = 3;
                menu_.sel = cfg_.training ? 0 : 1;
                break;
            case STEP_PLAYERS:
                menu_.count = 4;
                menu_.horizontal = true;
                menu_.sel = cfg_.numPlayers - 1;
                break;
            case STEP_LOGS:
                menu_.count = 3;
                menu_.horizontal = true;
                menu_.sel = cfg_.numLogs - 1;
                break;
            case STEP_CHARS:
                charIdx_ = 0;
                for (int i = 0; i < 4; i++) {
                    cfg_.profile[i] = nextFree(i, -1, 1);
                    cfg_.bot[i] = false;
                }
                break;
        }
    }

    void back() {
        switch (step_) {
            case STEP_SPORT: scenes::go(makeMainMenu(0)); break;
            case STEP_MODE: enter(STEP_SPORT); break;
            case STEP_PLAYERS: enter(STEP_MODE); break;
            case STEP_LOGS: enter(STEP_PLAYERS); break;
            case STEP_CHARS:
                if (charIdx_ > 0) charIdx_--;
                else enter(cfg_.training ? STEP_MODE : STEP_LOGS);
                break;
        }
    }

    bool takenByOther(int profile, int player) const {
        for (int i = 0; i < player; i++)
            if (cfg_.profile[i] == profile) return true;
        return false;
    }

    // Siguiente personaje libre a partir de `from` en la dirección `dir`
    int nextFree(int player, int from, int dir) const {
        int p = from;
        for (int k = 0; k < MAX_PROFILES; k++) {
            p = (p + dir + MAX_PROFILES) % MAX_PROFILES;
            if (save::data().profiles[p].used && !takenByOther(p, player)) return p;
        }
        return from < 0 ? 0 : from;
    }

    void updateChars(float dt, u32 down) {
        (void)dt;
        int& prof = cfg_.profile[charIdx_];
        if (down & (WPAD_BUTTON_LEFT | WPAD_BUTTON_UP)) {
            prof = nextFree(charIdx_, prof, -1);
            audio::play(SFX_MOVE, 0.6f);
            spin_ = 1;
        }
        if (down & (WPAD_BUTTON_RIGHT | WPAD_BUTTON_DOWN)) {
            prof = nextFree(charIdx_, prof, 1);
            audio::play(SFX_MOVE, 0.6f);
            spin_ = 1;
        }
        if (!cfg_.training && charIdx_ > 0 && (down & WPAD_BUTTON_MINUS)) {
            cfg_.bot[charIdx_] = !cfg_.bot[charIdx_];
            audio::play(SFX_MOVE, 0.6f);
        }
        if (down & WPAD_BUTTON_A) {
            audio::play(SFX_OK, 0.8f);
            if (charIdx_ + 1 < cfg_.numPlayers) {
                charIdx_++;
                cfg_.profile[charIdx_] = nextFree(charIdx_, -1, 1);
                spin_ = 1;
            } else {
                scenes::go(makeAizkoScene(cfg_));
            }
        } else if (down & WPAD_BUTTON_B) {
            audio::play(SFX_BACK, 0.7f);
            back();
        }
    }

    AizkoConfig cfg_;
    Step step_ = STEP_SPORT;
    ui::ListMenu menu_;
    float t_ = 0, stepT_ = 0, spin_ = 0;
    int charIdx_ = 0;
};

void SetupScene::draw() {
    const float W = gfx::screenW();
    char buf[64];
    gfx::begin2D();
    ui::background(t_);
    float in = easeOutCubic(stepT_ / 0.3f);
    float slide = (1 - in) * 80;

    switch (step_) {
        case STEP_SPORT: {
            ui::header(txt::CHOOSE_SPORT);
            const char* names[4] = {txt::AIZKOLARITZA, txt::HARRI, txt::SOKATIRA, txt::TRONTZA};
            for (int i = 0; i < 4; i++)
                ui::button(W / 2 + slide, 140 + i * 78, 470, 64, names[i], menu_.anim[i], menu_.enabled[i],
                           i == 0 ? txt::AIZKO_DESC : txt::SOON);
            break;
        }
        case STEP_MODE: {
            ui::header(txt::CHOOSE_MODE);
            ui::button(W / 2 + slide, 150, 540, 84, txt::TRAINING, menu_.anim[0], true, txt::TRAINING_DESC);
            ui::button(W / 2 + slide, 255, 540, 84, txt::COMPETITION, menu_.anim[1], true, txt::COMPETITION_DESC);
            ui::button(W / 2 + slide, 360, 540, 84, txt::SWING_TEST, menu_.anim[2], true, txt::SWING_TEST_DESC);
            break;
        }
        case STEP_PLAYERS: {
            ui::header(txt::COMPETITION);
            font::plain(txt::HOW_MANY_PLAYERS, W / 2, 130, 36, UI_TEXT, ALIGN_CENTER);
            for (int i = 0; i < 4; i++) {
                snprintf(buf, sizeof(buf), "%d", i + 1);
                float x = W / 2 + (i - 1.5f) * 150 + slide;
                ui::button(x, 270, 120, 120, buf, menu_.anim[i]);
                for (int k = 0; k <= i; k++) ui::colorDot(x - i * 9 + k * 18, 352, 6, FAV_COLORS[(k * 5) % NUM_COLORS]);
            }
            break;
        }
        case STEP_LOGS: {
            ui::header(txt::COMPETITION);
            font::plain(txt::HOW_MANY_LOGS, W / 2, 130, 36, UI_TEXT, ALIGN_CENTER);
            for (int i = 0; i < 3; i++) {
                snprintf(buf, sizeof(buf), "%d", i + 1);
                ui::button(W / 2 + (i - 1) * 180 + slide, 270, 140, 120, buf, menu_.anim[i]);
            }
            break;
        }
        case STEP_CHARS: {
            const Profile& pr = save::data().profiles[cfg_.profile[charIdx_]];
            spin_ = approach(spin_, 0, 4, 1.0f / 60.0f);
            drawCharacterPreview(Appearance::from(pr), 0, sinf(t_ * 0.8f) * 0.4f + spin_ * 6.28f, t_, 0);
            gfx::begin2D();
            ui::header(txt::CHOOSE_CHAR);
            if (cfg_.numPlayers > 1) {
                snprintf(buf, sizeof(buf), txt::PLAYER_N, charIdx_ + 1);
                font::draw(buf, W / 2, 92, 32, UI_WHITE, UI_BLUE_DARK, ALIGN_CENTER);
            }
            // Nombre con flechas
            float y = 360;
            gfx::roundRect(W / 2 - 170, y, 340, 58, 29, rgba(255, 255, 255, 230));
            ui::colorDot(W / 2 - 130, y + 29, 11, FAV_COLORS[pr.color % NUM_COLORS]);
            font::plain(pr.name, W / 2 + 10, y + 9, 34, UI_TEXT, ALIGN_CENTER);
            float pulse = 4 * sinf(t_ * 6);
            gfx::triangle(W / 2 - 200 - pulse, y + 29, W / 2 - 182 - pulse, y + 14, W / 2 - 182 - pulse, y + 44, UI_BLUE);
            gfx::triangle(W / 2 + 200 + pulse, y + 29, W / 2 + 182 + pulse, y + 14, W / 2 + 182 + pulse, y + 44, UI_BLUE);
            if (cfg_.bot[charIdx_]) font::draw(txt::CPU, W / 2 + 120, y - 30, 30, UI_GOLD, rgba(0x60, 0x30, 0x10), ALIGN_CENTER);
            if (!cfg_.training && charIdx_ > 0) {
                ui::buttonIcon(40, 410, "-", 14, rgba(0x80, 0x88, 0x94));
                font::plain(txt::CPU, 62, 398, 22, UI_TEXT);
            }
            break;
        }
    }
    ui::hints(step_ == STEP_CHARS ? txt::OK : txt::SELECT, txt::BACK);
}

}  // namespace

Scene* makeSetupScene() { return new SetupScene(); }
