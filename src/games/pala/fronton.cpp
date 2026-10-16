#include "games/pala/fronton.h"
#include <stdio.h>
#include "engine/font.h"
#include "engine/tuning.h"
#include "game/mii/mii.h"
#include "game/palette.h"

using namespace tuning;

static const Color WALL = rgba(0x52, 0xA2, 0x72);       // frontis y pared izquierda, verdes
static const Color WALL_TOP = rgba(0xE8, 0xE2, 0xD4);   // remate de piedra clara
static const Color FLOOR = rgba(0x98, 0xB0, 0xA4);      // cancha de cemento
static const Color FLOOR_OUT = rgba(0xC4, 0xC2, 0xB8);  // contracancha
static const Color LINE = rgba(0xFA, 0xFA, 0xF6);
static const Color LINE_RED = rgba(0xE0, 0x3C, 0x30);
static const Color CHAPA = rgba(0xC8, 0xD0, 0xD8);
static const Color STONE = rgba(0xD8, 0xCE, 0xB8);
static const Color SEAT = rgba(0x2C, 0x78, 0xC8);

// Cuadrilátero con un color por esquina (para degradados: sombra junto a las paredes)
static void quadGrad(MeshBuilder& b, Vec3 a, Vec3 c1, Vec3 c2, Vec3 d, Color ca, Color cb, Color cc, Color cd, Vec3 n) {
    Vert va{a, n, ca}, vb{c1, n, cb}, vc{c2, n, cc}, vd{d, n, cd};
    b.triSmooth(va, vb, vc);
    b.triSmooth(va, vc, vd);
}

static Color jitter(Color c, unsigned& seed, float amount) {
    seed = seed * 1103515245u + 12345u;
    float k = 1.0f + amount * (((seed >> 16) & 0xFF) / 127.5f - 1.0f);
    return shade(c, k);
}

// Pared de bloques de piedra: cuadros de color algo distinto en un plano (origen, eje u, eje v, normal)
static void blockWall(MeshBuilder& b, Vec3 o, Vec3 u, Vec3 v, float lu, float lv, float bu, float bv, Color c, Vec3 n,
                      unsigned seed) {
    int nu = (int)(lu / bu + 0.5f), nv = (int)(lv / bv + 0.5f);
    float su = lu / nu, sv = lv / nv;
    for (int j = 0; j < nv; j++)
        for (int i = 0; i < nu; i++) {
            float off = (j & 1) ? 0.5f : 0.0f;  // aparejo: hiladas desplazadas media pieza
            float u0 = fmaxf(0.0f, (i - off) * su), u1 = fminf(lu, (i + 1 - off) * su);
            if (i == nu - 1 && off > 0) u1 = lu;
            Color k = jitter(c, seed, 0.05f);
            // un poco más oscuro abajo (suciedad del suelo)
            float low = clampv(j * sv / 1.5f, 0.0f, 1.0f);
            Color k0 = shade(k, 0.86f + 0.14f * low), k1 = shade(k, 0.86f + 0.14f * clampv((j + 1) * sv / 1.5f, 0.0f, 1.0f));
            Vec3 p00 = o + u * u0 + v * (j * sv), p10 = o + u * u1 + v * (j * sv);
            Vec3 p11 = o + u * u1 + v * ((j + 1) * sv), p01 = o + u * u0 + v * ((j + 1) * sv);
            quadGrad(b, p00, p10, p11, p01, k0, k0, k1, k1, n);
        }
}

void Fronton::build() {
    if (mesh_.valid()) return;
    MeshBuilder b;
    const float XL = -COURT_W / 2, XR = COURT_W / 2;
    const float back = COURT_LEN + 4.0f;  // la cancha sigue un poco por detrás de la última raya
    const float out = XR + 5.0f;          // ancho de la contracancha

    // Suelo de la cancha en tiras, más oscuro junto al frontis y la pared izquierda (oclusión suave)
    const Vec3 up(0, 1, 0);
    const int NX = 10, NZ = 17;
    for (int iz = 0; iz < NZ; iz++)
        for (int ix = 0; ix < NX; ix++) {
            float x0 = XL + COURT_W * ix / NX, x1 = XL + COURT_W * (ix + 1) / NX;
            float z0 = back * iz / NZ, z1 = back * (iz + 1) / NZ;
            auto col = [&](float x, float z) {
                float ao = fminf(1.0f, fminf((x - XL) / 1.6f, z / 1.6f));
                return shade(FLOOR, 0.78f + 0.22f * smooth01(ao));
            };
            quadGrad(b, Vec3(x0, 0, z0), Vec3(x0, 0, z1), Vec3(x1, 0, z1), Vec3(x1, 0, z0), col(x0, z0), col(x0, z1),
                     col(x1, z1), col(x1, z0), up);
        }
    b.box(Vec3(0, -0.15f, back / 2), Vec3(COURT_W / 2, 0.14f, back / 2), shade(FLOOR, 0.8f));
    b.box(Vec3((XR + out) / 2, -0.06f, back / 2), Vec3((out - XR) / 2, 0.05f, back / 2), FLOOR_OUT);
    // Líneas del suelo: lado abierto, falta, pasa y las rayas de los cuadros
    const float lw = 0.06f, ly = 0.006f;
    b.box(Vec3(XR, ly, back / 2), Vec3(lw, ly, back / 2), LINE);
    for (float z = CUADRO; z < back; z += CUADRO) b.box(Vec3(XR - 0.3f, ly, z), Vec3(0.3f, ly, lw), LINE);
    b.box(Vec3(0, ly, FALTA_Z), Vec3(COURT_W / 2, ly, lw), LINE_RED);
    b.box(Vec3(0, ly, PASA_Z), Vec3(COURT_W / 2, ly, lw), LINE_RED);

    // Frontis de bloques, con la chapa abajo, la línea de arriba y el remate de piedra
    const float fw = out - XL + 0.8f;
    b.box(Vec3((XL + out) / 2 - 0.4f, WALL_H / 2, -0.45f), Vec3(fw / 2, WALL_H / 2, 0.42f), shade(WALL, 0.9f));
    blockWall(b, Vec3(XL - 0.8f, 0, 0.002f), Vec3(1, 0, 0), Vec3(0, 1, 0), fw, WALL_H, 1.2f, 0.6f, WALL, Vec3(0, 0, 1), 7);
    b.box(Vec3((XL + out) / 2 - 0.4f, WALL_H + 0.18f, -0.4f), Vec3(fw / 2 + 0.1f, 0.18f, 0.5f), WALL_TOP);
    b.box(Vec3(0, CHAPA_H / 2, 0.025f), Vec3(COURT_W / 2, CHAPA_H / 2, 0.025f), CHAPA);
    b.box(Vec3(0, CHAPA_H * 0.5f, 0.052f), Vec3(COURT_W / 2, 0.012f, 0.004f), shade(CHAPA, 0.85f));  // remaches
    b.box(Vec3(0, CHAPA_H, 0.055f), Vec3(COURT_W / 2, 0.035f, 0.01f), LINE);
    b.box(Vec3(0, TOP_LINE, 0.012f), Vec3(COURT_W / 2, 0.06f, 0.01f), LINE);
    b.box(Vec3(XR, WALL_H / 2, 0.012f), Vec3(lw, WALL_H / 2, 0.01f), LINE);  // raya del lado abierto en el frontis

    // Pared izquierda de bloques con las rayas de los cuadros (falta y pasa en rojo)
    b.box(Vec3(XL - 0.45f, WALL_H / 2, back / 2), Vec3(0.42f, WALL_H / 2, back / 2), shade(WALL, 0.9f));
    blockWall(b, Vec3(XL + 0.002f, 0, back), Vec3(0, 0, -1), Vec3(0, 1, 0), back, WALL_H, 1.2f, 0.6f, WALL, Vec3(1, 0, 0), 11);
    b.box(Vec3(XL - 0.4f, WALL_H + 0.18f, back / 2), Vec3(0.5f, 0.18f, back / 2 + 0.1f), WALL_TOP);
    for (float z = CUADRO; z < back; z += CUADRO)
        b.box(Vec3(XL + 0.008f, WALL_H / 2, z), Vec3(0.006f, WALL_H / 2, lw * 0.7f), LINE);
    b.box(Vec3(XL + 0.012f, WALL_H / 2, FALTA_Z), Vec3(0.006f, WALL_H / 2, lw), LINE_RED);
    b.box(Vec3(XL + 0.012f, WALL_H / 2, PASA_Z), Vec3(0.006f, WALL_H / 2, lw), LINE_RED);
    b.box(Vec3(XL + 0.008f, TOP_LINE, back / 2), Vec3(0.006f, 0.06f, back / 2), LINE);

    // Rebote (pared del fondo, baja)
    b.box(Vec3((XL + out) / 2, 1.5f, back + 0.3f), Vec3((out - XL) / 2 + 0.4f, 1.5f, 0.3f), STONE);
    b.box(Vec3((XL + out) / 2, 3.05f, back + 0.3f), Vec3((out - XL) / 2 + 0.5f, 0.06f, 0.36f), WALL_TOP);

    // Grada del lado abierto: escalones de piedra con asientos azules y un muro al fondo
    unsigned stoneSeed = 3;
    for (int i = 0; i < 4; i++) {
        float x = out + 0.6f + i * 1.0f, h = 0.45f * (i + 1);
        b.box(Vec3(x, h / 2, back / 2 + 1), Vec3(0.5f, h / 2, back / 2 + 1), jitter(STONE, stoneSeed, 0.04f));
        b.box(Vec3(x - 0.12f, h + 0.05f, back / 2 + 1), Vec3(0.3f, 0.05f, back / 2 - 1), SEAT);
    }
    b.box(Vec3(out + 4.6f, 2.6f, back / 2 + 1), Vec3(0.3f, 2.6f, back / 2 + 1), STONE);
    // Barandilla blanca entre la contracancha y la grada
    for (float z = 1.0f; z < back; z += 2.0f) b.box(Vec3(out + 0.05f, 0.5f, z), Vec3(0.03f, 0.5f, 0.03f), LINE);
    b.box(Vec3(out + 0.05f, 1.0f, back / 2), Vec3(0.04f, 0.03f, back / 2), LINE);
    b.build(mesh_);

    // Lejos: casas blancas de tejado rojo detrás de la grada y montes verdes alrededor
    MeshBuilder f;
    unsigned seed = 21;
    for (int i = 0; i < 9; i++) {
        float z = -6 + i * 5.2f, h = 6 + (i % 3) * 2.2f, x = out + 9.0f + (i % 2) * 1.5f;
        Color wall = jitter(rgba(0xF4, 0xF0, 0xE6), seed, 0.04f);
        f.box(Vec3(x, h / 2, z), Vec3(2.2f, h / 2, 2.3f), wall);
        f.box(Vec3(x, h + 0.35f, z), Vec3(2.5f, 0.35f, 2.6f), jitter(rgba(0xC0, 0x4A, 0x30), seed, 0.08f));
        for (int w = 0; w < 3; w++)  // ventanas con contraventanas verdes
            f.box(Vec3(x - 2.21f, 2.0f + w * 2.0f, z), Vec3(0.02f, 0.45f, 0.9f), rgba(0x2E, 0x7A, 0x4A));
    }
    const Color hill = rgba(0x5E, 0xA8, 0x4E), hill2 = rgba(0x4A, 0x92, 0x48);
    for (int i = 0; i < 10; i++) {
        float a = -1.3f + i * 0.33f, r = 120 + (i % 3) * 25;
        Vec3 c(sinf(a) * r * 0.9f - 10, -2, -cosf(a) * r * 0.6f + 10);
        f.cylinder(c, 40 + (i % 4) * 9, 6, 22 + (i % 3) * 10, i & 1 ? hill : hill2, hill, 10, false);
    }
    f.build(far_);

    // Público en la grada: Miis de la consola si los hay, sentados mirando a la cancha
    numFans_ = 0;
    for (int k = 0; k < MAX_CROWD; k++) {
        Fan& s = fans_[numFans_++];
        int row = k % 3;
        float z = 3.0f + (k / 3) * 5.1f + (row & 1) * 2.4f + frand(-0.4f, 0.4f);
        float x = out + 0.6f + row * 1.0f + 0.05f;
        s.pos = Vec3(x, 0.45f * (row + 1) + 0.05f, z);
        s.phase = frand(0, 6.28f);
        s.jump = s.jumpV = 0;
        s.look.color = (u8)((k * 5 + 2) % NUM_COLORS);
        s.look.skin = (u8)(k % 3);
        s.look.hair = (u8)((k * 3) % NUM_HAIRS);
        s.look.style = (u8)((k * 7) % NUM_HAIR_STYLES);
        s.look.txapela = (k % 4) == 1;
        s.look.casual = 1;
        if (mii::userCount() > 0) s.look.user = (s8)((k * 7 + 5) % mii::userCount());
    }
}

void Fronton::update(float dt) {
    time_ += dt;
    cheerT_ = fmaxf(0.0f, cheerT_ - dt);
    clapT_ = fmaxf(0.0f, clapT_ - dt);
    for (int i = 0; i < numFans_; i++) {
        Fan& s = fans_[i];
        if (cheerT_ > 0 && s.jump <= 0 && frand() < dt * 2.5f) s.jumpV = frand(1.4f, 2.2f);
        s.jumpV -= 9.0f * dt;
        s.jump = fmaxf(0.0f, s.jump + s.jumpV * dt);
        if (s.jump <= 0 && s.jumpV < 0) s.jumpV = 0;
    }
}

void Fronton::draw() {
    gfx::drawMesh(far_, Xf());
    gfx::drawMesh(mesh_, Xf());
    const float yaw = -PI / 2;  // mirando a la cancha (-X)
    for (int i = 0; i < numFans_; i++) {
        Fan& s = fans_[i];
        CharacterModel* m = characterCache(s.look);  // en cada frame (ver characterCache)
        if (!m) continue;
        CharacterPose p;
        p.pos = s.pos + Vec3(0, s.jump, 0);
        p.yaw = yaw;
        p.lean = sinf(time_ * 1.3f + s.phase) * 0.04f;
        p.blink = fmodf(time_ + s.phase, 4.0f) < 0.12f;
        Xf root = Xf().translate(p.pos).rotateY(yaw);
        if (cheerT_ > 0 && s.jump > 0.02f) {
            p.handL = root.apply(Vec3(-0.3f, 1.3f, 0.05f));
            p.handR = root.apply(Vec3(0.3f, 1.3f, 0.05f));
        } else if (clapT_ > 0 || cheerT_ > 0) {
            float c = fabsf(sinf(time_ * 14.0f + s.phase)) * 0.1f;
            p.handL = root.apply(Vec3(-0.045f - c, 0.72f, 0.26f));
            p.handR = root.apply(Vec3(0.045f + c, 0.72f, 0.26f));
        } else {
            float sway = sinf(time_ * 1.3f + s.phase) * 0.02f;
            p.handL = root.apply(Vec3(-0.21f, 0.47f + sway, 0.04f));
            p.handR = root.apply(Vec3(0.21f, 0.47f - sway, 0.04f));
        }
        drawCharacter(*m, p);
    }
}

void Fronton::drawLabels() {
    // Número de cada cuadro en la pared izquierda, abajo (como en los frontones de verdad)
    char buf[8];
    for (int k = 1; k * CUADRO < COURT_LEN + 1; k++) {
        float sx, sy;
        Vec3 p(-COURT_W / 2 + 0.02f, 1.6f, k * CUADRO - CUADRO * 0.5f);
        if (!gfx::project(p, sx, sy)) continue;
        snprintf(buf, sizeof(buf), "%d", k);
        font::draw(buf, sx, sy - 14, 26, LINE, rgba(0x2A, 0x5A, 0x3C), ALIGN_CENTER, 0.85f);
    }
}
