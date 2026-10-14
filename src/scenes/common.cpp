#include "scenes/common.h"
#include <string.h>
#include "engine/gfx.h"
#include "engine/save.h"
#include "game/mii/mii.h"
#include "game/palette.h"
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

void setProfileMii(Profile& p, int index) {
    const MiiInfo& m = mii::user(index);
    p.mii = (u8)(index + 1);
    // Nombre del Mii recortado sin partir una letra UTF-8
    int n = 0;
    while (m.name[n] && n < NAME_LEN) {
        int len = ((unsigned char)m.name[n] & 0xE0) == 0xC0 ? 2 : 1;
        if (n + len > NAME_LEN) break;
        n += len;
    }
    memcpy(p.name, m.name, n);
    p.name[n] = 0;
    if (!p.name[0]) strcpy(p.name, "Mii");
    // Color y piel parecidos para los puntos de color de los menús
    static const u8 FACE_TO_SKIN[6] = {0, 1, 2, 1, 3, 3};
    p.color = m.favColor % NUM_COLORS;
    p.skin = FACE_TO_SKIN[m.faceColor % 6];
}

void linkProfilesToMiis() {
    int count = mii::userCount();
    if (count == 0) return;
    bool taken[100] = {false};
    SaveData& d = save::data();
    for (int i = 0; i < MAX_PROFILES; i++)
        if (d.profiles[i].used && d.profiles[i].mii && d.profiles[i].mii <= count) taken[d.profiles[i].mii - 1] = true;
    bool changed = false;
    int next = 0;
    for (int i = 0; i < MAX_PROFILES; i++) {
        Profile& p = d.profiles[i];
        if (!p.used) continue;
        if (p.mii && p.mii <= count) {
            setProfileMii(p, p.mii - 1);  // el nombre o el color pueden haber cambiado en el Canal Mii
            continue;
        }
        while (next < count && taken[next]) next++;
        setProfileMii(p, next < count ? next : i % count);
        if (next < count) taken[next] = true;
        changed = true;
    }
    if (changed) save::write();
}

void drawMenuBackdrop(float t) {
    // Fondo de los menús: el probaleku visto desde el norte, con la cámara balanceándose despacio
    World& w = sharedWorld();
    gfx::begin2D();
    w.drawSky();
    Camera cam;
    float s = sinf(t * 0.05f);
    cam.eye = Vec3(s * 5.0f, 3.0f, 13.0f);
    cam.target = Vec3(s * 2.0f, 3.2f, -12.0f);
    gfx::begin3D(cam);
    w.setCameraHide(cam.eye, false);
    w.draw();
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
    p.handL = root.apply(Vec3(-0.21f, 0.47f + sway, 0.05f));
    if (wave > 0) {
        float w = sinf(time * 10) * 0.08f;
        p.handR = root.apply(Vec3(0.3f + w, 1.15f, 0.1f));
    } else {
        p.handR = root.apply(Vec3(0.21f, 0.47f - sway, 0.05f));
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
