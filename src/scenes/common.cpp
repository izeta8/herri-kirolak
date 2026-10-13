#include "scenes/common.h"
#include "engine/gfx.h"
#include "game/world.h"
#include "scenes/scenes.h"

World& sharedWorld() {
    static World w;
    static bool built = false;
    if (!built) {
        w.build();
        built = true;
    }
    return w;
}

void drawCharacterPreview(const Appearance& look, float offsetX, float yaw, float time, float wave) {
    Camera cam;
    cam.eye = Vec3(-offsetX, 0.95f, 3.4f);
    cam.target = Vec3(-offsetX, 0.72f, 0);
    cam.fov = 40;
    gfx::begin3D(cam);
    CharacterModel* m = characterCache(look);
    CharacterPose p;
    p.pos = Vec3(0, 0, 0);
    p.yaw = yaw;
    p.lean = sinf(time * 1.6f) * 0.03f;
    p.blink = fmodf(time, 3.5f) < 0.12f;
    p.headPitch = -0.05f;
    Xf root = Xf().translate(p.pos).rotateY(yaw);
    float sway = sinf(time * 1.6f) * 0.02f;
    p.handL = root.apply(Vec3(-0.31f, 0.5f + sway, 0.05f));
    if (wave > 0) {
        float w = sinf(time * 10) * 0.08f;
        p.handR = root.apply(Vec3(0.3f + w, 1.15f, 0.1f));
    } else {
        p.handR = root.apply(Vec3(0.31f, 0.5f - sway, 0.05f));
    }
    // Pequeña peana circular
    static Mesh base;
    if (!base.valid()) {
        MeshBuilder b;
        b.cylinder(Vec3(0, -0.06f, 0), 0.5f, 0.5f, 0.06f, rgba(0xFF, 0xFF, 0xFF), rgba(0xE8, 0xF0, 0xF8), 32, true);
        b.build(base);
    }
    gfx::drawMesh(base, Xf());
    drawCharacter(*m, p);
}
