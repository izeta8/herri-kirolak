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
#include "game/sports.h"
#include "scenes/scenes.h"

namespace {

enum Step { STEP_SPORT, STEP_MODE, STEP_PLAYERS, STEP_ROUNDS, STEP_LEVEL, STEP_CHARS };

// Lo último que se eligió (mientras el juego esté encendido): "Jokatu" empieza con eso marcado
MatchConfig s_last;

class SetupScene : public Scene {
public:
    SetupScene() {
        cfg_ = s_last;
        cfg_.demo = false;
        enter(STEP_SPORT);
    }

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
                cfg_.sport = r;
                enter(STEP_MODE);
                break;
            case STEP_MODE:
                if (r == 2) {
                    scenes::go(sport(cfg_.sport).makeGestureTest());
                    break;
                }
                cfg_.training = r == 0;
                if (cfg_.training) {
                    cfg_.numPlayers = 1;
                    cfg_.rounds = 1;
                    enter(STEP_CHARS);
                } else {
                    enter(STEP_PLAYERS);
                }
                break;
            case STEP_PLAYERS:
                cfg_.numPlayers = r + 1;
                enter(STEP_ROUNDS);
                break;
            case STEP_ROUNDS:
                cfg_.rounds = r + 1;
                enter(sport(cfg_.sport).botLevels ? STEP_LEVEL : STEP_CHARS);
                break;
            case STEP_LEVEL:
                cfg_.level = r;
                enter(STEP_CHARS);
                break;
            default:
                break;
        }
    }

    void draw() override;
    void drawSteps();

private:
    void enter(Step s) {
        step_ = s;
        stepT_ = 0;
        menu_ = ui::ListMenu();
        switch (s) {
            case STEP_SPORT:
                menu_.count = sportCount();
                menu_.sel = cfg_.sport;
                for (int i = 0; i < sportCount(); i++) menu_.enabled[i] = sport(i).makeMatch != nullptr;
                break;
            case STEP_MODE:
                menu_.count = sport(cfg_.sport).makeGestureTest ? 3 : 2;
                menu_.sel = cfg_.training ? 0 : 1;
                break;
            case STEP_PLAYERS:
                menu_.count = clampv(sport(cfg_.sport).maxPlayers, 1, 4);
                menu_.horizontal = true;
                menu_.sel = clampv(cfg_.numPlayers - 1, 0, menu_.count - 1);  // lo recordado puede no caber en este deporte
                break;
            case STEP_ROUNDS:
                menu_.count = clampv(sport(cfg_.sport).maxRounds, 1, 3);
                menu_.horizontal = true;
                menu_.sel = clampv(cfg_.rounds - 1, 0, menu_.count - 1);
                break;
            case STEP_LEVEL:
                menu_.count = 3;
                menu_.horizontal = true;
                menu_.sel = clampv(cfg_.level, 0, 2);
                break;
            case STEP_CHARS:
                charIdx_ = 0;
                // Los mismos personajes que la última vez (si siguen existiendo y no se repiten); si no, el siguiente libre
                for (int i = 0; i < 4; i++) {
                    int p = s_last.profile[i];
                    bool ok = p >= 0 && p < MAX_PROFILES && save::data().profiles[p].used && !takenByOther(p, i);
                    cfg_.profile[i] = ok ? p : nextFree(i, -1, 1);
                    cfg_.bot[i] = i > 0 && s_last.bot[i];
                }
                break;
        }
    }

    void back() {
        switch (step_) {
            case STEP_SPORT: scenes::go(makeMainMenu(0)); break;
            case STEP_MODE: enter(STEP_SPORT); break;
            case STEP_PLAYERS: enter(STEP_MODE); break;
            case STEP_ROUNDS: enter(STEP_PLAYERS); break;
            case STEP_LEVEL: enter(STEP_ROUNDS); break;
            case STEP_CHARS:
                if (charIdx_ > 0) charIdx_--;
                else enter(cfg_.training ? STEP_MODE : sport(cfg_.sport).botLevels ? STEP_LEVEL : STEP_ROUNDS);
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
        // Puntero: A sobre una flecha cambia de personaje; A sobre "CPU" lo cambia; A en otro sitio, aceptar
        ui::usePointer();
        if (down & WPAD_BUTTON_A) {
            const float W = gfx::screenW(), y = 360;
            if (ui::pointerOver(W / 2 - 225, y, 50, 58)) down = (down & ~WPAD_BUTTON_A) | WPAD_BUTTON_LEFT;
            else if (ui::pointerOver(W / 2 + 175, y, 50, 58)) down = (down & ~WPAD_BUTTON_A) | WPAD_BUTTON_RIGHT;
            else if (!cfg_.training && charIdx_ > 0 && ui::pointerOver(20, 392, 110, 40)) down = (down & ~WPAD_BUTTON_A) | WPAD_BUTTON_MINUS;
        }
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
                s_last = cfg_;
                scenes::goMatch(sport(cfg_.sport).makeMatch, cfg_);
            }
        } else if (down & WPAD_BUTTON_B) {
            audio::play(SFX_BACK, 0.7f);
            back();
        }
    }

    MatchConfig cfg_;
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
            float gap = sportCount() > 1 ? fminf(78.0f, 280.0f / (sportCount() - 1)) : 78.0f;  // que quepan todos
            for (int i = 0; i < sportCount(); i++)
                ui::menuButton(menu_, i, W / 2 + slide, 132 + i * gap, 470, gap - 12, sport(i).name, sport(i).desc);
            break;
        }
        case STEP_MODE: {
            ui::header(txt::CHOOSE_MODE);
            ui::menuButton(menu_, 0, W / 2 + slide, 150, 540, 84, txt::TRAINING, txt::TRAINING_DESC);
            ui::menuButton(menu_, 1, W / 2 + slide, 255, 540, 84, txt::COMPETITION, txt::COMPETITION_DESC);
            if (menu_.count > 2)
                ui::menuButton(menu_, 2, W / 2 + slide, 360, 540, 84, txt::SWING_TEST, txt::SWING_TEST_DESC);
            break;
        }
        case STEP_PLAYERS: {
            ui::header(txt::COMPETITION);
            font::plain(txt::HOW_MANY_PLAYERS, W / 2, 130, 36, UI_TEXT, ALIGN_CENTER);
            for (int i = 0; i < menu_.count; i++) {
                snprintf(buf, sizeof(buf), "%d", i + 1);
                float x = W / 2 + (i - (menu_.count - 1) * 0.5f) * 150 + slide;
                ui::menuButton(menu_, i, x, 270, 120, 120, buf);
                for (int k = 0; k <= i; k++) ui::colorDot(x - i * 9 + k * 18, 352, 6, FAV_COLORS[(k * 5) % NUM_COLORS]);
            }
            break;
        }
        case STEP_ROUNDS: {
            ui::header(txt::COMPETITION);
            font::plain(sport(cfg_.sport).roundsQuestion, W / 2, 130, 36, UI_TEXT, ALIGN_CENTER);
            const int* values = sport(cfg_.sport).roundValues;
            for (int i = 0; i < menu_.count; i++) {
                snprintf(buf, sizeof(buf), "%d", values ? values[i] : i + 1);
                ui::menuButton(menu_, i, W / 2 + (i - (menu_.count - 1) * 0.5f) * 180 + slide, 270, 140, 120, buf);
            }
            break;
        }
        case STEP_LEVEL: {
            ui::header(txt::COMPETITION);
            font::plain(txt::CPU_LEVEL, W / 2, 130, 36, UI_TEXT, ALIGN_CENTER);
            for (int i = 0; i < 3; i++)
                ui::menuButton(menu_, i, W / 2 + (i - 1) * 200 + slide, 270, 180, 100, txt::LEVELS[i]);
            break;
        }
        case STEP_CHARS: {
            const Profile& pr = save::data().profiles[cfg_.profile[charIdx_]];
            spin_ = approach(spin_, 0, 4, 1.0f / 60.0f);
            drawCharacterPreview(Appearance::from(pr), 0, sinf(t_ * 0.8f) * 0.4f + spin_ * 6.28f, t_, 0);
            gfx::begin2D();
            ui::header(sport(cfg_.sport).chooseChar ? sport(cfg_.sport).chooseChar : txt::CHOOSE_CHAR);
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
            // Qué mando es de este jugador y si está listo (Wii Remote conectado; en la pala, con Nunchuk)
            if (!cfg_.bot[charIdx_]) {
                const input::Pad& pad = input::pad(charIdx_);
                bool needNunchuk = sport(cfg_.sport).nunchuk;
                snprintf(buf, sizeof(buf), txt::REMOTE_N, charIdx_ + 1);
                Color c = pad.connected ? UI_TEXT_LIGHT : UI_RED;
                if (!pad.connected) snprintf(buf, sizeof(buf), txt::CONNECT_REMOTE, charIdx_ + 1);
                else if (needNunchuk && !pad.nunchuk) {
                    snprintf(buf, sizeof(buf), "%s", txt::CONNECT_NUNCHUK);
                    c = UI_RED;
                }
                gfx::roundRect(W / 2 - 150, y + 66, 300, 30, 15, rgba(255, 255, 255, 200));
                font::plain(buf, W / 2, y + 70, 20, c, ALIGN_CENTER);
            }
            if (!cfg_.training && charIdx_ > 0) {
                ui::buttonIcon(40, 410, "-", 14, rgba(0x80, 0x88, 0x94));
                font::plain(txt::CPU, 62, 398, 22, UI_TEXT);
            }
            break;
        }
    }
    drawSteps();
    ui::hints(step_ == STEP_CHARS ? txt::OK : txt::SELECT, txt::BACK);
}

}  // namespace

// Indicador de pasos arriba a la derecha: un punto por paso de este recorrido y el nombre del actual
void SetupScene::drawSteps() {
    Step steps[6];
    int n = 0;
    steps[n++] = STEP_SPORT;
    steps[n++] = STEP_MODE;
    if (!cfg_.training || step_ == STEP_PLAYERS || step_ == STEP_ROUNDS || step_ == STEP_LEVEL) {
        steps[n++] = STEP_PLAYERS;
        steps[n++] = STEP_ROUNDS;
        if (sport(cfg_.sport).botLevels) steps[n++] = STEP_LEVEL;
    }
    steps[n++] = STEP_CHARS;
    int cur = 0;
    for (int i = 0; i < n; i++)
        if (steps[i] == step_) cur = i;
    const float W = gfx::screenW();
    const char* name = txt::STEP_NAMES[step_];
    float tw = font::width(name, 22);
    float pw = n * 22 + tw + 40, px = W - pw - 20, py = 22;
    gfx::roundRectGrad(px, py, pw, 40, 20, UI_GLASS_TOP, UI_GLASS_BOTTOM);
    for (int i = 0; i < n; i++) {
        float cx = px + 22 + i * 22;
        if (i < cur) gfx::circle(cx, py + 20, 6, UI_TEAL, 14);
        else if (i == cur) gfx::circle(cx, py + 20, 8, UI_TEAL_GLOW, 16);
        else gfx::circle(cx, py + 20, 5, rgba(0xB4, 0xC6, 0xD2), 12);
    }
    font::plain(name, px + n * 22 + 22, py + 7, 22, UI_TEAL);
}

Scene* makeSetupScene() { return new SetupScene(); }
