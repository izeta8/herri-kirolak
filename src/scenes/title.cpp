// Pantalla de título: de fondo, por turnos, un aizkolari de la CPU cortando troncos y un partido de pala entre dos CPUs.
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/save.h"
#include "engine/tuning.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "game/world.h"
#include "games/aizkolaritza/aizkolari.h"
#include "games/aizkolaritza/log.h"
#include "scenes/scenes.h"

namespace {

const float DEMO_TIME = 22.0f;  // s de cada demo de fondo

class TitleScene : public Scene {
public:
    ~TitleScene() override { delete pala_; }
    TitleScene() {
        MatchConfig cfg;
        cfg.sport = 1;
        cfg.training = false;
        cfg.numPlayers = 2;
        cfg.bot[0] = cfg.bot[1] = true;
        cfg.profile[0] = 0;
        cfg.profile[1] = 1;
        cfg.demo = true;
        pala_ = makePalaScene(cfg);
        chips_.init();
        log_.reset();
        aizk_.setup(Appearance::from(save::data().profiles[0]), &log_);
        aizk_.setReady();
    }

    bool palaTurn() const { return ((int)(t_ / DEMO_TIME) & 1) == 1; }

    void update(float dt) override {
        t_ += dt;
        if (t_ > 0.5f && (input::allDown() & (WPAD_BUTTON_A | WPAD_BUTTON_PLUS))) {
            audio::play(SFX_OK);
            scenes::go(makeMainMenu());
        }
        if (palaTurn()) {
            pala_->update(dt);
            return;
        }
        sharedWorld().update(dt);
        // Demo: la CPU corta en V alternando lados
        if (log_.splitting()) {
            if (log_.splitTime() > 2.5f) {
                log_.reset();
                chips_.clear();
                aizk_.reset();
                aizk_.setReady();
                last_ = 0;
            }
        } else if (aizk_.phase() == Aizkolari::READY) {
            botT_ -= dt;
            if (botT_ <= 0) {
                if (aizk_.side() == 0 && log_.depth[0] >= tuning::LOG_RADIUS * 0.95f) {
                    aizk_.turnAround();
                } else {
                    last_ = last_ <= 0 ? 1 : -1;
                    aizk_.chop(0.9f, (float)last_);
                }
                botT_ = frand(0.35f, 0.6f);
            }
        }
        aizk_.update(dt);
        if (aizk_.impact()) {
            int side = aizk_.side();
            float gain = tuning::LOG_RADIUS * 0.11f;
            if (side == 0) gain = fminf(gain, tuning::LOG_RADIUS * 1.0f - log_.depth[0]);
            else gain = fminf(gain, 2 * tuning::LOG_RADIUS - log_.total());
            log_.addDepth(side, fmaxf(gain, 0.0f));
            chips_.spawn(log_.impactPoint(side, 0), Vec3(0, 0, side == 0 ? 1.0f : -1.0f), 6, 0.9f);
            audio::play(SFX_CHOP, 0.35f, frand(0.92f, 1.08f));
            if (log_.cut()) {
                log_.startSplit();
                aizk_.celebrate();
                audio::play(SFX_SPLIT, 0.5f);
                sharedWorld().cheer(1.5f);
            }
        }
        log_.update(dt);
        chips_.update(dt);
    }

    void draw() override {
        if (palaTurn()) pala_->draw();
        else drawAizko();
        drawTitle();
    }

    void drawAizko() {
        World& world = sharedWorld();
        gfx::begin2D();
        world.drawSky();
        Camera cam;
        float a = t_ * 0.12f + 0.4f;
        cam.target = log_.center() + Vec3(0, 1.3f, 0);
        // Casi horizontal y algo lejos para que se vean las casas, las gradas y la iglesia alrededor
        cam.eye = cam.target + Vec3(sinf(a) * 6.0f, 0.9f + sinf(t_ * 0.3f) * 0.2f, cosf(a) * 6.0f);
        gfx::begin3D(cam);
        world.setCameraHide(cam.eye, false);
        world.draw();
        world.drawShadow(log_.center(), tuning::LOG_LENGTH * 0.55f, tuning::LOG_RADIUS * 1.6f);
        log_.draw();
        aizk_.draw();
        chips_.draw();
    }

    void drawTitle() {
        gfx::begin2D();
        // Fundido en blanco al cambiar de demo
        float ph = fmodf(t_, DEMO_TIME);
        float flash = t_ > DEMO_TIME * 0.5f ? fmaxf(1.0f - ph / 0.5f, 1.0f - (DEMO_TIME - ph) / 0.5f) : 0;
        if (flash > 0) ui::fade(clampv(flash, 0.0f, 1.0f), UI_WHITE);
        const float W = gfx::screenW();
        float in = easeOutBack(clampv((t_ - 0.2f) / 0.6f, 0.0f, 1.0f));
        float y = 70 + (1 - in) * -120;
        // Logo a lo Wii Sports Resort: "Wii" gris encima y el nombre en azul con borde blanco
        float titleW = font::width(txt::GAME_TITLE, 78);
        font::draw("Wii", W / 2 - titleW / 2 + 6, y - 34, 36, rgba(0x9C, 0xA2, 0xAA), UI_WHITE, ALIGN_LEFT);
        font::draw(txt::GAME_TITLE, W / 2 + 3, y + 6, 78, rgba(0x08, 0x3C, 0x78, 110), rgba(0x08, 0x3C, 0x78, 110),
                   ALIGN_CENTER);
        font::draw(txt::GAME_TITLE, W / 2, y, 78, rgba(0x18, 0x8C, 0xDC), UI_WHITE, ALIGN_CENTER);
        float blink = 0.55f + 0.45f * sinf(t_ * 4);
        if (t_ > 0.8f) {
            float tw = font::width(txt::PRESS, 30) + font::width(txt::PRESS_A_TAIL, 30) + 48;
            float x = W / 2 - tw / 2;
            font::draw(txt::PRESS, x, 384, 30, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_LEFT, blink);
            x += font::width(txt::PRESS, 30) + 24;
            ui::buttonIcon(x, 404, "A", 17);
            font::draw(txt::PRESS_A_TAIL, x + 24, 384, 30, UI_WHITE, rgba(0x20, 0x30, 0x40), ALIGN_LEFT, blink);
        }
    }

private:
    Scene* pala_ = nullptr;
    float t_ = 0, botT_ = 1.0f;
    int last_ = 0;
    Log log_;
    Chips chips_;
    Aizkolari aizk_;
};

}  // namespace

Scene* makeTitleScene() { return new TitleScene(); }
