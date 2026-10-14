#include "game/character.h"
#include "engine/tuning.h"
#include "game/mii/mii.h"
#include "game/palette.h"

// Miis calcados a los de la Wii:
// - Cabeza alta en forma de huevo: ancha en la frente y con la barbilla en punta. Sin orejas a la vista.
// - Cara de dibujo de línea: ojos pequeños (iris negro con algo de blanco y raya del párpado), cejas gruesas
//   inclinadas, nariz de una raya en gancho y boca de una raya recta.
// - Pelo como un casco, con el flequillo en picos y los lados bajando hasta los ojos.
// - Cuerpo estrecho, brazos largos con manga del color de la camiseta, manos de bola y piernas largas.

static Mesh s_shoe;
static bool s_sharedBuilt = false;

static const float HIP_Y = 0.42f;
static const float NECK_Y = 0.48f;  // sobre la cadera
// Hombro respecto a la cadera (espacio del torso)
static const Vec3 SHOULDER(0.13f, 0.40f, 0.0f);

// Cabeza: elipsoide alto con la mitad de abajo en punta (barbilla). Origen en la barbilla.
static const float HA = 0.215f, HB = 0.27f, HC = 0.2f;
static const Vec3 HEAD_C(0, HB - 0.03f, 0);

static const Color PANTS_MII = rgba(0x5E, 0x58, 0x54);  // gris pardo de los Miis
static const Color PANTS_WHITE = rgba(0xF4, 0xF4, 0xF0);
static const Color INK = rgba(0x16, 0x14, 0x14);

static void buildShared() {
    if (s_sharedBuilt) return;
    MeshBuilder b;
    b.sphere(Vec3(0, 0.035f, 0.03f), Vec3(0.058f, 0.042f, 0.095f), rgba(0x4A, 0x48, 0x4C), 12, 6);
    b.build(s_shoe);
    s_sharedBuilt = true;
}

// ------------------------------------------------------------------ cabeza y cara
static float chinX(float yn) { return yn > 0.15f ? 1.0f : 1.0f - 0.48f * powf((0.15f - yn) / 1.15f, 1.35f); }
static float chinZ(float yn) { return yn > 0.15f ? 1.0f : 1.0f - 0.2f * powf((0.15f - yn) / 1.15f, 1.3f); }

// Punto de la superficie de la cabeza (relativo al centro) para ángulos polares
static Vec3 headSurf(float theta, float phi, float grow = 1.0f) {
    float yn = cosf(theta);
    return Vec3(HA * grow * chinX(yn) * sinf(theta) * cosf(phi), HB * grow * yn, HC * grow * chinZ(yn) * sinf(theta) * sinf(phi));
}
static Vec3 headNormal(const Vec3& p) { return normalize(Vec3(p.x / (HA * HA), p.y / (HB * HB), p.z / (HC * HC))); }

// Casquete de la superficie (cabeza o pelo): para cada meridiano, desde el polo hasta thetaMax(phi)
template <typename F>
static void headShell(MeshBuilder& b, const Vec3& center, float grow, Color col, F thetaMax, int rings, int segs) {
    for (int j = 0; j < segs; j++) {
        float p0 = 2 * PI * j / segs, p1 = 2 * PI * (j + 1) / segs;
        float t0max = thetaMax(p0), t1max = thetaMax(p1);
        for (int i = 0; i < rings; i++) {
            float a0 = t0max * i / rings, a1 = t0max * (i + 1) / rings;
            float c0 = t1max * i / rings, c1 = t1max * (i + 1) / rings;
            Vec3 v00 = headSurf(a0, p0, grow), v01 = headSurf(a1, p0, grow);
            Vec3 v10 = headSurf(c0, p1, grow), v11 = headSurf(c1, p1, grow);
            Vert A{center + v00, headNormal(v00), col}, B{center + v01, headNormal(v01), col};
            Vert C{center + v11, headNormal(v11), col}, D{center + v10, headNormal(v10), col};
            if (i > 0) b.triSmooth(A, B, C);
            b.triSmooth(A, C, D);
        }
    }
}

// Punto "pintado" en la cara: (x, y) relativos al centro de la cabeza, levantado `lift` sobre la piel
static Vec3 facePt(float x, float y, float lift) {
    float yn = clampv(y / HB, -1.0f, 1.0f);
    float xs = x / (HA * chinX(yn));
    float in = 1 - yn * yn - xs * xs;
    float z = HC * chinZ(yn) * sqrtf(fmaxf(in, 0.0f));
    return HEAD_C + Vec3(x, y, z + lift);
}

// Polígono convexo pintado (abanico desde el primer punto = centro)
static void decalFan(MeshBuilder& b, const float* xy, int n, float lift, Color col) {
    Vec3 c = facePt(xy[0], xy[1], lift);
    Vec3 nc = headNormal(c - HEAD_C);
    for (int k = 1; k < n; k++) {
        int k2 = k + 1 < n ? k + 1 : 1;
        Vec3 p = facePt(xy[k * 2], xy[k * 2 + 1], lift), q = facePt(xy[k2 * 2], xy[k2 * 2 + 1], lift);
        b.triSmooth({c, nc, col}, {p, headNormal(p - HEAD_C), col}, {q, headNormal(q - HEAD_C), col});
    }
}

// Elipse pintada
static void decalEllipse(MeshBuilder& b, float cx, float cy, float rx, float ry, float lift, Color col, int n = 14) {
    float xy[(16 + 2) * 2];
    n = n > 16 ? 16 : n;
    xy[0] = cx;
    xy[1] = cy;
    for (int k = 0; k <= n; k++) {
        xy[2 + k * 2] = cx + cosf(2 * PI * k / n) * rx;
        xy[3 + k * 2] = cy + sinf(2 * PI * k / n) * ry;
    }
    decalFan(b, xy, n + 2, lift, col);
}

// Trazo pintado a lo largo de una polilínea con grosor variable
static void decalStroke(MeshBuilder& b, const float* xy, const float* halfW, int n, float lift, Color col) {
    for (int k = 0; k + 1 < n; k++) {
        float dx = xy[k * 2 + 2] - xy[k * 2], dy = xy[k * 2 + 3] - xy[k * 2 + 1];
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 1e-5f) continue;
        float nx = -dy / len, ny = dx / len;
        Vec3 a0 = facePt(xy[k * 2] + nx * halfW[k], xy[k * 2 + 1] + ny * halfW[k], lift);
        Vec3 a1 = facePt(xy[k * 2] - nx * halfW[k], xy[k * 2 + 1] - ny * halfW[k], lift);
        Vec3 b0 = facePt(xy[k * 2 + 2] + nx * halfW[k + 1], xy[k * 2 + 3] + ny * halfW[k + 1], lift);
        Vec3 b1 = facePt(xy[k * 2 + 2] - nx * halfW[k + 1], xy[k * 2 + 3] - ny * halfW[k + 1], lift);
        Vec3 n = headNormal(a0 - HEAD_C);
        b.triSmooth({a0, n, col}, {a1, n, col}, {b1, n, col});
        b.triSmooth({a0, n, col}, {b1, n, col}, {b0, n, col});
    }
}

static void buildFace(MeshBuilder& b, const Appearance& a, bool blink) {
    Color skin = SKIN_COLORS[a.skin % NUM_SKINS];
    Color hair = HAIR_COLORS[a.hair % NUM_HAIRS];
    Color brow = a.hair == 4 ? shade(hair, 0.7f) : shade(hair, 0.75f);
    const float EY = -0.012f;
    for (int s = -1; s <= 1; s += 2) {
        const float ex = s * 0.074f;
        if (blink) {
            float line[5 * 2], w[5];
            for (int k = 0; k < 5; k++) {
                float t = k / 4.0f;
                line[k * 2] = ex + s * (-0.026f + 0.052f * t);
                line[k * 2 + 1] = EY - 0.004f + 0.009f * sinf(PI * t);
                w[k] = 0.004f;
            }
            decalStroke(b, line, w, 5, 0.003f, INK);
        } else {
            // Ojo de Mii: blanco ovalado pequeño, iris negro grande hacia dentro y la raya de arriba
            decalEllipse(b, ex, EY, 0.028f, 0.021f, 0.0015f, rgba(0xFC, 0xFC, 0xFC));
            decalEllipse(b, ex - s * 0.004f, EY + 0.001f, 0.0165f, 0.0195f, 0.003f, INK);
            decalEllipse(b, ex - s * 0.004f + 0.006f, EY + 0.008f, 0.0045f, 0.0045f, 0.0045f, UI_WHITE, 8);
            float lid[7 * 2], w[7];
            for (int k = 0; k < 7; k++) {
                float t = PI * (1 - k / 6.0f);  // de dentro a fuera por arriba
                lid[k * 2] = ex + s * cosf(t) * -0.03f;
                lid[k * 2 + 1] = EY + sinf(t) * 0.022f;
                w[k] = 0.0025f + 0.003f * k / 6.0f;
            }
            decalStroke(b, lid, w, 7, 0.0045f, INK);
        }
        // Ceja gruesa e inclinada: más baja hacia el centro
        float bx[4 * 2], bw[4];
        for (int k = 0; k < 4; k++) {
            float t = k / 3.0f;
            bx[k * 2] = ex + s * (-0.036f + 0.072f * t);
            bx[k * 2 + 1] = EY + 0.042f + 0.022f * t - 0.004f * t * t;
            bw[k] = 0.0085f - 0.004f * t;
        }
        decalStroke(b, bx, bw, 4, 0.002f, brow);
    }
    // Nariz: raya fina en gancho
    {
        const float nose[] = {0.004f, -0.03f, -0.006f, -0.055f, -0.012f, -0.075f, -0.006f, -0.088f, 0.008f, -0.089f};
        const float nw[] = {0.0025f, 0.003f, 0.0033f, 0.0033f, 0.0028f};
        decalStroke(b, nose, nw, 5, 0.002f, INK);
        decalEllipse(b, 0.008f, -0.078f, 0.012f, 0.009f, 0.001f, shade(skin, 0.93f), 10);
    }
    // Boca: una raya recta
    {
        const float mouth[] = {-0.036f, -0.135f, 0.0f, -0.136f, 0.036f, -0.135f};
        const float mw[] = {0.0028f, 0.0032f, 0.0028f};
        decalStroke(b, mouth, mw, 3, 0.002f, INK);
    }
}

static void buildHair(MeshBuilder& b, const Appearance& a, bool low) {
    Color hair = HAIR_COLORS[a.hair % NUM_HAIRS];
    Color hairDark = shade(hair, 0.86f);
    const Vec3 c = HEAD_C + Vec3(0, 0.012f, -0.004f);
    const float grow = 1.09f;
    int style = a.txapela ? 0 : a.style % NUM_HAIR_STYLES;
    // Hasta dónde baja el pelo según el lado (phi = 90° es la cara). Delante, flequillo en picos.
    auto cut = [style](float phi) {
        float fr = sinf(phi);  // 1 delante, −1 detrás
        float side = 1.78f, back = style == 2 || style == 3 ? 2.5f : 2.15f;
        float front = 0.98f;
        if (fr > 0.55f) {
            float u = (phi - PI / 2) / 0.6f;            // −1..1 a lo ancho de la frente
            float saw = fabsf(fmodf(u * 2.5f + 10.25f, 1.0f) - 0.5f) * 2;  // dientes de sierra
            front += 0.28f * (1 - saw) * (1 - fabsf(u) * 0.3f);
        }
        if (fr > 0) return lerpf(side, front, powf(fr, 1.3f));
        return lerpf(side, back, -fr);
    };
    headShell(b, c, grow, hair, cut, low ? 8 : 14, low ? 24 : 48);
    if (a.txapela) return;
    if (style == 1) {
        // De punta: mechones cónicos hacia arriba y atrás
        for (int k = 0; k < 7; k++) {
            float ang = -0.9f + k * 0.3f;
            Vec3 base = c + Vec3(sinf(ang) * 0.11f, 0.24f, cosf(ang) * 0.04f + 0.02f);
            Xf saved = b.xf;
            b.xf = saved * Xf().translate(base).rotateZ(-ang * 0.6f).rotateX(-0.5f - (k & 1) * 0.2f);
            b.cylinder(Vec3(0, 0, 0), 0.055f, 0.0f, 0.12f + (k & 1) * 0.03f, (k & 1) ? hairDark : hair, hair, 6, false);
            b.xf = saved;
        }
    }
    if (style == 2 || style == 3) {
        // Melena: faldón por los lados y detrás. 2 = larga hasta los hombros, 3 = media melena
        float bottom = style == 2 ? -0.36f : -0.2f;
        const int segs = low ? 12 : 20;
        for (int j = 0; j < segs; j++) {
            // de un lado de la cara al otro pasando por detrás (sin taparla: de 144° a 396°)
            float p0 = PI * 0.8f + PI * 1.4f * j / segs;
            float p1 = PI * 0.8f + PI * 1.4f * (j + 1) / segs;
            float rTop = 1.08f, rBot = style == 2 ? 1.0f : 1.1f;
            Vec3 d0(cosf(p0), 0, sinf(p0)), d1(cosf(p1), 0, sinf(p1));
            Vec3 t0 = c + Vec3(d0.x * HA * rTop, -0.02f, d0.z * HC * rTop), t1 = c + Vec3(d1.x * HA * rTop, -0.02f, d1.z * HC * rTop);
            Vec3 b0 = c + Vec3(d0.x * HA * rBot, bottom, d0.z * HC * rBot), b1 = c + Vec3(d1.x * HA * rBot, bottom, d1.z * HC * rBot);
            b.triSmooth({t0, d0, hair}, {b0, d0, hairDark}, {b1, d1, hairDark});
            b.triSmooth({t0, d0, hair}, {b1, d1, hairDark}, {t1, d1, hair});
            b.triSmooth({t0, -d0, hairDark}, {b1, -d1, hairDark}, {b0, -d0, hairDark});
            b.triSmooth({t0, -d0, hairDark}, {t1, -d1, hairDark}, {b1, -d1, hairDark});
        }
    }
}

// low: menos polígonos (el público, que siempre se ve de lejos)
static void buildHead(MeshBuilder& b, const Appearance& a, bool blink, bool low) {
    Color skin = SKIN_COLORS[a.skin % NUM_SKINS];
    headShell(b, HEAD_C, 1.0f, skin, [](float) { return PI; }, low ? 10 : 18, low ? 16 : 30);
    buildFace(b, a, blink);
    buildHair(b, a, low);
    if (a.txapela) {
        // Txapela negra ladeada con su "txertoa"
        const Color beret = rgba(0x1C, 0x1E, 0x26);
        Xf saved = b.xf;
        b.xf = saved * Xf().translate(HEAD_C + Vec3(0.0f, 0.24f, -0.01f)).rotateZ(0.16f).rotateX(-0.08f);
        b.sphere(Vec3(0, 0, 0), Vec3(0.26f, 0.075f, 0.24f), beret, 22, 8);
        b.cylinder(Vec3(0, 0.06f, 0), 0.013f, 0.008f, 0.04f, beret, beret, 6, true);
        b.xf = saved;
    }
}

// ------------------------------------------------------------------ cuerpo
// Sólido de revolución: perfil (y, radio) con un color por tramo; achatado en Z
static void lathe(MeshBuilder& b, const float* ys, const float* rs, const Color* cols, int n, float zs, int segs = 20) {
    for (int i = 0; i + 1 < n; i++) {
        float dr = rs[i + 1] - rs[i], dy = ys[i + 1] - ys[i];
        float l = sqrtf(dr * dr + dy * dy);
        if (l < 1e-5f) continue;
        float nr = dy / l, ny = -dr / l;
        for (int j = 0; j < segs; j++) {
            float a0 = 2 * PI * j / segs, a1 = 2 * PI * (j + 1) / segs;
            Vec3 d0(cosf(a0), 0, sinf(a0) * zs), d1(cosf(a1), 0, sinf(a1) * zs);
            Vec3 n0 = normalize(Vec3(cosf(a0) * nr, ny, sinf(a0) * nr / zs));
            Vec3 n1 = normalize(Vec3(cosf(a1) * nr, ny, sinf(a1) * nr / zs));
            Vec3 p00 = d0 * rs[i] + Vec3(0, ys[i], 0), p01 = d1 * rs[i] + Vec3(0, ys[i], 0);
            Vec3 p10 = d0 * rs[i + 1] + Vec3(0, ys[i + 1], 0), p11 = d1 * rs[i + 1] + Vec3(0, ys[i + 1], 0);
            Color c = cols[i];
            b.triSmooth({p00, n0, c}, {p10, n0, c}, {p11, n1, c});
            b.triSmooth({p00, n0, c}, {p11, n1, c}, {p01, n1, c});
        }
    }
}

// Mii para un aspecto del juego. Jugadores: un invitado de la Wii con su piel, pelo y color. Público: Miis
// aleatorios hechos con las piezas oficiales (siempre los mismos para el mismo aspecto).
static MiiInfo miiFor(const Appearance& a) {
    static const u8 SKIN_TO_MII[NUM_SKINS] = {0, 1, 2, 4};
    static const u8 HAIR_TO_MII[NUM_HAIRS] = {0, 1, 7, 2, 4};
    static const u8 STYLE_TO_MII[NUM_HAIR_STYLES] = {33, 55, 12, 24};
    // Un Mii del Canal Mii, tal cual
    if (a.user >= 0 && a.user < mii::userCount()) return mii::user(a.user);
    bool female = a.style >= 2;
    MiiInfo m;
    if (a.casual) {
        u32 seed = 2654435761u * (1u + a.color + 13u * a.skin + 131u * a.hair + 1031u * a.style + 7919u * a.txapela);
        m = mii::random(seed, female);
    } else {
        m = mii::guest(female ? 3 : 0);
        m.hairType = STYLE_TO_MII[a.style % NUM_HAIR_STYLES];
    }
    m.faceColor = SKIN_TO_MII[a.skin % NUM_SKINS];
    m.hairColor = HAIR_TO_MII[a.hair % NUM_HAIRS];
    m.browColor = m.hairColor;
    m.favColor = a.color % 12;
    return m;
}

void CharacterModel::build(const Appearance& a) {
    buildShared();
    release();
    look = a;
    bool low = a.casual != 0;
    MeshBuilder b;
    buildHead(b, a, false, low);
    b.build(head);
    b.clear();
    buildHead(b, a, true, low);
    b.build(headBlink);
    b.clear();

    Color shirt = FAV_COLORS[a.color % NUM_COLORS];
    Color skin = SKIN_COLORS[a.skin % NUM_SKINS];
    // Cabeza de Mii de verdad si la consola tiene las piezas (si no, se queda la nuestra)
    if (mii::available()) {
        MiiInfo info = miiFor(a);
        mii = new MiiHead();
        if (mii->build(info, low ? 64 : 256)) {
            skin = mii::skinColor(info.faceColor);
            if (a.user >= 0) shirt = mii::favoriteColor(info.favColor);  // la camiseta del color favorito del Mii
            if (a.txapela) {
                const Color c = rgba(0x1C, 0x1E, 0x26);
                b.sphere(Vec3(0, 0, 0), Vec3(0.27f, 0.075f, 0.25f), c, 22, 8);
                b.cylinder(Vec3(0, 0.06f, 0), 0.013f, 0.008f, 0.04f, c, c, 6, true);
                b.build(beret);
                b.clear();
            }
        } else {
            delete mii;
            mii = nullptr;
        }
    }
    Color pantsCol = a.casual ? PANTS_MII : PANTS_WHITE;
    // Cuerpo de Wii Sports Resort (con brazos y dedos) para los Miis de verdad
    if (mii && miibody::available(a.casual ? miibody::CROWD : miibody::PLAYER)) {
        const MiiInfo info = miiFor(a);
        const Color zones[4] = {skin, shirt, pantsCol, rgba(0x4A, 0x48, 0x4C)};
        body.build(a.casual ? miibody::CROWD : miibody::PLAYER, zones, info.height, info.build);
        if (!a.casual) {
            Color sashCol = (a.color == 0 || a.color == 7) ? rgba(0x22, 0x22, 0x2A) : rgba(0xC8, 0x1E, 0x1E);
            b.cylinder(Vec3(0, -0.04f, 0), 1.0f, 1.0f, 0.08f, sashCol, sashCol, 18, true);
            b.build(sash);
            b.clear();
        }
    }
    // Torso de Mii: estrecho, un poco más ancho en la cintura que en los hombros
    const float ys[] = {-0.03f, 0.0f, 0.12f, 0.26f, 0.37f, 0.44f, 0.475f, 0.48f};
    const float rs[] = {0.12f, 0.145f, 0.152f, 0.145f, 0.13f, 0.1f, 0.05f, 0.0f};
    const Color cs[] = {shirt, shirt, shirt, shirt, shirt, shirt, shirt, shirt};
    lathe(b, ys, rs, cs, 8, 0.74f, low ? 12 : 18);
    if (!a.casual) {
        Color sash = (a.color == 0 || a.color == 7) ? rgba(0x22, 0x22, 0x2A) : rgba(0xC8, 0x1E, 0x1E);
        b.cylinder(Vec3(0, -0.01f, 0), 0.152f, 0.148f, 0.08f, sash, sash, 18, true);
    }
    b.build(torso);
    b.clear();

    b.cylinder(Vec3(0, 0, 0), 0.052f, 0.056f, 1.0f, pantsCol, pantsCol, 8, false);
    b.build(legUnit);
    b.clear();
    b.sphere(Vec3(0, 0.0f, 0), Vec3(0.145f, 0.08f, 0.105f), pantsCol, 12, 6);
    b.build(pants);
    b.clear();

    // Brazo: manga larga del color de la camiseta (cilindro unitario a lo largo de +Y) y hombro redondeado
    b.cylinder(Vec3(0, 0, 0), 0.042f, 0.038f, 1.0f, shirt, shirt, 8, false);
    b.build(sleeve);
    b.clear();
    b.sphere(Vec3(0, 0, 0), Vec3(0.045f, 0.045f, 0.045f), shirt, 8, 5);
    b.build(shoulder);
    b.clear();

    // Mano: bola
    b.sphere(Vec3(0, 0, 0), Vec3(0.052f, 0.056f, 0.05f), skin, 10, 6);
    b.build(hand);
    built = true;
}

void CharacterModel::release() {
    if (mii) {
        mii->release();
        delete mii;
        mii = nullptr;
    }
    beret.release();
    body.release();
    sash.release();
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

// Mii con el cuerpo de Wii Sports Resort
static void drawResortCharacter(const CharacterModel& m, const CharacterPose& p) {
    miibody::Pose bp;
    bp.root = Xf().translate(p.pos).rotateY(p.yaw);
    bp.lean = p.lean;
    bp.side = p.side;
    bp.headPitch = p.headPitch;
    bp.squash = p.squash;
    bp.handL = p.handL;
    bp.handR = p.handR;
    bp.handsVisible = p.handsVisible;
    Xf head, waist;
    miibody::draw(m.body, bp, &head, &waist);
    if (m.sash.valid()) {
        float r = tuning::MII_SASH_UNITS * tuning::MII_BODY_UNIT * m.body.sxz;
        gfx::drawMesh(m.sash, waist * Xf().scale(Vec3(r, 1, r * tuning::MII_SASH_DEPTH)));
    }
    if (m.mii && m.mii->ready()) {
        Xf h = head * Xf().scale(miibody::headWidth() / 0.43f);
        m.mii->draw(h, p.blink);
        if (m.beret.valid()) gfx::drawMesh(m.beret, h * Xf().translate(Vec3(0, m.mii->top() - 0.035f, -0.01f)).rotateZ(0.16f));
    }
}

void drawCharacter(const CharacterModel& m, const CharacterPose& p) {
    if (!m.built) return;
    if (m.body.valid()) {
        drawResortCharacter(m, p);
        return;
    }
    Xf root = Xf().translate(p.pos).rotateY(p.yaw);
    float sq = clampv(p.squash, -0.3f, 0.3f);
    float hipY = HIP_Y * (1 - sq * 0.5f);
    const float stance = 0.075f;

    for (int s = -1; s <= 1; s += 2) {
        Vec3 foot(s * stance, 0, 0), hip(s * 0.07f, hipY, 0);
        Vec3 d = hip - foot;
        float len = length(d);
        gfx::drawMesh(m.legUnit, root * Xf().translate(foot + Vec3(0, 0.05f, 0)).rotateZ(-atan2f(d.x, d.y)).scale(Vec3(1, len - 0.05f, 1)));
        gfx::drawMesh(s_shoe, root * Xf().translate(foot));
    }
    Xf torso = characterTorsoXf(p);
    gfx::drawMesh(m.pants, root * Xf().translate(Vec3(0, hipY, 0)));
    gfx::drawMesh(m.torso, torso);
    Xf head = torso * Xf().translate(Vec3(0, NECK_Y, 0)).rotateX(p.headPitch);
    if (m.mii && m.mii->ready()) {
        m.mii->draw(head, p.blink);
        if (m.beret.valid()) gfx::drawMesh(m.beret, head * Xf().translate(Vec3(0, m.mii->top() - 0.035f, -0.01f)).rotateZ(0.16f));
    } else {
        gfx::drawMesh(p.blink ? m.headBlink : m.head, head);
    }
    if (p.handsVisible) {
        for (int s = -1; s <= 1; s += 2) {
            Vec3 sh = torso.apply(Vec3(s * SHOULDER.x, SHOULDER.y, SHOULDER.z));
            Vec3 hand = s < 0 ? p.handL : p.handR;
            gfx::drawMesh(m.shoulder, Xf().translate(sh));
            drawLimb(m.sleeve, sh, hand);
            gfx::drawMesh(m.hand, Xf().translate(hand));
        }
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
