#pragma once
// Tronco tumbado (eje X) con dos cortes en V: uno por delante (+Z) y otro por detrás (-Z).
#include "engine/mesh.h"

class Log {
public:
    Vec3 pos{0, 0, 0};  // punto del suelo bajo el centro del tronco
    float depth[2] = {0, 0};  // profundidad de cada V (m): 0 = delante (+Z), 1 = detrás (-Z)

    void reset();
    void addDepth(int side, float d);
    float total() const { return depth[0] + depth[1]; }
    bool cut() const;
    float progress() const;  // 0..1
    void startSplit();
    bool splitting() const { return splitT_ >= 0; }
    float splitTime() const { return splitT_; }
    void update(float dt);
    void draw();
    // Punto donde golpea el hacha en el lado `side` (mundo)
    Vec3 impactPoint(int side, float tilt) const;
    float topY() const;
    Vec3 center() const;

private:
    void rebuild();
    Mesh half_[2];
    Mesh stakes_;
    bool dirty_ = true;
    float splitT_ = -1;
};

// Astillas que salen volando con cada hachazo
class Chips {
public:
    void init();
    void spawn(const Vec3& at, const Vec3& outward, int n, float energy);
    void update(float dt);
    void draw();
    void clear() { count_ = 0; }

private:
    struct Chip {
        Vec3 p, v, axis;
        float ang, angV, scale;
        int kind;
        bool rest;
    };
    static const int MAX = 180;
    Chip chips_[MAX];
    int count_ = 0, next_ = 0;
    Mesh mesh_[2];
};
