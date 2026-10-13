#pragma once
// Mallas con color por vértice, compiladas a display lists de GX.
#include <vector>
#include "engine/gfx.h"

struct Vert {
    Vec3 p;
    Vec3 n;
    Color c;
};

class Mesh {
public:
    // Sin destructor a propósito: las mallas viven toda la partida o se liberan con release().
    void build(const std::vector<Vert>& tris);
    void release();
    bool valid() const { return dl_ != nullptr && size_ > 0; }
    void call() const;
    int vertexCount() const { return count_; }

private:
    void* dl_ = nullptr;
    unsigned size_ = 0;
    int count_ = 0;
};

// Construye triángulos (sin índices). Todas las primitivas usan la transformación actual `xf`
// y quedan orientadas hacia fuera (orden antihorario).
class MeshBuilder {
public:
    std::vector<Vert> v;
    Xf xf;

    void clear() { v.clear(); xf = Xf(); }
    // Triángulo plano; si su normal no apunta hacia `outward` se le da la vuelta
    void tri(Vec3 a, Vec3 b, Vec3 c, Color col, const Vec3& outward);
    void triFlat(Vec3 a, Vec3 b, Vec3 c, Color col);  // orden tal cual
    void quad(Vec3 a, Vec3 b, Vec3 c, Vec3 d, Color col, const Vec3& outward);
    void triSmooth(const Vert& a, const Vert& b, const Vert& c);

    void box(const Vec3& center, const Vec3& half, Color col);
    // Elipsoide; `cut` > -1 recorta la parte inferior (y normalizada < cut) para hacer semiesferas
    void sphere(const Vec3& center, const Vec3& radii, Color col, int segs = 16, int rings = 10, float cut = -1.0f,
                bool capCut = false);
    // Cilindro/cono a lo largo de +Y desde `base`
    void cylinder(const Vec3& base, float r0, float r1, float h, Color side, Color cap, int segs = 16, bool caps = true);
    void capsule(const Vec3& center, float r, float halfLen, Color col, int segs = 14);
    void disc(const Vec3& center, float r, Color col, int segs = 24);  // horizontal, mirando a +Y

    void build(Mesh& out) const { out.build(v); }

private:
    Vert tv(const Vec3& p, const Vec3& n, Color c) const;
};
