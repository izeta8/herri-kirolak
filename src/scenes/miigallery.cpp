// Galería de Miis (escena de prueba 9): varios Miis de cerca para revisar caras, peinados y cuerpos.
// A: cambiar el zoom · ←/→: mover la cámara · B: girarlos / pararlos
#include "engine/gfx.h"
#include "engine/input.h"
#include "engine/ui.h"
#include "game/character.h"
#include "game/mii/mii.h"
#include "game/palette.h"
#include "scenes/scenes.h"

namespace {

class MiiGallery : public Scene {
public:
    MiiGallery() {
        const u8 colors[N] = {0, 5, 7, 4, 2, 9};
        for (int i = 0; i < N; i++) {
            looks_[i].color = colors[i];
            looks_[i].skin = (u8)(i % 3);
            looks_[i].hair = (u8)((i * 2 + 1) % NUM_HAIRS);
            looks_[i].style = (u8)(i % NUM_HAIR_STYLES);
            looks_[i].txapela = i == 0;
            looks_[i].casual = i >= 3;
            if (i < mii::userCount()) looks_[i].user = (s8)i;  // los Miis de la consola, en su orden
        }
        // El segundo jugador, la primera mujer de la consola: así se ven los dos trajes (hombre y mujer)
        for (int k = 0; k < mii::userCount(); k++)
            if (mii::user(k).female) {
                looks_[1].user = (s8)k;
                break;
            }
    }

    void update(float dt) override {
        t_ += dt;
        unsigned d = input::allDown();
        // Sin tocar nada, el zoom cambia solo cada 4 s (para capturar en Dolphin sin darle el foco)
        if (d) autoZoom_ = false;
        autoT_ += dt;
        if (autoZoom_ && autoT_ > 4.0f) {
            autoT_ = 0;
            zoom_ = (zoom_ + 1) % 3;
        }
        if (d & WPAD_BUTTON_A) zoom_ = (zoom_ + 1) % 3;
        if (d & WPAD_BUTTON_B) spin_ = !spin_;
        if (d & (WPAD_BUTTON_LEFT | WPAD_BUTTON_UP)) pan_ -= 0.6f;
        if (d & (WPAD_BUTTON_RIGHT | WPAD_BUTTON_DOWN)) pan_ += 0.6f;
        if (spin_) yaw_ += dt * 0.8f;
    }

    void draw() override {
        gfx::begin2D();
        ui::background(t_);
        Camera cam;
        const float dist[3] = {4.2f, 2.4f, 1.2f};
        const float h[3] = {0.85f, 1.05f, 1.15f};
        cam.fov = 30;
        cam.eye = Vec3(pan_, h[zoom_] + 0.1f, dist[zoom_]);
        cam.target = Vec3(pan_, h[zoom_], 0);
        gfx::begin3D(cam);
        for (int i = 0; i < N; i++) {
            CharacterModel* m = characterCache(looks_[i]);
            CharacterPose p;
            p.pos = Vec3((i - (N - 1) * 0.5f) * 0.6f, 0, 0);
            p.yaw = yaw_;
            p.blink = fmodf(t_ + i * 0.7f, 3.0f) < 0.12f;
            Xf root = Xf().translate(p.pos).rotateY(p.yaw);
            p.handL = root.apply(Vec3(-0.21f, 0.47f, 0.05f));
            p.handR = root.apply(Vec3(0.21f, 0.47f, 0.05f));
            drawCharacter(*m, p);
        }
    }

private:
    static const int N = 6;
    Appearance looks_[N];
    float t_ = 0, yaw_ = 0, pan_ = 0, autoT_ = 0;
    bool autoZoom_ = true;
    int zoom_ = 0;
    bool spin_ = false;
};

}  // namespace

Scene* makeMiiGalleryScene() { return new MiiGallery(); }
