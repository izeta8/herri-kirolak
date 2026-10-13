#include "game/world.h"
#include "game/palette.h"
#include "engine/gfx.h"

// Escenario al estilo de la isla de Wii Sports Resort: la plaza en un prado rodeado de playa y mar turquesa,
// colinas detrás y un volcán a lo lejos.

static const float GRASS_R = 31.0f;  // fin del prado
static const float BEACH_R = 37.0f;  // fin de la playa (empieza el agua)
static const float SEA_Y = -0.25f;

// Anillo horizontal con degradado de color del borde interior al exterior
static void addRing(MeshBuilder& b, float r0, float r1, float y0, float y1, Color c0, Color c1, int segs = 64) {
    const Vec3 up(0, 1, 0);
    for (int j = 0; j < segs; j++) {
        float a0 = 2 * PI * j / segs, a1 = 2 * PI * (j + 1) / segs;
        Vec3 p00(cosf(a0) * r0, y0, sinf(a0) * r0), p01(cosf(a1) * r0, y0, sinf(a1) * r0);
        Vec3 p10(cosf(a0) * r1, y1, sinf(a0) * r1), p11(cosf(a1) * r1, y1, sinf(a1) * r1);
        b.triSmooth({p00, up, c0}, {p11, up, c1}, {p10, up, c1});
        b.triSmooth({p00, up, c0}, {p01, up, c0}, {p11, up, c1});
    }
}

static void addGround(MeshBuilder& b) {
    // Prado en anillos concéntricos con tonos alternos (efecto "césped cortado" de Wii Sports)
    const int segs = 48;
    const float radii[] = {0, 3.4f, 6, 9, 13, 18, 24, GRASS_R};
    const Color greens[] = {rgba(0x74, 0xC8, 0x44), rgba(0x68, 0xBC, 0x3E)};
    for (int r = 0; r < 7; r++) {
        for (int j = 0; j < segs; j++) {
            float a0 = 2 * PI * j / segs, a1 = 2 * PI * (j + 1) / segs;
            Color c = greens[(r + j / 4) & 1];
            if (r == 0) c = rgba(0xE2, 0xC9, 0x92);  // serrín de la plaza
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
        b.quad(i0, o0, o1, i1, rgba(0xC4, 0xAC, 0x7C), Vec3(0, 1, 0));
    }
    // Playa: arena clara que baja hasta el agua, con la orilla mojada más oscura
    addRing(b, GRASS_R, GRASS_R + 1.2f, 0.0f, -0.02f, rgba(0x9C, 0xC8, 0x5A), rgba(0xF6, 0xEA, 0xC4), 64);
    addRing(b, GRASS_R + 1.2f, BEACH_R - 1.0f, -0.02f, -0.16f, rgba(0xF6, 0xEA, 0xC4), rgba(0xF0, 0xDF, 0xB0));
    addRing(b, BEACH_R - 1.0f, BEACH_R + 0.6f, -0.16f, SEA_Y - 0.1f, rgba(0xE4, 0xCC, 0x94), rgba(0xD8, 0xBC, 0x84));
    // Espuma de la orilla
    addRing(b, BEACH_R - 0.2f, BEACH_R + 0.5f, SEA_Y + 0.03f, SEA_Y + 0.03f, rgba(0xF4, 0xFC, 0xFC), rgba(0xC8, 0xF0, 0xF0));
    // Mar: turquesa en la orilla, azul intenso mar adentro
    addRing(b, BEACH_R + 0.5f, BEACH_R + 7, SEA_Y, SEA_Y, rgba(0x6C, 0xE0, 0xE4), rgba(0x30, 0xBC, 0xE0));
    addRing(b, BEACH_R + 7, 70, SEA_Y, SEA_Y, rgba(0x30, 0xBC, 0xE0), rgba(0x1C, 0x90, 0xDC));
    addRing(b, 70, 150, SEA_Y, SEA_Y, rgba(0x1C, 0x90, 0xDC), rgba(0x16, 0x78, 0xD0));
    addRing(b, 150, 330, SEA_Y, SEA_Y, rgba(0x16, 0x78, 0xD0), rgba(0x20, 0x84, 0xD4));
}

static void addFence(MeshBuilder& b) {
    const float R = 4.4f;
    const int posts = 28;
    const Color wood = rgba(0xA8, 0x7A, 0x4C), woodDark = rgba(0x8C, 0x62, 0x3C);
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

// Árbol redondo de Wuhu: tronco y copa de bolas
static void addTree(MeshBuilder& b, const Vec3& p, float s, Color leaves) {
    b.cylinder(p, 0.2f * s, 0.15f * s, 1.6f * s, rgba(0x86, 0x5E, 0x3A), rgba(0x86, 0x5E, 0x3A), 7, false);
    b.sphere(p + Vec3(0, 2.3f * s, 0), Vec3(1.25f * s, 1.1f * s, 1.25f * s), leaves, 12, 8);
    b.sphere(p + Vec3(0.5f * s, 2.9f * s, 0.2f * s), Vec3(0.8f * s, 0.7f * s, 0.8f * s), shade(leaves, 1.1f), 10, 6);
    b.sphere(p + Vec3(-0.45f * s, 2.7f * s, -0.3f * s), Vec3(0.75f * s, 0.65f * s, 0.75f * s), shade(leaves, 0.95f), 10, 6);
}

// Cilindro entre dos puntos usando la transformación del constructor
static void addSegment(MeshBuilder& b, const Vec3& a, const Vec3& c, float r0, float r1, Color col) {
    Vec3 d = c - a;
    float len = length(d);
    Vec3 y = d * (1.0f / len);
    Vec3 ref = fabsf(y.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
    Vec3 x = normalize(cross(y, ref));
    Vec3 z = cross(x, y);
    Xf saved = b.xf;
    b.xf = Xf::fromBasis(a, x, y, z);
    b.cylinder(Vec3(0, 0, 0), r0, r1, len, col, col, 8, false);
    b.xf = saved;
}

// Palmera: tronco curvado a anillos y hojas largas que caen
static void addPalm(MeshBuilder& b, const Vec3& p, float s, float yaw) {
    Vec3 dir(sinf(yaw), 0, cosf(yaw));
    const int segs = 7;
    Vec3 cur = p;
    for (int i = 0; i < segs; i++) {
        float t = (i + 1) / (float)segs;
        Vec3 next = p + Vec3(0, 5.6f * s * t, 0) + dir * (1.6f * s * t * t);
        float r = 0.2f * s * (1 - 0.35f * t);
        addSegment(b, cur, next, r * 1.05f, r * 0.9f, (i & 1) ? rgba(0x9E, 0x7A, 0x50) : rgba(0x8A, 0x68, 0x44));
        cur = next;
    }
    const Vec3 top = cur;
    const Color leaf = rgba(0x3E, 0xA8, 0x3A), leafUnder = rgba(0x2C, 0x86, 0x30);
    for (int k = 0; k < 8; k++) {
        float ang = yaw + k * 2 * PI / 8 + (k & 1) * 0.2f;
        Vec3 out(cosf(ang), 0, sinf(ang)), side(-sinf(ang), 0, cosf(ang));
        float len = (k & 1) ? 2.4f : 2.9f;
        Vec3 prevL, prevR;
        for (int j = 0; j <= 5; j++) {
            float t = j / 5.0f;
            Vec3 c = top + out * (len * s * t) + Vec3(0, (0.55f * t - 1.5f * t * t) * s, 0);
            float w = s * 0.42f * sinf(PI * fminf(t * 1.15f + 0.08f, 1.0f));
            Vec3 l = c - side * w, r = c + side * w;
            if (j > 0) {
                b.quad(prevL, prevR, r, l, leaf, Vec3(0, 1, 0));
                b.quad(prevL, prevR, r, l, leafUnder, Vec3(0, -1, 0));
            }
            prevL = l;
            prevR = r;
        }
    }
    for (int k = 0; k < 3; k++) {
        float a = yaw + k * 2.1f;
        b.sphere(top + Vec3(cosf(a) * 0.18f * s, -0.2f * s, sinf(a) * 0.18f * s), Vec3(0.14f * s, 0.14f * s, 0.14f * s),
                 rgba(0x6A, 0x4A, 0x2A), 8, 5);
    }
}

static void addBaserri(MeshBuilder& b, const Vec3& p, float yaw) {
    Xf saved = b.xf;
    b.xf = Xf().translate(p).rotateY(yaw);
    const Color wall = rgba(0xF6, 0xF2, 0xE8), roof = rgba(0xC8, 0x52, 0x36), wood = rgba(0x7A, 0x3E, 0x26);
    b.box(Vec3(0, 2.2f, 0), Vec3(4.5f, 2.2f, 3.5f), wall);
    // Tejado a dos aguas
    Vec3 a(-4.9f, 4.4f, -3.9f), bb(4.9f, 4.4f, -3.9f), c(4.9f, 6.6f, 0), d(-4.9f, 6.6f, 0);
    Vec3 e(-4.9f, 4.4f, 3.9f), f(4.9f, 4.4f, 3.9f);
    b.quad(a, bb, c, d, roof, Vec3(0, 1, -1));
    b.quad(e, f, c, d, roof, Vec3(0, 1, 1));
    b.tri(Vec3(-4.5f, 4.4f, -3.5f), Vec3(-4.5f, 4.4f, 3.5f), Vec3(-4.5f, 6.4f, 0), wall, Vec3(-1, 0, 0));
    b.tri(Vec3(4.5f, 4.4f, -3.5f), Vec3(4.5f, 4.4f, 3.5f), Vec3(4.5f, 6.4f, 0), wall, Vec3(1, 0, 0));
    // Puerta, ventanas y balcón de madera en la fachada (+Z)
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

// Volcán de la isla: cono rocoso con falda verde y cráter oscuro
static void addVolcano(MeshBuilder& b, const Vec3& p) {
    const Color rock = rgba(0xA8, 0x86, 0x60), rockDark = rgba(0x8A, 0x6C, 0x4C), crater = rgba(0x6A, 0x52, 0x3E);
    b.cylinder(p + Vec3(0, -2, 0), 50, 36, 9, rgba(0x58, 0xA8, 0x44), rgba(0x58, 0xA8, 0x44), 20, false);
    b.cylinder(p + Vec3(0, 5, 0), 38, 7, 50, rock, crater, 18, true);
    b.cylinder(p + Vec3(7, 3, 5), 30, 5, 42, rockDark, rockDark, 14, true);
    b.cylinder(p + Vec3(-9, 2, -3), 26, 4, 36, rock, rockDark, 14, true);
}

static bool nearBaserri(const Vec3& p) {
    const Vec3 houses[] = {Vec3(-19, 0, 7), Vec3(19, 0, 9)};
    for (const Vec3& h : houses)
        if (length(p - h) < 7.5f) return true;
    return false;
}

void World::build(unsigned seed) {
    srand(seed);
    MeshBuilder b;
    addGround(b);
    addFence(b);
    // Colinas verdes detrás de la cámara de la lehiaketa (lado +Z); hacia −Z queda el mar abierto con el volcán
    const Color hillCols[] = {rgba(0x5E, 0xB4, 0x46), rgba(0x52, 0xA6, 0x40), rgba(0x6A, 0xBC, 0x4C), rgba(0x4C, 0x9C, 0x3E)};
    for (int i = 0; i < 9; i++) {
        float a = PI * (0.1f + 0.8f * i / 8.0f) + frand(-0.06f, 0.06f);
        float d = frand(48, 62);
        float r = frand(14, 24);
        b.sphere(Vec3(cosf(a) * d, -r * 0.4f, sinf(a) * d), Vec3(r, r * frand(0.6f, 0.9f), r), hillCols[i % 4], 16, 8, -0.1f);
    }
    addVolcano(b, Vec3(-30, 0, -128));
    // Árboles redondos en el prado
    for (int i = 0; i < 18; i++) {
        float a = frand(0, 2 * PI), d = frand(12, 26);
        Vec3 p(cosf(a) * d, 0, sinf(a) * d);
        if (nearBaserri(p) || (p.z < -6 && fabsf(p.x) < -p.z * 0.9f)) continue;  // no tapar el mar ni el volcán
        addTree(b, p, frand(0.7f, 1.0f), (i & 1) ? rgba(0x3A, 0x9A, 0x3A) : rgba(0x48, 0xAA, 0x40));
    }
    // Palmeras en la playa
    for (int i = 0; i < 22; i++) {
        float a = 2 * PI * i / 22 + frand(-0.1f, 0.1f);
        float d = frand(GRASS_R - 1.5f, BEACH_R - 2.0f);
        Vec3 p(cosf(a) * d, 0, sinf(a) * d);
        if (nearBaserri(p)) continue;
        addPalm(b, p, frand(0.85f, 1.15f), a + PI + frand(-0.6f, 0.6f));
    }
    addBaserri(b, Vec3(-19, 0, 7), 1.92f);
    addBaserri(b, Vec3(19, 0, 9), -2.01f);
    addIkurrina(b, Vec3(-3.2f, 0, -5.4f));
    b.build(scenery_);

    b.clear();
    // Nubes grandes y esponjosas
    for (int i = 0; i < 14; i++) {
        float a = frand(0, 2 * PI), d = frand(70, 120), y = frand(24, 40);
        Vec3 c(cosf(a) * d, y, sinf(a) * d);
        for (int k = 0; k < 5; k++)
            b.sphere(c + Vec3(frand(-7, 7), frand(-1, 2.5f), frand(-4, 4)), Vec3(frand(5, 9), frand(3, 4.5f), frand(4, 6)),
                     UI_WHITE, 10, 6);
    }
    b.build(clouds_);

    b.clear();
    b.disc(Vec3(0, 0, 0), 1.0f, rgba(0x20, 0x30, 0x10, 0x60), 20);
    b.build(shadow_);

    // Público alrededor de la valla: Miis con ropa de calle
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
        s.look.txapela = (k % 4) == 0;
        s.look.casual = 1;
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
    // Cielo de Resort: azul intenso arriba que se aclara hacia el horizonte
    gfx::rectGrad(0, 0, w, 140, rgba(0x14, 0x78, 0xE0), rgba(0x48, 0xA8, 0xF0));
    gfx::rectGrad(0, 140, w, 160, rgba(0x48, 0xA8, 0xF0), HORIZON);
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

bool World::hidden(const Spectator& s) const {
    if (!hideOn_) return false;
    Vec3 c(hideCam_.x, 0, hideCam_.z), p(s.pos.x, 0, s.pos.z);
    return length(c) > 4.5f && dot(normalize(p), normalize(c)) > 0.55f;
}

void World::draw() {
    gfx::setFog(true, 70.0f, 320.0f, HORIZON);
    gfx::drawMesh(scenery_, Xf());
    gfx::drawMesh(clouds_, Xf().rotateY(time_ * 0.004f));
    gfx::setFog(false);

    for (int i = 0; i < numSpectators_; i++) {
        Spectator& s = spectators_[i];
        if (!s.model) s.model = characterCache(s.look);
        if (hidden(s)) continue;
        drawShadow(s.pos, 0.3f, 0.3f);
    }
    for (int i = 0; i < numSpectators_; i++) {
        Spectator& s = spectators_[i];
        if (hidden(s)) continue;
        CharacterPose p;
        p.pos = s.pos + Vec3(0, s.jump, 0);
        p.yaw = s.yaw;
        p.lean = sinf(time_ * 1.3f + s.phase) * 0.04f;
        p.blink = fmodf(time_ + s.phase, 4.0f) < 0.12f;
        Xf root = Xf().translate(p.pos).rotateY(p.yaw);
        bool clapping = clapT_ > 0 || cheerT_ > 0;
        if (cheerT_ > 0 && s.jump > 0.02f) {
            p.handL = root.apply(Vec3(-0.3f, 1.3f, 0.05f));
            p.handR = root.apply(Vec3(0.3f, 1.3f, 0.05f));
        } else if (clapping) {
            float c = fabsf(sinf(time_ * 14.0f + s.phase)) * 0.1f;
            p.handL = root.apply(Vec3(-0.035f - c, 0.78f, 0.24f));
            p.handR = root.apply(Vec3(0.035f + c, 0.78f, 0.24f));
        } else {
            float sway = sinf(time_ * 1.3f + s.phase) * 0.02f;
            p.handL = root.apply(Vec3(-0.31f, 0.5f + sway, 0.04f));
            p.handR = root.apply(Vec3(0.31f, 0.5f - sway, 0.04f));
        }
        drawCharacter(*s.model, p);
    }
}
