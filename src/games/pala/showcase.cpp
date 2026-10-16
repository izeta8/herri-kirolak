// Prueba (START_SCENE 16): un pelotari de cerca repitiendo sus animaciones, para revisarlas con capturas.
// Ciclo: espera, carrera a la derecha y a la izquierda, plantado, derecha, revés, golpe fuerte, celebración, lamento.
#include <stdio.h>
#include "engine/font.h"
#include "engine/gfx.h"
#include "engine/save.h"
#include "game/palette.h"
#include "game/world.h"
#include "games/pala/fronton.h"
#include "games/pala/pelotari.h"
#include "scenes/scenes.h"

namespace {

const char* PHASES[] = {"itxaron", "korrika >", "korrika <", "zuzentzen", "eskuinez", "ezkerrez", "gogor", "poza", "tristura"};
const int NUM_PHASES = 9;
const float PHASE_T = 1.6f;

class ShowcaseScene : public Scene {
public:
    ShowcaseScene() {
        fronton_.build();
        pel_.setup(Appearance::from(save::data().profiles[0]));
        pel_.pos = Vec3(0, 0, 12);
    }
    void update(float dt) override {
        t_ += dt;
        int ph = (int)(t_ / PHASE_T) % NUM_PHASES;
        float k = fmodf(t_, PHASE_T);
        if (ph != last_) {
            last_ = ph;
            pel_.setMood(ph == 7 ? Pelotari::MOOD_HAPPY : ph == 8 ? Pelotari::MOOD_SAD : Pelotari::MOOD_NONE);
            if (ph == 4) pel_.swing(1, 1.0f, 0.12f, 0.6f);
            if (ph == 5) pel_.swing(-1, 0.9f, 0.12f, 0.6f);
            if (ph == 6) pel_.swing(1, 1.2f, 0.08f, 1.0f);
        }
        Vec3 v;
        if (ph == 1) v = Vec3(4.5f, 0, 0);
        if (ph == 2) v = Vec3(-4.5f, 0, 0);
        pel_.vel = approachv(pel_.vel, v, 14, dt);
        pel_.pos += pel_.vel * dt;
        pel_.setPlanted(ph == 3);
        pel_.prepare(ph == 5 ? -1 : 1, 1.0f, ph == 3 || (ph >= 4 && ph <= 6 && k < 0.05f) ? 1.0f : 0.0f);
        pel_.update(dt);
        fronton_.update(dt);
    }
    void draw() override {
        gfx::begin2D();
        sharedWorld().drawSky();
        Camera cam;
        cam.eye = pel_.pos + Vec3(1.6f, 1.3f, 2.6f);
        cam.target = pel_.pos + Vec3(0, 0.7f, 0);
        cam.fov = 40;
        gfx::begin3D(cam);
        fronton_.draw();
        sharedWorld().drawShadow(pel_.pos, 0.36f, 0.36f);
        pel_.draw();
        gfx::begin2D();
        int ph = (int)(t_ / PHASE_T) % NUM_PHASES;
        font::draw(PHASES[ph], 40, 30, 36, UI_WHITE, UI_TEAL);
    }

private:
    Fronton fronton_;
    Pelotari pel_;
    float t_ = 0;
    int last_ = -1;
};

}  // namespace

Scene* makePelotariShowcase() { return new ShowcaseScene(); }
