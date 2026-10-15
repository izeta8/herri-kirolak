#include "game/world.h"
#include "game/mii/mii.h"
#include "game/palette.h"
#include "engine/gfx.h"

// Escenario de aizkolaritza: el probaleku de Usurbil (entre Etxebeste kalea e Irazu kalea).
// Orientación: −Z = sur (iglesia de San Salvador con la torre y el pórtico), +Z = norte (frontón cubierto),
// +X = oeste (casas de Etxebeste kalea), −X = este (gradas con toldo y casas de Irazu kalea).
// La plataforma empedrada está a y = 0; las calles alrededor, un metro más abajo, con muro de cantos rodados.

namespace {

const float PX0 = -10.5f, PX1 = 9.5f;   // bordes este / oeste de la plataforma
const float PZ0 = -20.0f, PZ1 = 19.0f;  // bordes sur / norte
const float STREET_Y = -1.0f;

const Color STONE_CHURCH = rgba(0xC2, 0xAE, 0x88), STONE_CHURCH_D = rgba(0xA8, 0x95, 0x72);
const Color ASHLAR = rgba(0xCC, 0xB8, 0x92), ASHLAR_D = rgba(0xB4, 0xA0, 0x7C);
const Color RENDER = rgba(0xF4, 0xF2, 0xEC), TIMBER = rgba(0x5E, 0x3C, 0x26), ROOF = rgba(0xB8, 0x5C, 0x3C);
const Color GLASS = rgba(0x2E, 0x36, 0x42);

Color jitter(Color c, float k) { return shade(c, 1.0f + frand(-k, k)); }

// Rectángulo vertical en el plano local z = dz (mirando a +z), de (x0, y0) a (x1, y1)
void face(MeshBuilder& b, float x0, float y0, float x1, float y1, float dz, Color c) {
    b.quad(Vec3(x0, y0, dz), Vec3(x1, y0, dz), Vec3(x1, y1, dz), Vec3(x0, y1, dz), c, Vec3(0, 0, 1));
}
// Rectángulo horizontal (mirando arriba o abajo)
void flat(MeshBuilder& b, float x0, float z0, float x1, float z1, float y, Color c, bool up = true) {
    b.quad(Vec3(x0, y, z0), Vec3(x1, y, z0), Vec3(x1, y, z1), Vec3(x0, y, z1), c, Vec3(0, up ? 1.0f : -1.0f, 0));
}

// ------------------------------------------------------------------ suelo, plataforma y muros
void addGround(MeshBuilder& b) {
    // Calles y plazas de alrededor: losas grises claras
    for (int i = -14; i < 14; i++)
        for (int j = -20; j < 20; j++) {
            float x = i * 3.0f, z = j * 3.0f;
            if (x >= PX0 && x + 3 <= PX1 && z >= PZ0 && z + 3 <= PZ1) continue;
            flat(b, x, z, x + 3, z + 3, STREET_Y, jitter(rgba(0xAE, 0xAA, 0xA2), 0.04f));
        }
    // Plataforma: banda de hormigón claro con juntas
    const float CX0 = -6.5f, CX1 = 6.5f, CZ0 = -17.0f, CZ1 = 16.0f;
    for (float x = PX0; x < PX1 - 0.01f; x += 2.0f)
        for (float z = PZ0; z < PZ1 - 0.01f; z += 2.0f) {
            float x1 = fminf(x + 2, PX1), z1 = fminf(z + 2, PZ1);
            if (x >= CX0 && x1 <= CX1 && z >= CZ0 && z1 <= CZ1) continue;
            flat(b, x + 0.02f, z + 0.02f, x1 - 0.02f, z1 - 0.02f, 0.0f, jitter(rgba(0xCE, 0xCC, 0xC4), 0.03f));
        }
    flat(b, PX0, PZ0, PX1, PZ1, -0.01f, rgba(0x9A, 0x98, 0x92));  // juntas
    // El empedrado ("las piedras"): adoquines pequeños de tonos grises y ocres
    const Color cobble[] = {rgba(0x70, 0x6E, 0x6A), rgba(0x76, 0x73, 0x6E), rgba(0x7C, 0x78, 0x72), rgba(0x72, 0x6E, 0x68),
                            rgba(0x80, 0x7B, 0x73), rgba(0x6C, 0x6A, 0x67)};
    // Hileras al tresbolillo de adoquines de largo variable
    const float S = 0.22f;
    int row = 0;
    for (float z = CZ0; z < CZ1 - 0.01f; z += S, row++) {
        float x = CX0 - ((row & 1) ? S * 0.6f : 0.0f);
        while (x < CX1 - 0.01f) {
            float len = frand(0.2f, 0.3f);
            float x0 = fmaxf(x, CX0), x1 = fminf(x + len, CX1);
            if (x1 - x0 > 0.05f) flat(b, x0 + 0.02f, z + 0.02f, x1 - 0.02f, z + S - 0.02f, 0.012f, cobble[rand() % 6]);
            x += len;
        }
    }
    flat(b, CX0, CZ0, CX1, CZ1, 0.004f, rgba(0x60, 0x5E, 0x5A));  // llagas entre adoquines
    // Bordillo de piedra que rodea el empedrado
    flat(b, CX0 - 0.25f, CZ0 - 0.25f, CX1 + 0.25f, CZ0, 0.008f, rgba(0xB0, 0xAE, 0xA6));
    flat(b, CX0 - 0.25f, CZ1, CX1 + 0.25f, CZ1 + 0.25f, 0.008f, rgba(0xB0, 0xAE, 0xA6));
    flat(b, CX0 - 0.25f, CZ0, CX0, CZ1, 0.008f, rgba(0xB0, 0xAE, 0xA6));
    flat(b, CX1, CZ0, CX1 + 0.25f, CZ1, 0.008f, rgba(0xB0, 0xAE, 0xA6));
}

// Muro de cantos rodados (como el de la calle): mortero oscuro y piedras abombadas
void stoneWall(MeshBuilder& b, float len, float h) {
    face(b, 0, 0, len, h, 0, rgba(0x6E, 0x66, 0x5A));
    const Color pebbles[] = {rgba(0xA8, 0x9C, 0x86), rgba(0x8E, 0x86, 0x78), rgba(0xBA, 0xAE, 0x96), rgba(0x80, 0x78, 0x6C),
                             rgba(0x9C, 0x8C, 0x72)};
    for (float y = 0.1f; y < h - 0.05f; y += 0.2f)
        for (float x = frand(0, 0.15f); x < len - 0.12f; x += frand(0.26f, 0.36f)) {
            float rx = frand(0.12f, 0.17f), ry = frand(0.07f, 0.1f);
            Color c = pebbles[rand() % 5];
            Vec3 cen(x + rx, y, 0.045f);
            const int n = 6;
            for (int k = 0; k < n; k++) {
                float a0 = 2 * PI * k / n, a1 = 2 * PI * (k + 1) / n;
                Vec3 p0(x + rx + cosf(a0) * rx, y + sinf(a0) * ry, 0.005f), p1(x + rx + cosf(a1) * rx, y + sinf(a1) * ry, 0.005f);
                b.triSmooth({cen, Vec3(0, 0, 1), c}, {p0, normalize(Vec3(cosf(a0), sinf(a0), 0.8f)), shade(c, 0.85f)},
                            {p1, normalize(Vec3(cosf(a1), sinf(a1), 0.8f)), shade(c, 0.85f)});
            }
        }
}

void addWalls(MeshBuilder& b) {
    Xf saved = b.xf;
    float h = -STREET_Y;
    // Oeste (Etxebeste) y este (Irazu): muros de cantos mirando a la calle
    b.xf = Xf().translate(Vec3(PX1, STREET_Y, PZ1)).rotateY(PI / 2);
    stoneWall(b, PZ1 - PZ0, h);
    b.xf = Xf().translate(Vec3(PX0, STREET_Y, PZ0)).rotateY(-PI / 2);
    stoneWall(b, PZ1 - PZ0, h);
    b.xf = Xf().translate(Vec3(PX1, STREET_Y, PZ0)).rotateY(PI);
    stoneWall(b, PX1 - PX0, h);
    b.xf = Xf().translate(Vec3(PX0, STREET_Y, PZ1));
    stoneWall(b, PX1 - PX0, h);
    b.xf = saved;
    // Albardilla: piedra gris corrida encima del muro, un poco más alta que la plataforma
    const Color cap = rgba(0xA4, 0xA2, 0x9C), capTop = rgba(0xBC, 0xBA, 0xB4);
    for (int side = 0; side < 4; side++) {
        Vec3 c, half;
        if (side == 0) { c = Vec3(PX1 - 0.15f, 0.1f, (PZ0 + PZ1) / 2); half = Vec3(0.28f, 0.12f, (PZ1 - PZ0) / 2); }
        if (side == 1) { c = Vec3(PX0 + 0.15f, 0.1f, (PZ0 + PZ1) / 2); half = Vec3(0.28f, 0.12f, (PZ1 - PZ0) / 2); }
        if (side == 2) { c = Vec3((PX0 + PX1) / 2, 0.1f, PZ0 + 0.15f); half = Vec3((PX1 - PX0) / 2, 0.12f, 0.28f); }
        if (side == 3) { c = Vec3((PX0 + PX1) / 2, 0.1f, PZ1 - 0.15f); half = Vec3((PX1 - PX0) / 2, 0.12f, 0.28f); }
        b.box(c, half, cap);
        flat(b, c.x - half.x, c.z - half.z, c.x + half.x, c.z + half.z, c.y + half.y + 0.002f, capTop);
    }
    // Mojón redondo de cantos en la esquina noreste (el de Irazu kalea)
    b.cylinder(Vec3(PX0 - 0.4f, STREET_Y, PZ1 + 0.4f), 0.32f, 0.3f, 1.55f, rgba(0x96, 0x8C, 0x7C), rgba(0xA4, 0xA2, 0x9C), 10, true);
}

// ------------------------------------------------------------------ casas
struct HouseLook {
    float width, depth;
    int floors;
    bool timber;      // entramado de madera a la vista
    bool stoneFrames; // ventanas recercadas en piedra (casa grande de Irazu)
    Color shutter;
    int windows;
};

void window(MeshBuilder& b, float cx, float y0, float w, float h, const HouseLook& L) {
    face(b, cx - w / 2, y0, cx + w / 2, y0 + h, 0.012f, GLASS);
    Color fr = L.stoneFrames ? ASHLAR : rgba(0x6A, 0x44, 0x2A);
    float t = L.stoneFrames ? 0.16f : 0.07f;
    face(b, cx - w / 2 - t, y0 - t, cx + w / 2 + t, y0, 0.03f, fr);
    face(b, cx - w / 2 - t, y0 + h, cx + w / 2 + t, y0 + h + t, 0.03f, fr);
    face(b, cx - w / 2 - t, y0, cx - w / 2, y0 + h, 0.03f, fr);
    face(b, cx + w / 2, y0, cx + w / 2 + t, y0 + h, 0.03f, fr);
    face(b, cx - 0.02f, y0, cx + 0.02f, y0 + h, 0.02f, fr);  // parteluz
    // Contraventanas abiertas a los lados
    face(b, cx - w / 2 - t - w * 0.5f, y0, cx - w / 2 - t, y0 + h, 0.05f, L.shutter);
    face(b, cx + w / 2 + t, y0, cx + w / 2 + t + w * 0.5f, y0 + h, 0.05f, L.shutter);
}

void balcony(MeshBuilder& b, float x0, float x1, float y, bool flowers) {
    const Color wood = rgba(0x5A, 0x38, 0x22);
    b.box(Vec3((x0 + x1) / 2, y, 0.45f), Vec3((x1 - x0) / 2, 0.06f, 0.45f), wood);
    b.box(Vec3((x0 + x1) / 2, y + 0.95f, 0.88f), Vec3((x1 - x0) / 2, 0.04f, 0.04f), wood);
    for (float x = x0 + 0.06f; x < x1; x += 0.16f) face(b, x, y, x + 0.05f, y + 0.95f, 0.9f, wood);
    face(b, x0, y + 0.06f, x0 + 0.06f, y + 0.95f, 0.9f, wood);
    if (flowers)
        for (float x = x0 + 0.3f; x < x1 - 0.3f; x += 0.7f) {
            b.box(Vec3(x, y + 1.05f, 0.82f), Vec3(0.22f, 0.08f, 0.08f), rgba(0x8A, 0x4A, 0x2A));
            b.sphere(Vec3(x, y + 1.2f, 0.82f), Vec3(0.24f, 0.12f, 0.1f), rgba(0x3E, 0x8A, 0x36), 8, 4);
            for (int k = -1; k <= 1; k++)
                b.sphere(Vec3(x + k * 0.12f, y + 1.27f, 0.88f), Vec3(0.05f, 0.05f, 0.05f), rgba(0xE0, 0x28, 0x3A), 5, 3);
        }
}

// Casa con la fachada en el plano local z = 0 mirando a +z; de x = 0 a width; planta baja de piedra
void house(MeshBuilder& b, const HouseLook& L, bool ikurrina) {
    const float GF = 3.2f, FH = 2.9f;
    float H = GF + L.floors * FH;
    b.box(Vec3(L.width / 2, H / 2, -L.depth / 2), Vec3(L.width / 2, H / 2, L.depth / 2), RENDER);
    // Planta baja: sillares
    for (float y = 0; y < GF - 0.01f; y += 0.4f)
        for (float x = (int(y / 0.4f) & 1) ? -0.35f : 0.0f; x < L.width; x += 0.7f)
            face(b, fmaxf(x, 0.0f) + 0.015f, y + 0.015f, fminf(x + 0.7f, L.width) - 0.015f, y + 0.385f, 0.02f, jitter(ASHLAR, 0.06f));
    face(b, 0, 0, L.width, GF, 0.01f, ASHLAR_D);
    // Puertas en arco (bajo) y escaparate
    float door = L.width * 0.3f;
    face(b, door - 0.6f, 0, door + 0.6f, 2.4f, 0.03f, rgba(0x5A, 0x3A, 0x22));
    face(b, door - 0.75f, 2.4f, door + 0.75f, 2.65f, 0.03f, ASHLAR_D);
    face(b, L.width * 0.62f, 0.6f, L.width * 0.88f, 2.3f, 0.03f, GLASS);
    // Pisos
    float step = L.width / L.windows;
    for (int f = 0; f < L.floors; f++) {
        float y0 = GF + f * FH;
        if (L.timber) {
            face(b, 0, y0, L.width, y0 + 0.16f, 0.025f, TIMBER);
            face(b, 0, y0, 0.2f, y0 + FH, 0.025f, TIMBER);
            face(b, L.width - 0.2f, y0, L.width, y0 + FH, 0.025f, TIMBER);
            for (int w = 1; w < L.windows; w++) face(b, w * step - 0.08f, y0, w * step + 0.08f, y0 + FH, 0.025f, TIMBER);
        }
        for (int w = 0; w < L.windows; w++) window(b, step * (w + 0.5f), y0 + 0.75f, step * 0.34f, 1.45f, L);
        if (f < 2 && (f == 0 || L.timber)) balcony(b, step * 0.15f, L.width - step * 0.15f, y0 + 0.12f, (f + L.windows) % 2 == 0);
    }
    if (L.timber) face(b, 0, H - 0.18f, L.width, H, 0.025f, TIMBER);
    if (ikurrina) {
        // Ikurriña colgada del balcón del primer piso
        float x = L.width * 0.5f, y = GF + 0.15f, z = 0.92f;
        Vec3 o(x - 0.6f, y - 0.75f, z);
        const float w = 1.2f, h = 0.75f;
        b.quad(o, o + Vec3(w, 0, 0), o + Vec3(w, h, 0), o + Vec3(0, h, 0), rgba(0xD5, 0x2B, 0x1E), Vec3(0, 0, 1));
        float t = 0.07f, z2 = 0.006f;
        b.quad(o + Vec3(0, t, z2), o + Vec3(t, 0, z2), o + Vec3(w, h - t, z2), o + Vec3(w - t, h, z2), rgba(0x00, 0x96, 0x3E), Vec3(0, 0, 1));
        b.quad(o + Vec3(0, h - t, z2), o + Vec3(t, h, z2), o + Vec3(w, t, z2), o + Vec3(w - t, 0, z2), rgba(0x00, 0x96, 0x3E), Vec3(0, 0, 1));
        b.quad(o + Vec3(w / 2 - 0.04f, 0, 0.012f), o + Vec3(w / 2 + 0.04f, 0, 0.012f), o + Vec3(w / 2 + 0.04f, h, 0.012f),
               o + Vec3(w / 2 - 0.04f, h, 0.012f), UI_WHITE, Vec3(0, 0, 1));
        b.quad(o + Vec3(0, h / 2 - 0.04f, 0.012f), o + Vec3(w, h / 2 - 0.04f, 0.012f), o + Vec3(w, h / 2 + 0.04f, 0.012f),
               o + Vec3(0, h / 2 + 0.04f, 0.012f), UI_WHITE, Vec3(0, 0, 1));
    }
    // Alero de madera muy volado y tejado a un agua hacia atrás
    const float eave = 1.1f;
    flat(b, -0.3f, 0, L.width + 0.3f, eave, H + 0.05f, rgba(0x7A, 0x4E, 0x30), false);
    for (float x = 0.1f; x < L.width; x += 0.55f) b.box(Vec3(x, H, eave * 0.5f), Vec3(0.05f, 0.07f, eave * 0.5f), rgba(0x6A, 0x42, 0x28));
    Vec3 e0(-0.3f, H + 0.12f, eave), e1(L.width + 0.3f, H + 0.12f, eave);
    Vec3 r0(-0.3f, H + 2.6f, -L.depth * 0.5f), r1(L.width + 0.3f, H + 2.6f, -L.depth * 0.5f);
    b.quad(e0, e1, r1, r0, ROOF, Vec3(0, 1, 1));
    b.quad(r0, r1, Vec3(L.width + 0.3f, H, -L.depth - 0.3f), Vec3(-0.3f, H, -L.depth - 0.3f), shade(ROOF, 0.9f), Vec3(0, 1, -1));
    face(b, -0.3f, H, L.width + 0.3f, H + 0.14f, eave, rgba(0x6A, 0x42, 0x28));
}

void wallLamp(MeshBuilder& b, const Vec3& p) {
    const Color iron = rgba(0x22, 0x22, 0x24);
    b.box(p + Vec3(0, 0, 0.3f), Vec3(0.03f, 0.03f, 0.3f), iron);
    b.cylinder(p + Vec3(0, -0.45f, 0.6f), 0.08f, 0.14f, 0.4f, rgba(0xF2, 0xEE, 0xD8), iron, 6, true);
    b.cylinder(p + Vec3(0, -0.05f, 0.6f), 0.16f, 0.04f, 0.18f, iron, iron, 6, true);
}

void addHouses(MeshBuilder& b) {
    Xf saved = b.xf;
    // Etxebeste kalea (oeste), de sur a norte, fachadas mirando a la plaza
    const HouseLook west[] = {
        {7.5f, 10, 3, true, false, rgba(0x7A, 0x2A, 0x22), 3}, {8.5f, 10, 3, true, false, rgba(0x6A, 0x3A, 0x22), 3},
        {7.0f, 10, 3, false, false, rgba(0x8A, 0x2C, 0x24), 2}, {9.0f, 10, 3, true, false, rgba(0x5A, 0x3A, 0x24), 3},
        {8.0f, 10, 2, true, false, rgba(0x7A, 0x2A, 0x22), 3},
    };
    float z = PZ0 - 3.0f;
    for (int i = 0; i < 5; i++) {
        b.xf = Xf().translate(Vec3(15.5f, STREET_Y, z)).rotateY(-PI / 2);
        house(b, west[i], i == 1);
        wallLamp(b, Vec3(west[i].width * 0.85f, 3.0f, 0));
        z += west[i].width;
    }
    // Irazu kalea (este), de norte a sur
    const HouseLook east[] = {
        {12.0f, 12, 2, false, true, rgba(0x5A, 0x3A, 0x24), 4},  // la casa grande con ventanas recercadas
        {8.0f, 10, 3, true, false, rgba(0x8A, 0x2C, 0x24), 3}, {9.5f, 10, 3, false, false, rgba(0x6A, 0x3A, 0x22), 3},
        {8.0f, 10, 3, true, false, rgba(0x7A, 0x2A, 0x22), 3}, {7.0f, 10, 3, true, false, rgba(0x5A, 0x3A, 0x24), 2},
    };
    z = PZ1 + 10.0f;
    for (int i = 0; i < 5; i++) {
        b.xf = Xf().translate(Vec3(-16.5f, STREET_Y, z)).rotateY(PI / 2);
        house(b, east[i], false);
        wallLamp(b, Vec3(east[i].width * 0.15f, 3.0f, 0));
        z -= east[i].width;
    }
    b.xf = saved;
}

// ------------------------------------------------------------------ iglesia de San Salvador
void courses(MeshBuilder& b, float x0, float x1, float y0, float y1, float dz) {
    for (float y = y0 + 0.45f; y < y1; y += 0.45f) face(b, x0, y, x1, y + 0.04f, dz, STONE_CHURCH_D);
}

void arch(MeshBuilder& b, float cx, float y0, float w, float h, float dz, Color c) {
    face(b, cx - w / 2, y0, cx + w / 2, y0 + h - w / 2, dz, c);
    const int n = 8;
    Vec3 cen(cx, y0 + h - w / 2, dz);
    for (int k = 0; k < n; k++) {
        float a0 = PI * k / n, a1 = PI * (k + 1) / n;
        b.tri(cen, cen + Vec3(cosf(a0) * w / 2, sinf(a0) * w / 2, 0), cen + Vec3(cosf(a1) * w / 2, sinf(a1) * w / 2, 0), c, Vec3(0, 0, 1));
    }
}

void addChurch(MeshBuilder& b) {
    Xf saved = b.xf;
    // Nave: de x −15 a 4, fachada norte en z = −27
    const float NX0 = -15, NX1 = 4, NZ = -27, ND = 24, NH = 17;
    b.xf = Xf().translate(Vec3(0, STREET_Y, 0));
    b.box(Vec3((NX0 + NX1) / 2, NH / 2, NZ - ND / 2), Vec3((NX1 - NX0) / 2, NH / 2, ND / 2), STONE_CHURCH);
    b.xf = Xf().translate(Vec3(0, STREET_Y, NZ));
    courses(b, NX0, NX1, 0, NH, 0.01f);
    // Ventanales altos
    for (float x : {-11.5f, -5.5f, 0.5f}) arch(b, x, 10.5f, 1.4f, 4.0f, 0.02f, rgba(0x3A, 0x34, 0x2E));
    // Contrafuertes escalonados
    for (float x : {-14.0f, -8.5f, -2.5f}) {
        b.box(Vec3(x, 7.0f, 0.9f), Vec3(0.8f, 7.0f, 0.9f), STONE_CHURCH);
        b.box(Vec3(x, 15.0f, 0.5f), Vec3(0.7f, 1.6f, 0.5f), STONE_CHURCH);
        courses(b, x - 0.8f, x + 0.8f, 0, 14, 1.81f);
    }
    // Capillas bajas entre contrafuertes, con su tejadillo
    for (float x : {-11.25f, -5.5f}) {
        b.box(Vec3(x, 2.2f, 1.0f), Vec3(2.0f, 2.2f, 1.0f), STONE_CHURCH_D);
        b.quad(Vec3(x - 2.1f, 4.4f, 2.2f), Vec3(x + 2.1f, 4.4f, 2.2f), Vec3(x + 2.1f, 5.4f, -0.05f), Vec3(x - 2.1f, 5.4f, -0.05f), ROOF,
               Vec3(0, 1, 1));
    }
    // Tejado de la nave
    b.xf = Xf().translate(Vec3(0, STREET_Y, 0));
    b.quad(Vec3(NX0 - 0.4f, NH, NZ + 0.4f), Vec3(NX1 + 0.4f, NH, NZ + 0.4f), Vec3(NX1 - 3, NH + 4, NZ - ND / 2),
           Vec3(NX0 + 3, NH + 4, NZ - ND / 2), ROOF, Vec3(0, 1, 1));
    b.quad(Vec3(NX0 - 0.4f, NH, NZ - ND - 0.4f), Vec3(NX1 + 0.4f, NH, NZ - ND - 0.4f), Vec3(NX1 - 3, NH + 4, NZ - ND / 2),
           Vec3(NX0 + 3, NH + 4, NZ - ND / 2), shade(ROOF, 0.9f), Vec3(0, 1, -1));
    b.tri(Vec3(NX0 - 0.4f, NH, NZ + 0.4f), Vec3(NX0 - 0.4f, NH, NZ - ND - 0.4f), Vec3(NX0 + 3, NH + 4, NZ - ND / 2), ROOF, Vec3(-1, 1, 0));

    // Torre en la esquina de Etxebeste, con el pórtico mirando a la plaza
    const float TX = 8.0f, TZ = -31.0f, TW = 4.2f;  // centro y semiancho
    b.box(Vec3(TX, 10.0f, TZ), Vec3(TW, 10.0f, TW), STONE_CHURCH);
    b.box(Vec3(TX, 1.0f, TZ), Vec3(TW + 0.25f, 1.0f, TW + 0.25f), STONE_CHURCH_D);  // zócalo
    b.xf = Xf().translate(Vec3(TX, STREET_Y, TZ + TW));
    courses(b, -TW, TW, 0, 20, 0.01f);
    // Pórtico: arco hundido con la puerta, columnas y hornacina encima
    arch(b, 0, 0, 3.4f, 6.2f, 0.02f, STONE_CHURCH_D);
    arch(b, 0, 0, 2.5f, 5.4f, 0.03f, rgba(0x4A, 0x40, 0x34));
    face(b, -0.9f, 0, 0.9f, 3.4f, 0.04f, rgba(0x5A, 0x3A, 0x22));
    for (int s = -1; s <= 1; s += 2) b.cylinder(Vec3(s * 2.0f, 0, 0.3f), 0.22f, 0.2f, 6.0f, STONE_CHURCH, STONE_CHURCH, 8, true);
    b.box(Vec3(0, 6.6f, 0.25f), Vec3(2.6f, 0.25f, 0.3f), STONE_CHURCH);  // cornisa
    face(b, -1.4f, 6.9f, 1.4f, 9.2f, 0.03f, rgba(0x4A, 0x40, 0x34));   // ventana del balcón
    b.box(Vec3(0, 6.95f, 0.55f), Vec3(1.6f, 0.08f, 0.55f), STONE_CHURCH_D);
    for (float x = -1.5f; x <= 1.5f; x += 0.25f) face(b, x, 7.0f, x + 0.05f, 7.9f, 1.08f, rgba(0x26, 0x26, 0x28));
    b.box(Vec3(0, 7.9f, 1.05f), Vec3(1.6f, 0.04f, 0.04f), rgba(0x26, 0x26, 0x28));
    arch(b, 0, 10.2f, 1.6f, 2.6f, 0.03f, STONE_CHURCH_D);  // hornacina con el escudo
    b.sphere(Vec3(0, 11.1f, 0.1f), Vec3(0.5f, 0.55f, 0.15f), STONE_CHURCH, 8, 5);
    // Campanario: cuerpo más estrecho con huecos en arco en las cuatro caras
    b.xf = Xf().translate(Vec3(TX, STREET_Y, TZ));
    b.box(Vec3(0, 20.2f, 0), Vec3(TW + 0.3f, 0.3f, TW + 0.3f), STONE_CHURCH_D);
    b.box(Vec3(0, 24.0f, 0), Vec3(TW - 0.4f, 3.6f, TW - 0.4f), STONE_CHURCH);
    for (int k = 0; k < 4; k++) {
        Xf inner = b.xf;
        b.xf = inner * Xf().rotateY(k * PI / 2).translate(Vec3(0, 0, TW - 0.4f));
        arch(b, 0, 21.2f, 1.8f, 4.6f, 0.02f, rgba(0x2E, 0x2A, 0x26));
        b.xf = inner;
    }
    b.box(Vec3(0, 27.9f, 0), Vec3(TW, 0.3f, TW), STONE_CHURCH_D);
    // Remate: tambor octogonal y cúpula
    b.cylinder(Vec3(0, 28.2f, 0), 3.0f, 2.8f, 2.6f, STONE_CHURCH, STONE_CHURCH, 8, false);
    b.sphere(Vec3(0, 30.8f, 0), Vec3(2.8f, 2.4f, 2.8f), shade(STONE_CHURCH, 0.92f), 8, 6, 0.0f);
    b.cylinder(Vec3(0, 33.0f, 0), 0.5f, 0.3f, 1.4f, STONE_CHURCH, STONE_CHURCH, 8, true);
    b.box(Vec3(0, 35.2f, 0), Vec3(0.06f, 0.8f, 0.06f), rgba(0x30, 0x30, 0x30));
    b.box(Vec3(0, 35.5f, 0), Vec3(0.4f, 0.06f, 0.06f), rgba(0x30, 0x30, 0x30));
    b.xf = saved;
}

// ------------------------------------------------------------------ frontón cubierto (norte)
void addFronton(MeshBuilder& b) {
    const float Z0 = PZ1 + 1.5f, Z1 = 52, X0 = -16.5f, X1 = 15.5f, H = 11.5f;
    const Color wood = rgba(0xB0, 0x70, 0x40), col = rgba(0xEC, 0xEC, 0xE8);
    // Cubierta: tablas de madera por debajo y canto oscuro
    flat(b, X0, Z0, X1, Z1, H, wood, false);
    for (float x = X0; x < X1; x += 1.2f) b.box(Vec3(x + 0.6f, H - 0.25f, (Z0 + Z1) / 2), Vec3(0.12f, 0.25f, (Z1 - Z0) / 2), rgba(0x8A, 0x54, 0x2E));
    b.box(Vec3((X0 + X1) / 2, H + 0.4f, (Z0 + Z1) / 2), Vec3((X1 - X0) / 2 + 0.3f, 0.4f, (Z1 - Z0) / 2 + 0.3f), rgba(0x4A, 0x4C, 0x52));
    // Columnas blancas redondas
    for (float z = Z0 + 0.5f; z < Z1; z += 9.0f)
        for (float x : {X0 + 0.6f, X1 - 0.6f}) b.cylinder(Vec3(x, STREET_Y, z), 0.45f, 0.45f, H - STREET_Y, col, col, 12, false);
    // Fachada acristalada sobre la plaza con cuadros blancos y negros
    Xf saved = b.xf;
    b.xf = Xf().translate(Vec3(X1, 0, Z0)).rotateY(PI);  // mirando al sur, a la plaza
    face(b, 0, 6.2f, X1 - X0, H, 0, rgba(0x9C, 0xB0, 0xBC));
    for (float x = 0; x < X1 - X0; x += 1.6f) face(b, x, 6.2f, x + 0.08f, H, 0.02f, rgba(0x30, 0x32, 0x36));
    for (float y = 6.2f; y < H; y += 1.6f) face(b, 0, y, X1 - X0, y + 0.08f, 0.02f, rgba(0x30, 0x32, 0x36));
    for (int i = 0; i < 20; i++) {
        float x = (i * 7 % 19) * 1.6f + 0.08f, y = 6.28f + (i * 3 % 3) * 1.6f;
        face(b, x, y, x + 1.52f, y + 1.52f, 0.01f, (i & 1) ? rgba(0x22, 0x22, 0x26) : rgba(0xEE, 0xEE, 0xEA));
    }
    face(b, 0, 5.6f, X1 - X0, 6.2f, 0.05f, rgba(0xE6, 0xE6, 0xE2));
    b.xf = saved;
    // Paredes verdes del frontón (izquierda y frontis)
    const Color green = rgba(0x7C, 0xAE, 0x86);
    b.box(Vec3(X0 + 0.3f, (STREET_Y + 9) / 2, (Z0 + 4 + Z1) / 2), Vec3(0.3f, (9 - STREET_Y) / 2, (Z1 - Z0 - 4) / 2), green);
    b.box(Vec3((X0 + X1) / 2, (STREET_Y + 10) / 2, Z1 - 0.3f), Vec3((X1 - X0) / 2, (10 - STREET_Y) / 2, 0.3f), green);
    // Cancha con sus rayas
    flat(b, X0 + 0.6f, Z0, X1, Z1 - 0.6f, STREET_Y + 0.01f, rgba(0x5E, 0x66, 0x6E));
    for (float z = Z0 + 4; z < Z1; z += 4) flat(b, X0 + 0.6f, z, X1, z + 0.1f, STREET_Y + 0.02f, UI_WHITE);
}

// ------------------------------------------------------------------ gradas con toldo, porterías, banderitas
void addStands(MeshBuilder& b) {
    const float ZA = -13.0f, ZB = 11.0f;
    const Color metal = rgba(0x8C, 0x90, 0x96), plank = rgba(0xB4, 0x8E, 0x5E);
    // Cinco escalones subiendo hacia Irazu
    for (int k = 0; k < 5; k++) {
        float x = -7.4f - k * 0.62f, y = 0.32f + k * 0.38f;
        b.box(Vec3(x, y, (ZA + ZB) / 2), Vec3(0.3f, 0.04f, (ZB - ZA) / 2), plank);
        b.box(Vec3(x - 0.2f, y - 0.2f, (ZA + ZB) / 2), Vec3(0.02f, 0.2f, (ZB - ZA) / 2), metal);
    }
    for (float z = ZA; z <= ZB + 0.01f; z += 2.0f) {
        b.box(Vec3(-8.6f, 0.95f, z), Vec3(1.55f, 0.03f, 0.03f), metal);
        for (int k = 0; k < 5; k++) b.box(Vec3(-7.4f - k * 0.62f, (0.32f + k * 0.38f) / 2, z), Vec3(0.025f, (0.32f + k * 0.38f) / 2, 0.025f), metal);
    }
    // Barandilla de atrás
    b.box(Vec3(-10.25f, 2.95f, (ZA + ZB) / 2), Vec3(0.03f, 0.03f, (ZB - ZA) / 2), metal);
    // Toldo: lonas blancas en arcos, inclinado hacia la plaza
    for (float z = ZA; z < ZB; z += 2.0f) {
        b.cylinder(Vec3(-10.2f, 0, z), 0.04f, 0.04f, 4.4f, metal, metal, 6, false);
        b.cylinder(Vec3(-6.9f, 0, z), 0.04f, 0.04f, 3.5f, metal, metal, 6, false);
        const int n = 4;
        for (int k = 0; k < n; k++) {
            float t0 = k / (float)n, t1 = (k + 1) / (float)n;
            float x0 = lerpf(-10.4f, -6.5f, t0), x1 = lerpf(-10.4f, -6.5f, t1);
            float y0 = lerpf(4.4f, 3.5f, t0) + sinf(PI * t0) * 0.25f, y1 = lerpf(4.4f, 3.5f, t1) + sinf(PI * t1) * 0.25f;
            Color c = rgba(0xF2, 0xF2, 0xEE);
            b.quad(Vec3(x0, y0, z + 0.06f), Vec3(x1, y1, z + 0.06f), Vec3(x1, y1, z + 1.94f), Vec3(x0, y0, z + 1.94f), c, Vec3(0.2f, 1, 0));
            b.quad(Vec3(x0, y0 - 0.01f, z + 0.06f), Vec3(x1, y1 - 0.01f, z + 0.06f), Vec3(x1, y1 - 0.01f, z + 1.94f),
                   Vec3(x0, y0 - 0.01f, z + 1.94f), rgba(0xD8, 0xDA, 0xDA), Vec3(0, -1, 0));
        }
        b.box(Vec3(-8.45f, 4.1f, z), Vec3(1.95f, 0.03f, 0.03f), metal);
    }
}

void goal(MeshBuilder& b, const Vec3& p, float yaw) {
    Xf saved = b.xf;
    b.xf = Xf().translate(p).rotateY(yaw);
    const Color red = rgba(0xD8, 0x2A, 0x2A), white = UI_WHITE, net = rgba(0xD6, 0xD8, 0xDC);
    for (int s = -1; s <= 1; s += 2)
        for (int k = 0; k < 10; k++) b.box(Vec3(s * 1.5f, 0.1f + k * 0.2f, 0), Vec3(0.04f, 0.1f, 0.04f), (k & 1) ? red : white);
    for (int k = 0; k < 15; k++) b.box(Vec3(-1.4f + k * 0.2f, 2.0f, 0), Vec3(0.1f, 0.04f, 0.04f), (k & 1) ? red : white);
    // Red: cuadrícula de hilos hacia atrás
    for (float x = -1.5f; x <= 1.5f; x += 0.25f) b.box(Vec3(x, 1.0f, -0.5f), Vec3(0.006f, 1.0f, 0.006f), net);
    for (float y = 0.25f; y <= 2.0f; y += 0.25f) b.box(Vec3(0, y, -0.5f), Vec3(1.5f, 0.006f, 0.006f), net);
    for (float y = 0.25f; y <= 2.0f; y += 0.25f)
        for (int s = -1; s <= 1; s += 2) b.box(Vec3(s * 1.5f, y, -0.25f), Vec3(0.006f, 0.006f, 0.25f), net);
    b.xf = saved;
}

void bunting(MeshBuilder& b, const Vec3& a, const Vec3& c) {
    // Banderitas de fiestas en una cuerda entre dos puntos
    const int n = 26;
    for (int k = 0; k < n; k++) {
        float t = (k + 0.5f) / n;
        Vec3 p = lerpv(a, c, t) - Vec3(0, sinf(PI * t) * 1.2f, 0);
        Vec3 dir = normalize(c - a) * 0.22f;
        Color col = (k & 1) ? rgba(0xE8, 0x2A, 0x2A) : UI_WHITE;
        b.tri(p - dir, p + dir, p + Vec3(0, -0.45f, 0), col, Vec3(dir.z, 0, -dir.x));
        b.tri(p - dir, p + dir, p + Vec3(0, -0.45f, 0), col, Vec3(-dir.z, 0, dir.x));
    }
}

void addHills(MeshBuilder& b) {
    // Montes verdes de Gipuzkoa al fondo
    const Color hills[] = {rgba(0x5E, 0x9A, 0x4E), rgba(0x52, 0x8E, 0x48), rgba(0x6A, 0xA2, 0x56), rgba(0x4C, 0x86, 0x46)};
    for (int i = 0; i < 16; i++) {
        float a = 2 * PI * i / 16 + frand(-0.1f, 0.1f);
        float d = frand(150, 210), r = frand(45, 70);
        b.sphere(Vec3(cosf(a) * d, -r * 0.55f, sinf(a) * d), Vec3(r, r * frand(0.75f, 1.0f), r), hills[i % 4], 14, 7, -0.1f);
    }
}

}  // namespace

void World::build(unsigned seed) {
    srand(seed);
    MeshBuilder b;
    addGround(b);
    addWalls(b);
    addHouses(b);
    addChurch(b);
    addFronton(b);
    addStands(b);
    goal(b, Vec3(0, 0, 15.6f), PI);
    goal(b, Vec3(0, 0, -16.6f), 0);
    bunting(b, Vec3(4.0f, 8.5f, -26.6f), Vec3(15.5f, 8.0f, -18.0f));
    bunting(b, Vec3(-15.0f, 7.5f, -26.6f), Vec3(-16.5f, 8.0f, -14.0f));
    addHills(b);
    b.build(scenery_);

    b.clear();
    // Nubes
    for (int i = 0; i < 12; i++) {
        float a = frand(0, 2 * PI), d = frand(90, 150), y = frand(40, 60);
        Vec3 c(cosf(a) * d, y, sinf(a) * d);
        for (int k = 0; k < 5; k++)
            b.sphere(c + Vec3(frand(-8, 8), frand(-1, 3), frand(-5, 5)), Vec3(frand(6, 10), frand(3, 5), frand(4, 7)), UI_WHITE, 10, 6);
    }
    b.build(clouds_);

    b.clear();
    b.disc(Vec3(0, 0, 0), 1.0f, rgba(0x20, 0x20, 0x20, 0x55), 20);
    b.build(shadow_);

    // Público: en las gradas, a lo largo de Etxebeste y al fondo junto a la iglesia
    numSpectators_ = 0;
    auto add = [this](const Vec3& p, int k) {
        if (numSpectators_ >= MAX_SPECTATORS) return;
        Spectator& s = spectators_[numSpectators_++];
        s.pos = p;
        s.yaw = atan2f(-p.x, -p.z * 0.35f);  // mirando hacia el centro de la plaza
        s.phase = frand(0, 6.28f);
        s.jump = 0;
        s.jumpV = 0;
        // 10 aspectos distintos repartidos (para no llenar la caché de modelos)
        k = k % 10;
        s.look.color = (u8)((k * 5 + 1) % NUM_COLORS);
        s.look.skin = (u8)(k % 3);
        s.look.hair = (u8)((k * 3) % NUM_HAIRS);
        s.look.style = (u8)((k * 7) % NUM_HAIR_STYLES);
        s.look.txapela = (k % 4) == 0;
        s.look.casual = 1;
        // Con Miis en la consola, el público son esos Miis (repetidos si hay pocos)
        if (mii::userCount() > 0) s.look.user = (s8)((numSpectators_ * 7 + 3) % mii::userCount());
        s.model = nullptr;
    };
    int k = 0;
    for (int row = 0; row < 3; row++)
        for (int i = 0; i < 6; i++) add(Vec3(-7.6f - row * 1.24f, 0.36f + row * 0.76f, -9.0f + i * 3.4f + row * 1.1f + frand(-0.3f, 0.3f)), k++);
    for (int i = 0; i < 6; i++) add(Vec3(7.9f + frand(-0.3f, 0.3f), 0, -10.0f + i * 3.6f + frand(-0.5f, 0.5f)), k++);
    for (int i = 0; i < 5; i++) add(Vec3(-4.4f + i * 2.2f + frand(-0.3f, 0.3f), 0, -18.6f + frand(-0.3f, 0.3f)), k++);
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
    gfx::rectGrad(0, 0, w, 160, rgba(0x2C, 0x84, 0xE0), rgba(0x6A, 0xB4, 0xEE));
    gfx::rectGrad(0, 160, w, 140, rgba(0x6A, 0xB4, 0xEE), HORIZON);
    gfx::rect(0, 300, w, 180, HORIZON);
}

void World::drawShadow(const Vec3& pos, float rx, float rz, float alpha) {
    gfx::setLighting(false);
    gfx::setBlend(true);
    gfx::setDepth(true, false);
    (void)alpha;
    gfx::drawMesh(shadow_, Xf().translate(Vec3(pos.x, pos.y + 0.018f, pos.z)).scale(Vec3(rx, 1, rz)));
    gfx::setDepth(true, true);
    gfx::setBlend(false);
    gfx::setLighting(true);
}

bool World::hidden(const Spectator& s) const {
    if (!hideOn_) return false;
    // Con la cámara lejos, quitar al público que se interpone entre ella y los troncos
    Vec3 c(hideCam_.x, 0, hideCam_.z), p(s.pos.x, 0, s.pos.z);
    return length(c) > 4.5f && dot(p, normalize(c)) > 2.0f && fabsf(cross(normalize(c), p).y) < 2.5f;
}

void World::draw() {
    gfx::setFog(true, 90.0f, 320.0f, HORIZON);
    gfx::drawMesh(scenery_, Xf());
    gfx::drawMesh(clouds_, Xf().rotateY(time_ * 0.004f));
    gfx::setFog(false);

    for (int i = 0; i < numSpectators_; i++) {
        Spectator& s = spectators_[i];
        s.model = characterCache(s.look);  // en cada frame: así la caché sabe que está en uso
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
            p.handL = root.apply(Vec3(-0.045f - c, 0.72f, 0.26f));
            p.handR = root.apply(Vec3(0.045f + c, 0.72f, 0.26f));
        } else {
            float sway = sinf(time_ * 1.3f + s.phase) * 0.02f;
            p.handL = root.apply(Vec3(-0.21f, 0.47f + sway, 0.04f));
            p.handR = root.apply(Vec3(0.21f, 0.47f - sway, 0.04f));
        }
        drawCharacter(*s.model, p);
    }
}
