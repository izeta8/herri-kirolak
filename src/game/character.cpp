#include "game/character.h"
#include "game/palette.h"

static Mesh s_shoe;
static bool s_sharedBuilt = false;

static const float HIP_Y = 0.42f;
static const float NECK_Y = 0.50f;  // sobre la cadera
static const float HEAD_R = 0.21f;
// Hombro respecto a la cadera (espacio del torso)
static const Vec3 SHOULDER(0.17f, 0.40f, 0.0f);

// Pantalón de los Miis de Wii Sports Resort (ropa de calle) y blanco de aizkolari
static const Color PANTS_MII = rgba(0x5C, 0x60, 0x6C);
static const Color PANTS_WHITE = rgba(0xF4, 0xF4, 0xF0);

static void buildShared() {
    if (s_sharedBuilt) return;
    MeshBuilder b;
    b.sphere(Vec3(0, 0.04f, 0.03f), Vec3(0.068f, 0.05f, 0.105f), rgba(0x2A, 0x2A, 0x30), 12, 6);
    b.build(s_shoe);
    s_sharedBuilt = true;
}

// Punto sobre la superficie frontal de la cabeza (para pegar ojos y boca)
static float headFrontZ(float x, float y) {
    float r2 = HEAD_R * HEAD_R - x * x - y * y;
    return r2 > 0 ? sqrtf(r2) * 0.98f : 0.0f;
}

static void buildHead(MeshBuilder& b, const Appearance& a, bool blink) {
    Color skin = SKIN_COLORS[a.skin % NUM_SKINS];
    Color hair = HAIR_COLORS[a.hair % NUM_HAIRS];
    const Vec3 c(0, HEAD_R + 0.01f, 0);
    b.sphere(c, Vec3(HEAD_R, HEAD_R * 1.04f, HEAD_R * 0.98f), skin, 20, 14);
    // Orejas
    b.sphere(c + Vec3(-0.205f, -0.01f, 0), Vec3(0.035f, 0.05f, 0.03f), shade(skin, 0.95f), 8, 5);
    b.sphere(c + Vec3(0.205f, -0.01f, 0), Vec3(0.035f, 0.05f, 0.03f), shade(skin, 0.95f), 8, 5);
    // Ojos de Mii: blanco del ojo, iris oscuro grande y brillo
    const Color eye = rgba(0x22, 0x22, 0x28);
    for (int s = -1; s <= 1; s += 2) {
        float ex = s * 0.074f, ey = 0.03f;
        Vec3 ep = c + Vec3(ex, ey, headFrontZ(ex, ey) - 0.012f);
        if (blink) {
            b.sphere(ep + Vec3(0, -0.004f, 0.006f), Vec3(0.03f, 0.006f, 0.018f), eye, 8, 4);
        } else {
            b.sphere(ep, Vec3(0.031f, 0.042f, 0.018f), rgba(0xFC, 0xFC, 0xFA), 12, 7);
            b.sphere(ep + Vec3(-s * 0.003f, -0.004f, 0.009f), Vec3(0.02f, 0.03f, 0.016f), eye, 10, 6);
            b.sphere(ep + Vec3(-s * 0.001f + 0.006f, 0.01f, 0.022f), Vec3(0.0065f, 0.0065f, 0.005f), UI_WHITE, 6, 3);
        }
        // Ceja
        Xf saved = b.xf;
        b.xf = saved * Xf().translate(c + Vec3(s * 0.078f, 0.1f, headFrontZ(s * 0.078f, 0.1f) - 0.004f))
                           .rotateZ(s * -0.14f)
                           .rotateX(-0.3f);
        b.box(Vec3(0, 0, 0), Vec3(0.036f, 0.009f, 0.012f), hair);
        b.xf = saved;
    }
    // Nariz
    b.sphere(c + Vec3(0, -0.012f, headFrontZ(0, -0.012f)), Vec3(0.026f, 0.024f, 0.02f), shade(skin, 0.92f), 8, 5);
    // Boca sonriente: arco de bolitas con las comisuras hacia arriba
    const Color lip = rgba(0xB4, 0x34, 0x3C);
    for (int k = -3; k <= 3; k++) {
        float t = k * 0.22f;
        float mx = sinf(t) * 0.052f, my = -0.086f + (1 - cosf(t)) * 0.07f;
        b.sphere(c + Vec3(mx, my, headFrontZ(mx, my) - 0.002f), Vec3(0.011f, 0.0075f, 0.008f), lip, 6, 4);
    }
    // Mejillas sonrosadas
    b.sphere(c + Vec3(-0.125f, -0.05f, headFrontZ(0.125f, -0.05f) - 0.008f), Vec3(0.03f, 0.018f, 0.01f),
             mixColor(skin, rgba(0xF0, 0x80, 0x80), 0.3f), 8, 4);
    b.sphere(c + Vec3(0.125f, -0.05f, headFrontZ(0.125f, -0.05f) - 0.008f), Vec3(0.03f, 0.018f, 0.01f),
             mixColor(skin, rgba(0xF0, 0x80, 0x80), 0.3f), 8, 4);

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
    Color skin = SKIN_COLORS[a.skin % NUM_SKINS];
    Color pantsCol = a.casual ? PANTS_MII : PANTS_WHITE;
    b.sphere(Vec3(0, 0.26f, 0), Vec3(0.195f, 0.27f, 0.145f), shirt, 18, 10);
    if (a.casual) {
        // Cintura del pantalón de Mii
        b.cylinder(Vec3(0, 0.02f, 0), 0.176f, 0.17f, 0.07f, pantsCol, pantsCol, 16, true);
    } else {
        Color sash = (a.color == 0 || a.color == 7) ? rgba(0x22, 0x22, 0x2A) : rgba(0xC8, 0x1E, 0x1E);
        b.cylinder(Vec3(0, 0.03f, 0), 0.172f, 0.165f, 0.085f, sash, sash, 16, true);
    }
    b.build(torso);
    b.clear();

    b.cylinder(Vec3(0, 0, 0), 0.058f, 0.064f, 1.0f, pantsCol, pantsCol, 10, false);
    b.build(legUnit);
    b.clear();
    b.sphere(Vec3(0, 0.0f, 0), Vec3(0.17f, 0.09f, 0.13f), pantsCol, 14, 6);
    b.build(pants);
    b.clear();

    // Brazos: manga corta del color de la camiseta y antebrazo de piel (cilindros unitarios a lo largo de +Y)
    b.cylinder(Vec3(0, 0, 0), 0.056f, 0.05f, 1.0f, shirt, shirt, 10, true);
    b.build(sleeve);
    b.clear();
    b.cylinder(Vec3(0, 0, 0), 0.038f, 0.036f, 1.0f, skin, skin, 10, false);
    b.build(forearm);
    b.clear();
    b.sphere(Vec3(0, 0, 0), Vec3(0.062f, 0.062f, 0.062f), shirt, 10, 6);
    b.build(shoulder);
    b.clear();

    b.sphere(Vec3(0, 0, 0), Vec3(0.066f, 0.066f, 0.066f), skin, 12, 8);
    b.build(hand);
    built = true;
}

void CharacterModel::release() {
    head.release();
    headBlink.release();
    torso.release();
    legUnit.release();
    pants.release();
    hand.release();
    sleeve.release();
    forearm.release();
    shoulder.release();
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

// Dibuja un cilindro unitario (+Y) estirado del punto a al b
static void drawLimb(const Mesh& m, const Vec3& a, const Vec3& b) {
    Vec3 d = b - a;
    float len = length(d);
    if (len < 0.001f) return;
    Vec3 y = d * (1.0f / len);
    Vec3 ref = fabsf(y.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 x = normalize(cross(y, ref));
    Vec3 z = cross(x, y);
    gfx::drawMesh(m, Xf::fromBasis(a, x, y * len, z));
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
        gfx::drawMesh(m.legUnit, root * Xf().translate(foot + Vec3(0, 0.04f, 0)).rotateZ(-atan2f(d.x, d.y)).scale(Vec3(1, len - 0.04f, 1)));
        gfx::drawMesh(s_shoe, root * Xf().translate(foot));
    }
    Xf torso = characterTorsoXf(p);
    gfx::drawMesh(m.pants, root * Xf().translate(Vec3(0, hipY, 0)));
    gfx::drawMesh(m.torso, torso);
    Xf head = torso * Xf().translate(Vec3(0, NECK_Y, 0)).rotateX(p.headPitch);
    gfx::drawMesh(p.blink ? m.headBlink : m.head, head);
    if (p.handsVisible) {
        // Brazos de hombro a mano (los Miis de Resort tienen brazos)
        for (int s = -1; s <= 1; s += 2) {
            Vec3 sh = torso.apply(Vec3(s * SHOULDER.x, SHOULDER.y, SHOULDER.z));
            Vec3 hand = s < 0 ? p.handL : p.handR;
            Vec3 d = hand - sh;
            gfx::drawMesh(m.shoulder, Xf().translate(sh));
            drawLimb(m.forearm, sh, hand);
            drawLimb(m.sleeve, sh, sh + d * fminf(0.42f, 0.16f / fmaxf(length(d), 0.01f)));
        }
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
