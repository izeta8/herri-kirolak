#include "engine/mesh.h"
#include <malloc.h>
#include <gccore.h>

void Mesh::build(const std::vector<Vert>& tris) {
    release();
    count_ = (int)tris.size();
    if (count_ < 3) return;
    const int perChunk = 65535 / 3 * 3;
    int chunks = (count_ + perChunk - 1) / perChunk;
    unsigned cap = ((unsigned)(chunks * 3 + count_ * 28) + 63) & ~31u;
    dl_ = memalign(32, cap);
    if (!dl_) return;
    DCInvalidateRange(dl_, cap);
    gfx::use3DFormat();
    GX_BeginDispList(dl_, cap);
    for (int start = 0; start < count_; start += perChunk) {
        int n = count_ - start < perChunk ? count_ - start : perChunk;
        GX_Begin(GX_TRIANGLES, GX_VTXFMT0, n);
        for (int i = start; i < start + n; i++) {
            const Vert& t = tris[i];
            GX_Position3f32(t.p.x, t.p.y, t.p.z);
            GX_Normal3f32(t.n.x, t.n.y, t.n.z);
            GX_Color1u32(t.c);
        }
        GX_End();
    }
    size_ = GX_EndDispList();
    if (size_ == 0) {  // no cabía
        free(dl_);
        dl_ = nullptr;
    }
}

void Mesh::release() {
    if (dl_) free(dl_);
    dl_ = nullptr;
    size_ = 0;
    count_ = 0;
}

void Mesh::call() const { GX_CallDispList(dl_, size_); }

// ------------------------------------------------------------------ builder
Vert MeshBuilder::tv(const Vec3& p, const Vec3& n, Color c) const {
    return Vert{xf.apply(p), normalize(xf.applyDir(n)), c};
}

void MeshBuilder::triFlat(Vec3 a, Vec3 b, Vec3 c, Color col) {
    a = xf.apply(a); b = xf.apply(b); c = xf.apply(c);
    Vec3 n = normalize(cross(b - a, c - a));
    v.push_back({a, n, col});
    v.push_back({b, n, col});
    v.push_back({c, n, col});
}

void MeshBuilder::tri(Vec3 a, Vec3 b, Vec3 c, Color col, const Vec3& outward) {
    // Se decide en espacio de mundo para que funcione también con transformaciones espejo
    a = xf.apply(a); b = xf.apply(b); c = xf.apply(c);
    Vec3 n = cross(b - a, c - a);
    if (dot(n, xf.applyDir(outward)) < 0) { std::swap(b, c); n = -n; }
    n = normalize(n);
    v.push_back({a, n, col});
    v.push_back({b, n, col});
    v.push_back({c, n, col});
}

void MeshBuilder::quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color col, const Vec3& outward) {
    tri(a, b, c, col, outward);
    tri(a, c, d, col, outward);
}

void MeshBuilder::triSmooth(const Vert& a, const Vert& b, const Vert& c) {
    Vert A = tv(a.p, a.n, a.c), B = tv(b.p, b.n, b.c), C = tv(c.p, c.n, c.c);
    Vec3 fn = cross(B.p - A.p, C.p - A.p);
    if (dot(fn, A.n + B.n + C.n) < 0) std::swap(B, C);
    v.push_back(A);
    v.push_back(B);
    v.push_back(C);
}

void MeshBuilder::box(const Vec3& c, const Vec3& h, Color col) {
    Vec3 p[8];
    for (int i = 0; i < 8; i++)
        p[i] = Vec3(c.x + ((i & 1) ? h.x : -h.x), c.y + ((i & 2) ? h.y : -h.y), c.z + ((i & 4) ? h.z : -h.z));
    quad(p[0], p[1], p[3], p[2], col, Vec3(0, 0, -1));
    quad(p[4], p[5], p[7], p[6], col, Vec3(0, 0, 1));
    quad(p[0], p[1], p[5], p[4], col, Vec3(0, -1, 0));
    quad(p[2], p[3], p[7], p[6], col, Vec3(0, 1, 0));
    quad(p[0], p[2], p[6], p[4], col, Vec3(-1, 0, 0));
    quad(p[1], p[3], p[7], p[5], col, Vec3(1, 0, 0));
}

void MeshBuilder::sphere(const Vec3& c, const Vec3& r, Color col, int segs, int rings, float cut, bool capCut) {
    auto point = [&](float u, float yn, Vec3& pos, Vec3& nrm) {
        float s = sqrtf(fmaxf(0.0f, 1 - yn * yn));
        Vec3 unit(cosf(u) * s, yn, sinf(u) * s);
        pos = Vec3(c.x + unit.x * r.x, c.y + unit.y * r.y, c.z + unit.z * r.z);
        nrm = normalize(Vec3(unit.x / r.x, unit.y / r.y, unit.z / r.z));
    };
    float th0 = acosf(clampv(cut, -1.0f, 1.0f));  // ángulo polar del corte
    for (int i = 0; i < rings; i++) {
        float t0 = th0 * i / rings, t1 = th0 * (i + 1) / rings;
        float y0 = cosf(t0), y1 = cosf(t1);
        for (int j = 0; j < segs; j++) {
            float u0 = 2 * PI * j / segs, u1 = 2 * PI * (j + 1) / segs;
            Vert a, b, cc, d;
            point(u0, y0, a.p, a.n);
            point(u1, y0, b.p, b.n);
            point(u1, y1, cc.p, cc.n);
            point(u0, y1, d.p, d.n);
            a.c = b.c = cc.c = d.c = col;
            if (i > 0) triSmooth(a, b, cc);
            triSmooth(a, cc, d);
        }
    }
    if (capCut && cut > -1.0f) {
        float yc = c.y + cut * r.y;
        float s = sqrtf(1 - cut * cut);
        for (int j = 0; j < segs; j++) {
            float u0 = 2 * PI * j / segs, u1 = 2 * PI * (j + 1) / segs;
            tri(Vec3(c.x, yc, c.z), Vec3(c.x + cosf(u0) * s * r.x, yc, c.z + sinf(u0) * s * r.z),
                Vec3(c.x + cosf(u1) * s * r.x, yc, c.z + sinf(u1) * s * r.z), col, Vec3(0, -1, 0));
        }
    }
}

void MeshBuilder::cylinder(const Vec3& b, float r0, float r1, float h, Color side, Color cap, int segs, bool caps) {
    float slope = (r0 - r1) / h;
    for (int j = 0; j < segs; j++) {
        float u0 = 2 * PI * j / segs, u1 = 2 * PI * (j + 1) / segs;
        Vec3 d0(cosf(u0), 0, sinf(u0)), d1(cosf(u1), 0, sinf(u1));
        Vert a{b + d0 * r0, normalize(d0 + Vec3(0, slope, 0)), side};
        Vert bb{b + d1 * r0, normalize(d1 + Vec3(0, slope, 0)), side};
        Vert c{b + d1 * r1 + Vec3(0, h, 0), normalize(d1 + Vec3(0, slope, 0)), side};
        Vert d{b + d0 * r1 + Vec3(0, h, 0), normalize(d0 + Vec3(0, slope, 0)), side};
        triSmooth(a, bb, c);
        triSmooth(a, c, d);
        if (caps) {
            if (r1 > 0) tri(b + Vec3(0, h, 0), d.p, c.p, cap, Vec3(0, 1, 0));
            if (r0 > 0) tri(b, a.p, bb.p, cap, Vec3(0, -1, 0));
        }
    }
}

void MeshBuilder::capsule(const Vec3& c, float r, float halfLen, Color col, int segs) {
    cylinder(c - Vec3(0, halfLen, 0), r, r, halfLen * 2, col, col, segs, false);
    Xf saved = xf;
    sphere(c + Vec3(0, halfLen, 0), Vec3(r, r, r), col, segs, segs / 4, 0.0f);
    // semiesfera inferior: esfera recortada y volteada
    xf = saved * Xf().translate(c - Vec3(0, halfLen, 0)).scale(Vec3(1, -1, 1));
    sphere(Vec3(0, 0, 0), Vec3(r, r, r), col, segs, segs / 4, 0.0f);
    xf = saved;
}

void MeshBuilder::disc(const Vec3& c, float r, Color col, int segs) {
    for (int j = 0; j < segs; j++) {
        float u0 = 2 * PI * j / segs, u1 = 2 * PI * (j + 1) / segs;
        tri(c, c + Vec3(cosf(u0) * r, 0, sinf(u0) * r), c + Vec3(cosf(u1) * r, 0, sinf(u1) * r), col, Vec3(0, 1, 0));
    }
}
