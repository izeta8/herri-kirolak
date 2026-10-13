#include "games/aizkolaritza/log.h"
#include "engine/tuning.h"
#include "game/palette.h"

static const float R = tuning::LOG_RADIUS;
static const float HALF_LEN = tuning::LOG_LENGTH * 0.5f;
static const float V_SLOPE = 1.0f;  // caras de la V a 45°
static const int SEGS = 40;

void Log::reset() {
    depth[0] = depth[1] = 0;
    splitT_ = -1;
    dirty_ = true;
}

void Log::addDepth(int side, float d) {
    depth[side] += d;
    dirty_ = true;
}

bool Log::cut() const { return total() >= 2 * R - 0.002f; }
float Log::progress() const { return clampv(total() / (2 * R), 0.0f, 1.0f); }
void Log::startSplit() { splitT_ = 0; }
Vec3 Log::center() const { return pos + Vec3(0, R, 0); }
float Log::topY() const { return pos.y + 2 * R; }

Vec3 Log::impactPoint(int side, float tilt) const {
    float s = side == 0 ? 1.0f : -1.0f;
    float apex = R - depth[side];
    return center() + Vec3(-tilt * 0.035f, R * 0.45f, s * (fmaxf(apex, -R * 0.4f) * 0.8f + 0.05f));
}

void Log::update(float dt) {
    if (splitT_ >= 0) splitT_ += dt;
}

// Punto de la superficie tallada. Devuelve true si está dentro de una V (madera clara)
static bool carve(float x, float a, const float depth[2], Vec3& out) {
    float y = R * cosf(a), z = R * sinf(a);
    bool carved = false;
    float apexF = R - depth[0];
    float limF = apexF + fabsf(x) * V_SLOPE;
    if (depth[0] > 0.0005f && z > limF) { z = limF; carved = true; }
    float apexB = -(R - depth[1]);
    float limB = apexB - fabsf(x) * V_SLOPE;
    if (depth[1] > 0.0005f && z < limB) { z = limB; carved = true; }
    out = Vec3(x, y, z);
    return carved;
}

void Log::rebuild() {
    // Anillos: muy juntos cerca del corte, separados hacia los extremos
    float xs[64];
    int n = 0;
    for (float x = 0; x < 0.42f; x += 0.014f) xs[n++] = x;
    for (float x = 0.48f; x < HALF_LEN; x += 0.09f) xs[n++] = x;
    xs[n++] = HALF_LEN;

    for (int h = 0; h < 2; h++) {
        float sign = h == 0 ? -1.0f : 1.0f;
        MeshBuilder b;
        // Recorremos x de menor a mayor para que el orden de los triángulos sea consistente
        for (int k = 0; k < n - 1; k++) {
            float x0 = h == 0 ? -xs[n - 1 - k] : xs[k];
            float x1 = h == 0 ? -xs[n - 2 - k] : xs[k + 1];
            for (int j = 0; j < SEGS; j++) {
                float a0 = 2 * PI * j / SEGS, a1 = 2 * PI * (j + 1) / SEGS;
                Vec3 p00, p10, p01, p11;
                int carved = carve(x0, a0, depth, p00) + carve(x1, a0, depth, p10) + carve(x0, a1, depth, p01) +
                             carve(x1, a1, depth, p11);
                Color c;
                if (carved >= 2) {
                    float t = clampv(fabsf(p00.x) / 0.3f, 0.0f, 1.0f);
                    c = mixColor(WOOD_LIGHT, WOOD_MID, 0.25f + 0.5f * t);
                } else {
                    c = ((j * 7 + k / 3) % 5 == 0) ? BARK_DARK : mixColor(BARK, BARK_DARK, ((j * 13) % 7) / 14.0f);
                }
                b.triFlat(p00, p11, p10, c);
                b.triFlat(p00, p01, p11, c);
            }
        }
        // Tapa del extremo con anillos de crecimiento
        float xe = sign * HALF_LEN;
        Vec3 out(sign, 0, 0);
        for (int ringI = 0; ringI < 3; ringI++) {
            float rr = R * (1.0f - ringI * 0.3f);
            Color c = ringI == 1 ? WOOD_MID : WOOD_LIGHT;
            float xo = xe + sign * 0.001f * ringI;
            for (int j = 0; j < SEGS; j++) {
                float a0 = 2 * PI * j / SEGS, a1 = 2 * PI * (j + 1) / SEGS;
                b.tri(Vec3(xo, 0, 0), Vec3(xo, rr * cosf(a0), rr * sinf(a0)), Vec3(xo, rr * cosf(a1), rr * sinf(a1)), c, out);
            }
        }
        b.build(half_[h]);
    }

    if (!stakes_.valid()) {
        MeshBuilder b;
        // Cuñas que sujetan el tronco
        for (int s = -1; s <= 1; s += 2)
            for (int e = -1; e <= 1; e += 2) {
                Vec3 base(e * (HALF_LEN - 0.2f), 0, s * (R + 0.05f));
                b.box(base + Vec3(0, 0.06f, 0), Vec3(0.05f, 0.06f, 0.06f), rgba(0x8A, 0x62, 0x3C));
            }
        b.build(stakes_);
    }
    dirty_ = false;
}

void Log::draw() {
    if (dirty_) rebuild();
    Vec3 c = center();
    gfx::drawMesh(stakes_, Xf().translate(pos));
    if (splitT_ < 0) {
        Xf xf = Xf().translate(c);
        gfx::drawMesh(half_[0], xf);
        gfx::drawMesh(half_[1], xf);
        return;
    }
    // Las dos mitades se separan y se inclinan hacia el corte
    float t = easeOutBack(clampv(splitT_ / 0.6f, 0.0f, 1.0f));
    for (int h = 0; h < 2; h++) {
        float s = h == 0 ? -1.0f : 1.0f;
        Xf xf = Xf()
                    .translate(c + Vec3(s * (0.1f * t), -0.03f * t, 0))
                    .translate(Vec3(s * HALF_LEN, -R, 0))
                    .rotateZ(s * 0.12f * t)
                    .translate(Vec3(-s * HALF_LEN, R, 0));
        gfx::drawMesh(half_[h], xf);
    }
}

// ------------------------------------------------------------------ astillas
void Chips::init() {
    MeshBuilder b;
    b.box(Vec3(0, 0, 0), Vec3(0.032f, 0.008f, 0.02f), WOOD_LIGHT);
    b.build(mesh_[0]);
    b.clear();
    b.box(Vec3(0, 0, 0), Vec3(0.022f, 0.006f, 0.014f), mixColor(WOOD_LIGHT, BARK, 0.35f));
    b.build(mesh_[1]);
    count_ = 0;
    next_ = 0;
}

void Chips::spawn(const Vec3& at, const Vec3& outward, int n, float energy) {
    for (int i = 0; i < n; i++) {
        Chip& c = chips_[next_];
        next_ = (next_ + 1) % MAX;
        if (count_ < MAX) count_++;
        c.p = at + Vec3(frand(-0.03f, 0.03f), frand(-0.02f, 0.02f), frand(-0.02f, 0.02f));
        Vec3 dir = normalize(outward * frand(0.6f, 1.2f) + Vec3(frand(-0.8f, 0.8f), frand(0.5f, 1.4f), 0));
        c.v = dir * (energy * frand(1.6f, 3.4f));
        c.axis = normalize(Vec3(frand(-1, 1), frand(-1, 1), frand(-1, 1)));
        c.ang = frand(0, 6.28f);
        c.angV = frand(8, 22);
        c.scale = frand(0.7f, 1.4f) * (0.6f + 0.5f * energy);
        c.kind = (i % 3 == 0) ? 1 : 0;
        c.rest = false;
    }
}

void Chips::update(float dt) {
    for (int i = 0; i < count_; i++) {
        Chip& c = chips_[i];
        if (c.rest) continue;
        c.v.y -= 9.8f * dt;
        c.p += c.v * dt;
        c.ang += c.angV * dt;
        if (c.p.y < 0.012f) {
            c.p.y = 0.012f;
            if (fabsf(c.v.y) < 0.6f) {
                c.rest = true;
                c.axis = Vec3(0, 1, 0);
            } else {
                c.v.y = -c.v.y * 0.3f;
                c.v.x *= 0.5f;
                c.v.z *= 0.5f;
                c.angV *= 0.5f;
            }
        }
    }
}

void Chips::draw() {
    for (int i = 0; i < count_; i++) {
        const Chip& c = chips_[i];
        gfx::drawMesh(mesh_[c.kind], Xf().translate(c.p).rotateAxis(c.axis, c.ang).scale(c.scale));
    }
}
