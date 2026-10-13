// Kolpe proba: muestra en vivo lo que detecta el Wii Remote (para ajustar tuning.h con mandos reales)
#include <stdio.h>
#include "engine/audio.h"
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/tuning.h"
#include "engine/ui.h"
#include "game/palette.h"
#include "game/text.h"
#include "scenes/scenes.h"

namespace {

class SwingTestScene : public Scene {
public:
    void update(float dt) override {
        t_ += dt;
        const input::Pad& p = input::pad(pad_);
        for (int i = 0; i < input::MAX_PADS; i++)
            if (input::pad(i).down & WPAD_BUTTON_A) pad_ = i;
        float m = length(p.accel);
        hist_[histPos_] = m;
        histPos_ = (histPos_ + 1) % HIST;
        peakHold_ = fmaxf(peakHold_ - dt * 1.5f, m);
        if (p.chop.happened) {
            lastStrength_ = p.chop.strength;
            lastTilt_ = p.chop.tilt;
            chops_++;
            flash_ = 1;
            audio::play(p.chop.strength > 0.5f ? SFX_CHOP : SFX_CHOP_WEAK, 0.6f);
            input::rumble(pad_, 0.08f);
        }
        flash_ = fmaxf(0.0f, flash_ - dt * 3);
        if (input::allDown() & WPAD_BUTTON_B) {
            audio::play(SFX_BACK, 0.7f);
            scenes::go(makeSetupScene());
        }
    }

    void draw() override {
        const float W = gfx::screenW();
        char buf[96];
        gfx::begin2D();
        ui::background(t_);
        ui::header(txt::SWING_TEST);
        const input::Pad& p = input::pad(pad_);

        // Gráfica de |a| con los umbrales
        float gx = 50, gy = 100, gw = W - 100, gh = 170;
        ui::panel(gx - 10, gy - 10, gw + 20, gh + 20);
        const float maxG = 6.0f;
        auto yOf = [&](float g) { return gy + gh - clampv(g / maxG, 0.0f, 1.0f) * gh; };
        gfx::rect(gx, yOf(tuning::CHOP_MIN_G), gw, 2, withAlpha(UI_RED, 0.6f));
        gfx::rect(gx, yOf(tuning::SWING_START_G), gw, 2, withAlpha(UI_GOLD, 0.8f));
        gfx::rect(gx, yOf(1.0f), gw, 1, rgba(0xB0, 0xB8, 0xC0));
        for (int i = 0; i < HIST; i++) {
            float g = hist_[(histPos_ + i) % HIST];
            float x = gx + gw * i / HIST;
            float y = yOf(g);
            gfx::rect(x, y, gw / HIST + 1, gy + gh - y, g >= tuning::CHOP_MIN_G ? UI_RED : UI_BLUE);
        }
        snprintf(buf, sizeof(buf), "|a| = %.2f g   max = %.2f g", length(p.accel), peakHold_);
        font::plain(buf, gx + 8, gy + 4, 20, UI_TEXT);

        // Inclinación en vivo y último golpe
        float by = 300;
        ui::panel(40, by, W / 2 - 60, 120);
        snprintf(buf, sizeof(buf), "Okertzea: %+.2f", p.tiltLive);
        font::plain(buf, 60, by + 12, 24, UI_TEXT);
        float bx = 60, bw = W / 2 - 100;
        gfx::roundRect(bx, by + 60, bw, 22, 11, rgba(0xE0, 0xE8, 0xF0));
        gfx::rect(bx + bw / 2 - 1, by + 56, 2, 30, UI_TEXT_LIGHT);
        float tx = bx + bw / 2 + p.tiltLive * bw / 2;
        gfx::circle(tx, by + 71, 12, UI_BLUE, 20);
        snprintf(buf, sizeof(buf), "x %.2f  y %.2f  z %.2f", p.accel.x, p.accel.y, p.accel.z);
        font::plain(buf, 60, by + 88, 18, UI_TEXT_LIGHT);

        float cx = W / 2 + 20;
        ui::panel(cx, by, W / 2 - 60, 120, mixColor(UI_WHITE, rgba(0xFF, 0xF0, 0xB8), flash_));
        snprintf(buf, sizeof(buf), "Kolpeak: %d", chops_);
        font::plain(buf, cx + 20, by + 12, 24, UI_TEXT);
        if (chops_) {
            const char* side = lastTilt_ < -tuning::TILT_STRAIGHT ? txt::LEFT : lastTilt_ > tuning::TILT_STRAIGHT ? txt::RIGHT : "Zuzen";
            snprintf(buf, sizeof(buf), "Indarra: %d%%   %s (%+.2f)", (int)(lastStrength_ * 100), side, lastTilt_);
            font::plain(buf, cx + 20, by + 52, 22, UI_TEXT);
        }
        snprintf(buf, sizeof(buf), "Wii Remote %d", pad_ + 1);
        font::plain(buf, cx + 20, by + 86, 18, UI_TEXT_LIGHT);
        ui::hints(nullptr, txt::BACK);
    }

private:
    static const int HIST = 180;
    float hist_[HIST] = {};
    int histPos_ = 0;
    int pad_ = 0;
    int chops_ = 0;
    float t_ = 0, peakHold_ = 0, lastStrength_ = 0, lastTilt_ = 0, flash_ = 0;
};

}  // namespace

Scene* makeSwingTestScene() { return new SwingTestScene(); }
