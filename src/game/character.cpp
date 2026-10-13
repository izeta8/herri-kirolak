#include "game/character.h"
#include "game/palette.h"

static Mesh s_legUnit, s_shoe, s_pants;
static bool s_sharedBuilt = false;

static const float HIP_Y = 0.42f;
static const float NECK_Y = 0.50f;  // sobre la cadera
static const float HEAD_R = 0.21f;

static void buildShared() {
    if (s_sharedBuilt) return;
    MeshBuilder b;
    b.cylinder(Vec3(0, 0, 0), 0.058f, 0.062f, 1.0f, rgba(0xF4, 0xF4, 0xF0), rgba(0xF4, 0xF4, 0xF0), 10, false);
    b.build(s_legUnit);
    b.clear();
    b.sphere(Vec3(0, 0.04f, 0.03f), Vec3(0.068f, 0.05f, 0.105f), rgba(0x2A, 0x2A, 0x30), 12, 6);
    b.build(s_shoe);
    b.clear();
    // Pantalón blanco en la cadera
    b.sphere(Vec3(0, 0.0f, 0), Vec3(0.17f, 0.09f, 0.13f), rgba(0xF4, 0xF4, 0xF0), 14, 6);
    b.build(s_pants);
    s_sharedBuilt = true;
}

static void buildHead(MeshBuilder& b, const Appearance& a, bool blink) {
    Color skin = SKIN_COLORS[a.skin % NUM_SKINS];
    Color hair = HAIR_COLORS[a.hair % NUM_HAIRS];
    const Vec3 c(0, HEAD_R + 0.01f, 0);
    b.sphere(c, Vec3(HEAD_R, HEAD_R * 1.04f, HEAD_R * 0.98f), skin, 18, 12);
    // Orejas
    b.sphere(c + Vec3(-0.205f, -0.01f, 0), Vec3(0.035f, 0.05f, 0.03f), shade(skin, 0.95f), 8, 5);
    b.sphere(c + Vec3(0.205f, -0.01f, 0), Vec3(0.035f, 0.05f, 0.03f), shade(skin, 0.95f), 8, 5);
    // Ojos
    const Color eye = rgba(0x22, 0x22, 0x28);
    for (int s = -1; s <= 1; s += 2) {
        Vec3 ep = c + Vec3(s * 0.072f, 0.035f, 0.188f);
        if (blink) {
            b.sphere(ep, Vec3(0.026f, 0.006f, 0.018f), eye, 8, 4);
        } else {
            b.sphere(ep, Vec3(0.022f, 0.034f, 0.02f), eye, 10, 6);
            b.sphere(ep + Vec3(0.007f, 0.012f, 0.016f), Vec3(0.007f, 0.007f, 0.006f), UI_WHITE, 6, 3);
        }
        // Ceja
        Xf saved = b.xf;
        b.xf = saved * Xf().translate(c + Vec3(s * 0.075f, 0.098f, 0.175f)).rotateZ(s * -0.12f).rotateX(-0.25f);
        b.box(Vec3(0, 0, 0), Vec3(0.034f, 0.008f, 0.012f), hair);
        b.xf = saved;
    }
    // Nariz y boca
    b.sphere(c + Vec3(0, -0.005f, 0.205f), Vec3(0.027f, 0.027f, 0.022f), shade(skin, 0.93f), 8, 5);
    b.sphere(c + Vec3(0, -0.078f, 0.184f), Vec3(0.048f, 0.014f, 0.016f), rgba(0x9A, 0x2E, 0x2E), 10, 5);
    // Mejillas sonrosadas
    b.sphere(c + Vec3(-0.12f, -0.05f, 0.155f), Vec3(0.03f, 0.018f, 0.01f), mixColor(skin, rgba(0xF0, 0x80, 0x80), 0.35f), 8, 4);
    b.sphere(c + Vec3(0.12f, -0.05f, 0.155f), Vec3(0.03f, 0.018f, 0.01f), mixColor(skin, rgba(0xF0, 0x80, 0x80), 0.35f), 8, 4);

    Xf saved = b.xf;
    if (a.txapela) {
        // Pelo solo por detrás y los lados
        b.xf = saved * Xf().translate(c).rotateX(-1.1f);
        b.sphere(Vec3(0, 0, 0), Vec3(HEAD_R * 1.05f, HEAD_R * 1.05f, HEAD_R * 1.05f), hair, 18, 8, 0.25f);
        // Txapela: boina negra ladeada con su "txertoa"
        const Color beret = rgba(0x1C, 0x1E, 0x26);
        b.xf = saved * Xf().translate(c + Vec3(0.0f, 0.15f, -0.01f)).rotateZ(0.16f).rotateX(-0.08f);
        b.sphere(Vec3(0, 0, 0), Vec3(0.255f, 0.075f, 0.25f), beret, 20, 8);
        b.cylinder(Vec3(0, 0.06f, 0), 0.012f, 0.008f, 0.04f, beret, beret, 6, true);
    } else {
        // Casquete de pelo inclinado hacia atrás para dejar libre la frente
        b.xf = saved * Xf().translate(c).rotateX(-0.42f);
        b.sphere(Vec3(0, 0, 0), Vec3(HEAD_R * 1.07f, HEAD_R * 1.07f, HEAD_R * 1.06f), hair, 18, 9, 0.12f);
        // Flequillo
        b.xf = saved * Xf().translate(c + Vec3(0, 0.13f, 0.11f)).rotateX(0.5f);
        b.sphere(Vec3(0, 0, 0), Vec3(0.17f, 0.05f, 0.09f), hair, 12, 6);
    }
    b.xf = saved;
}

void CharacterModel::build(const Appearance& a) {
    buildShared();
    release();
    look = a;
    MeshBuilder b;
    buildHead(b, a, false);
    b.build(head);
    b.clear();
    buildHead(b, a, true);
    b.build(headBlink);
    b.clear();

    Color shirt = FAV_COLORS[a.color % NUM_COLORS];
    Color sash = (a.color == 0 || a.color == 7) ? rgba(0x22, 0x22, 0x2A) : rgba(0xC8, 0x1E, 0x1E);
    b.sphere(Vec3(0, 0.26f, 0), Vec3(0.195f, 0.27f, 0.145f), shirt, 16, 10);
    b.cylinder(Vec3(0, 0.03f, 0), 0.172f, 0.165f, 0.085f, sash, sash, 16, true);
    b.build(torso);
    b.clear();

    b.sphere(Vec3(0, 0, 0), Vec3(0.066f, 0.066f, 0.066f), SKIN_COLORS[a.skin % NUM_SKINS], 12, 8);
    b.build(hand);
    built = true;
}

void CharacterModel::release() {
    head.release();
    headBlink.release();
    torso.release();
    legs.release();
    hand.release();
    built = false;
}

Xf characterTorsoXf(const CharacterPose& p) {
    float sq = clampv(p.squash, -0.3f, 0.3f);
    return Xf()
        .translate(p.pos)
        .rotateY(p.yaw)
        .translate(Vec3(0, HIP_Y * (1 - sq * 0.5f), 0))
        .rotateX(p.lean)
        .rotateZ(p.side)
        .scale(Vec3(1 + sq * 0.3f, 1 - sq, 1 + sq * 0.3f));
}

void drawCharacter(const CharacterModel& m, const CharacterPose& p) {
    if (!m.built) return;
    Xf root = Xf().translate(p.pos).rotateY(p.yaw);
    float sq = clampv(p.squash, -0.3f, 0.3f);
    float hipY = HIP_Y * (1 - sq * 0.5f);
    float stance = 0.1f;

    // Piernas: de los pies a la cadera
    for (int s = -1; s <= 1; s += 2) {
        Vec3 foot(s * stance, 0, 0), hip(s * 0.085f, hipY, 0);
        Vec3 d = hip - foot;
        float len = length(d);
        gfx::drawMesh(s_legUnit, root * Xf().translate(foot + Vec3(0, 0.04f, 0)).rotateZ(-atan2f(d.x, d.y)).scale(Vec3(1, len - 0.04f, 1)));
        gfx::drawMesh(s_shoe, root * Xf().translate(foot));
    }
    Xf torso = characterTorsoXf(p);
    gfx::drawMesh(s_pants, root * Xf().translate(Vec3(0, hipY, 0)));
    gfx::drawMesh(m.torso, torso);
    Xf head = torso * Xf().translate(Vec3(0, NECK_Y, 0)).rotateX(p.headPitch);
    gfx::drawMesh(p.blink ? m.headBlink : m.head, head);
    if (p.handsVisible) {
        gfx::drawMesh(m.hand, Xf().translate(p.handL));
        gfx::drawMesh(m.hand, Xf().translate(p.handR));
    }
}

// ------------------------------------------------------------------ caché
static const int CACHE_SIZE = 32;
static CharacterModel s_cache[CACHE_SIZE];
static int s_next = 0;

CharacterModel* characterCache(const Appearance& a) {
    for (int i = 0; i < CACHE_SIZE; i++)
        if (s_cache[i].built && s_cache[i].look == a) return &s_cache[i];
    CharacterModel* m = &s_cache[s_next];
    s_next = (s_next + 1) % CACHE_SIZE;
    m->build(a);
    return m;
}
