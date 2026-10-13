#include "game/world.h"
#include "game/palette.h"
#include "engine/gfx.h"

static void addGround(MeshBuilder& b) {
    // Prado en anillos concéntricos con tonos alternos (efecto "césped cortado" de Wii Sports)
    const int segs = 48;
    const float radii[] = {0, 3.4f, 6, 9, 14, 22, 34, 50, 80};
    const Color greens[] = {rgba(0x6C, 0xC0, 0x4A), rgba(0x64, 0xB6, 0x44)};
    for (int r = 0; r < 8; r++) {
        for (int j = 0; j < segs; j++) {
            float a0 = 2 * PI * j / segs, a1 = 2 * PI * (j + 1) / segs;
            Color c = greens[(r + j / 4) & 1];
            if (r == 0) c = rgba(0xD9, 0xC0, 0x8A);  // serrín de la plaza
            Vec3 p00(cosf(a0) * radii[r], 0, sinf(a0) * radii[r]);
            Vec3 p01(cosf(a1) * radii[r], 0, sinf(a1) * radii[r]);
            Vec3 p10(cosf(a0) * radii[r + 1], 0, sinf(a0) * radii[r + 1]);
            Vec3 p11(cosf(a1) * radii[r + 1], 0, sinf(a1) * radii[r + 1]);
            if (r == 0) b.tri(Vec3(0, 0, 0), p10, p11, c, Vec3(0, 1, 0));
            else b.quad(p00, p10, p11, p01, c, Vec3(0, 1, 0));
        }
    }
    // Borde de la plaza
    for (int j = 0; j < segs; j++) {
        float a0 = 2 * PI * j / segs, a1 = 2 * PI * (j + 1) / segs;
        Vec3 i0(cosf(a0) * 3.4f, 0.02f, sinf(a0) * 3.4f), i1(cosf(a1) * 3.4f, 0.02f, sinf(a1) * 3.4f);
        Vec3 o0(cosf(a0) * 3.6f, 0.02f, sinf(a0) * 3.6f), o1(cosf(a1) * 3.6f, 0.02f, sinf(a1) * 3.6f);
        b.quad(i0, o0, o1, i1, rgba(0xB8, 0xA0, 0x74), Vec3(0, 1, 0));
    }
}

static void addFence(MeshBuilder& b) {
    const float R = 4.4f;
    const int posts = 28;
    const Color wood = rgba(0xA0, 0x74, 0x48), woodDark = rgba(0x86, 0x5E, 0x3A);
    for (int i = 0; i < posts; i++) {
        float a = 2 * PI * i / posts;
        Vec3 p(cosf(a) * R, 0, sinf(a) * R);
        b.cylinder(p, 0.05f, 0.045f, 0.75f, wood, woodDark, 8, true);
        float a1 = 2 * PI * (i + 1) / posts;
        Vec3 q(cosf(a1) * R, 0, sinf(a1) * R);
        Vec3 mid = (p + q) * 0.5f;
        float len = length(q - p);
        float yaw = atan2f(q.x - p.x, q.z - p.z);
        for (float h : {0.35f, 0.62f}) {
            Xf saved = b.xf;
            b.xf = Xf().translate(mid + Vec3(0, h, 0)).rotateY(yaw);
            b.box(Vec3(0, 0, 0), Vec3(0.025f, 0.04f, len * 0.5f), wood);
            b.xf = saved;
        }
    }
}

static void addTree(MeshBuilder& b, const Vec3& p, float s, Color leaves) {
    b.cylinder(p, 0.18f * s, 0.14f * s, 1.2f * s, rgba(0x7A, 0x54, 0x34), rgba(0x7A, 0x54, 0x34), 7, false);
    b.cylinder(p + Vec3(0, 1.0f * s, 0), 1.3f * s, 0.0f, 2.2f * s, leaves, leaves, 9, true);
    b.cylinder(p + Vec3(0, 2.1f * s, 0), 1.0f * s, 0.0f, 1.8f * s, shade(leaves, 1.08f), leaves, 9, true);
}

static void addBaserri(MeshBuilder& b, const Vec3& p, float yaw) {
    Xf saved = b.xf;
    b.xf = Xf().translate(p).rotateY(yaw);
    const Color wall = rgba(0xF2, 0xEE, 0xE2), roof = rgba(0xB0, 0x4A, 0x32), wood = rgba(0x7A, 0x3E, 0x26);
    b.box(Vec3(0, 2.2f, 0), Vec3(4.5f, 2.2f, 3.5f), wall);
    // Tejado a dos aguas
    Vec3 a(-4.9f, 4.4f, -3.9f), bb(4.9f, 4.4f, -3.9f), c(4.9f, 6.6f, 0), d(-4.9f, 6.6f, 0);
    Vec3 e(-4.9f, 4.4f, 3.9f), f(4.9f, 4.4f, 3.9f);
    b.quad(a, bb, c, d, roof, Vec3(0, 1, -1));
    b.quad(e, f, c, d, roof, Vec3(0, 1, 1));
    b.tri(Vec3(-4.5f, 4.4f, -3.5f), Vec3(-4.5f, 4.4f, 3.5f), Vec3(-4.5f, 6.4f, 0), wall, Vec3(-1, 0, 0));
    b.tri(Vec3(4.5f, 4.4f, -3.5f), Vec3(4.5f, 4.4f, 3.5f), Vec3(4.5f, 6.4f, 0), wall, Vec3(1, 0, 0));
    // Puerta en arco, ventanas y balcón de madera en la fachada (+Z)
    b.box(Vec3(0, 1.0f, 3.52f), Vec3(0.9f, 1.0f, 0.04f), wood);
    for (int s = -1; s <= 1; s += 2) {
        b.box(Vec3(s * 2.6f, 1.4f, 3.52f), Vec3(0.45f, 0.5f, 0.04f), rgba(0x4A, 0x6E, 0x9A));
        b.box(Vec3(s * 2.6f, 1.4f, 3.55f), Vec3(0.55f, 0.6f, 0.02f), wood);
    }
    b.box(Vec3(0, 3.0f, 3.8f), Vec3(3.2f, 0.06f, 0.35f), wood);
    b.box(Vec3(0, 3.35f, 4.12f), Vec3(3.2f, 0.32f, 0.03f), wood);
    for (int i = -3; i <= 3; i++) b.box(Vec3(i * 0.9f, 3.65f, 3.55f), Vec3(0.12f, 0.6f, 0.04f), wood);
    b.xf = saved;
}

static void addIkurrina(MeshBuilder& b, const Vec3& p) {
    b.cylinder(p, 0.05f, 0.04f, 4.2f, rgba(0xE8, 0xE8, 0xE8), rgba(0xE8, 0xE8, 0xE8), 8, true);
    // Bandera: fondo rojo, aspa verde y cruz blanca (todo plano, a dos caras)
    const float w = 1.6f, h = 0.95f;
    Vec3 o = p + Vec3(0.05f, 4.1f - h, 0);
    Color red = rgba(0xD5, 0x2B, 0x1E), green = rgba(0x00, 0x96, 0x3E), white = UI_WHITE;
    for (int side = -1; side <= 1; side += 2) {
        float z = side * 0.006f;
        Vec3 n(0, 0, (float)side);
        b.quad(o + Vec3(0, 0, z), o + Vec3(w, 0, z), o + Vec3(w, h, z), o + Vec3(0, h, z), red, n);
        float z2 = side * 0.012f, t = 0.09f;
        // Aspa verde
        b.quad(o + Vec3(0, t, z2), o + Vec3(t, 0, z2), o + Vec3(w, h - t, z2), o + Vec3(w - t, h, z2), green, n);
        b.quad(o + Vec3(0, h - t, z2), o + Vec3(t, h, z2), o + Vec3(w, t, z2), o + Vec3(w - t, 0, z2), green, n);
        float z3 = side * 0.018f, c = 0.055f;
        b.quad(o + Vec3(w / 2 - c, 0, z3), o + Vec3(w / 2 + c, 0, z3), o + Vec3(w / 2 + c, h, z3), o + Vec3(w / 2 - c, h, z3), white, n);
        b.quad(o + Vec3(0, h / 2 - c, z3), o + Vec3(w, h / 2 - c, z3), o + Vec3(w, h / 2 + c, z3), o + Vec3(0, h / 2 + c, z3), white, n);
    }
}

void World::build(unsigned seed) {
    srand(seed);
    MeshBuilder b;
    addGround(b);
    addFence(b);
    // Montes lejanos
    const Color hillCols[] = {rgba(0x58, 0xA8, 0x46), rgba(0x4C, 0x9A, 0x3E), rgba(0x66, 0xB2, 0x4E), rgba(0x5E, 0x96, 0x6A)};
    for (int i = 0; i < 14; i++) {
        float a = 2 * PI * i / 14 + frand(-0.15f, 0.15f);
        float d = frand(55, 75);
        float r = frand(18, 30);
        b.sphere(Vec3(cosf(a) * d, -r * 0.35f, sinf(a) * d), Vec3(r, r * frand(0.55f, 0.85f), r), hillCols[i % 4], 16, 8, -0.1f);
    }
    // Árboles
    for (int i = 0; i < 26; i++) {
        float a = frand(0, 2 * PI), d = frand(11, 32);
        Vec3 p(cosf(a) * d, 0, sinf(a) * d);
        if (p.x < -8 && p.x > -26 && p.z < -14 && p.z > -32) continue;  // no dentro del baserri
        addTree(b, p, frand(0.8f, 1.4f), (i & 1) ? rgba(0x2E, 0x84, 0x3A) : rgba(0x3A, 0x94, 0x40));
    }
    addBaserri(b, Vec3(-17, 0, -23), 0.5f);
    addBaserri(b, Vec3(22, 0, -28), -0.6f);
    addIkurrina(b, Vec3(-3.2f, 0, -5.4f));
    b.build(scenery_);

    b.clear();
    for (int i = 0; i < 12; i++) {
        float a = frand(0, 2 * PI), d = frand(60, 95), y = frand(26, 38);
        Vec3 c(cosf(a) * d, y, sinf(a) * d);
        for (int k = 0; k < 4; k++)
            b.sphere(c + Vec3(frand(-5, 5), frand(-1, 1.5f), frand(-3, 3)), Vec3(frand(4, 7), frand(2, 3), frand(3, 5)), UI_WHITE, 10, 5);
    }
    b.build(clouds_);

    b.clear();
    b.disc(Vec3(0, 0, 0), 1.0f, rgba(0x20, 0x30, 0x10, 0x60), 20);
    b.build(shadow_);

    // Público alrededor de la valla
    numSpectators_ = 0;
    for (int i = 0; i < 28; i++) {
        float a = 2 * PI * i / 28 + frand(-0.05f, 0.05f);
        float d = frand(5.0f, 5.9f);
        Spectator& s = spectators_[numSpectators_++];
        s.pos = Vec3(cosf(a) * d, 0, sinf(a) * d);
        s.yaw = atan2f(-s.pos.x, -s.pos.z);
        s.phase = frand(0, 6.28f);
        s.jump = 0;
        s.jumpV = 0;
        // 10 aspectos distintos repartidos (para no llenar la caché de modelos)
        int k = (i * 7) % 10;
        s.look.color = (u8)((k * 5 + 1) % NUM_COLORS);
        s.look.skin = (u8)(k % 3);
        s.look.hair = (u8)((k * 3) % NUM_HAIRS);
        s.look.txapela = (k % 3) == 0;
        s.model = nullptr;
    }
}

void World::cheer(float seconds) { cheerT_ = fmaxf(cheerT_, seconds); }
void World::clapLight(float seconds) { clapT_ = fmaxf(clapT_, seconds); }

void World::update(float dt) {
    time_ += dt;
    cheerT_ = fmaxf(0.0f, cheerT_ - dt);
    clapT_ = fmaxf(0.0f, clapT_ - dt);
    for (int i = 0; i < numSpectators_; i++) {
        Spectator& s = spectators_[i];
        if (cheerT_ > 0 && s.jump <= 0 && frand() < dt * 2.5f) s.jumpV = frand(1.6f, 2.4f);
        s.jumpV -= 9.0f * dt;
        s.jump = fmaxf(0.0f, s.jump + s.jumpV * dt);
        if (s.jump <= 0 && s.jumpV < 0) s.jumpV = 0;
    }
}

void World::drawSky() {
    float w = gfx::screenW();
    gfx::rectGrad(0, 0, w, 300, rgba(0x48, 0x9E, 0xEA), HORIZON);
    gfx::rect(0, 300, w, 180, HORIZON);
}

void World::drawShadow(const Vec3& pos, float rx, float rz, float alpha) {
    gfx::setLighting(false);
    gfx::setBlend(true);
    gfx::setDepth(true, false);
    (void)alpha;
    gfx::drawMesh(shadow_, Xf().translate(Vec3(pos.x, 0.012f, pos.z)).scale(Vec3(rx, 1, rz)));
    gfx::setDepth(true, true);
    gfx::setBlend(false);
    gfx::setLighting(true);
}

void World::draw() {
    gfx::setFog(true, 35.0f, 140.0f, HORIZON);
    gfx::drawMesh(scenery_, Xf());
    gfx::drawMesh(clouds_, Xf().rotateY(time_ * 0.004f));
    gfx::setFog(false);

    for (int i = 0; i < numSpectators_; i++) {
        Spectator& s = spectators_[i];
        if (!s.model) s.model = characterCache(s.look);
        drawShadow(s.pos, 0.3f, 0.3f);
    }
    for (int i = 0; i < numSpectators_; i++) {
        Spectator& s = spectators_[i];
        CharacterPose p;
        p.pos = s.pos + Vec3(0, s.jump, 0);
        p.yaw = s.yaw;
        p.lean = sinf(time_ * 1.3f + s.phase) * 0.04f;
        p.blink = fmodf(time_ + s.phase, 4.0f) < 0.12f;
        Xf root = Xf().translate(p.pos).rotateY(p.yaw);
        bool clapping = clapT_ > 0 || cheerT_ > 0;
        if (cheerT_ > 0 && s.jump > 0.02f) {
            p.handL = root.apply(Vec3(-0.24f, 1.35f, 0.05f));
            p.handR = root.apply(Vec3(0.24f, 1.35f, 0.05f));
        } else if (clapping) {
            float c = fabsf(sinf(time_ * 14.0f + s.phase)) * 0.1f;
            p.handL = root.apply(Vec3(-0.035f - c, 0.78f, 0.24f));
            p.handR = root.apply(Vec3(0.035f + c, 0.78f, 0.24f));
        } else {
            float sway = sinf(time_ * 1.3f + s.phase) * 0.02f;
            p.handL = root.apply(Vec3(-0.25f, 0.5f + sway, 0.04f));
            p.handR = root.apply(Vec3(0.25f, 0.5f - sway, 0.04f));
        }
        drawCharacter(*s.model, p);
    }
}
