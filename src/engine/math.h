#pragma once
#include <math.h>
#include <stdlib.h>

constexpr float PI = 3.14159265358979f;
constexpr float DEG = PI / 180.0f;

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() = default;
    constexpr Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    Vec3 operator-() const { return {-x, -y, -z}; }
    Vec3& operator+=(const Vec3& o) { x += o.x; y += o.y; z += o.z; return *this; }
    Vec3& operator-=(const Vec3& o) { x -= o.x; y -= o.y; z -= o.z; return *this; }
    Vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
};

inline float dot(const Vec3& a, const Vec3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vec3 cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
inline float length(const Vec3& v) { return sqrtf(dot(v, v)); }
inline Vec3 normalize(const Vec3& v) {
    float l = length(v);
    return l > 1e-6f ? v * (1.0f / l) : Vec3(0, 1, 0);
}

template <typename T> inline T clampv(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline Vec3 lerpv(const Vec3& a, const Vec3& b, float t) { return a + (b - a) * t; }
inline float smooth01(float t) { t = clampv(t, 0.0f, 1.0f); return t * t * (3 - 2 * t); }
inline float easeOutBack(float t) {
    const float c1 = 1.70158f, c3 = c1 + 1;
    t = clampv(t, 0.0f, 1.0f) - 1;
    return 1 + c3 * t * t * t + c1 * t * t;
}
inline float easeOutCubic(float t) { t = 1 - clampv(t, 0.0f, 1.0f); return 1 - t * t * t; }
// Acerca `cur` a `target` de forma suave e independiente del framerate
inline float approach(float cur, float target, float rate, float dt) {
    return target + (cur - target) * expf(-rate * dt);
}
inline Vec3 approachv(const Vec3& cur, const Vec3& target, float rate, float dt) {
    float k = expf(-rate * dt);
    return target + (cur - target) * k;
}
inline float frand() { return (float)rand() / (float)RAND_MAX; }
inline float frand(float lo, float hi) { return lo + (hi - lo) * frand(); }

// Colores RGBA empaquetados (0xRRGGBBAA)
typedef unsigned int Color;
constexpr Color rgba(int r, int g, int b, int a = 255) {
    return ((Color)r << 24) | ((Color)g << 16) | ((Color)b << 8) | (Color)a;
}
inline int colR(Color c) { return (c >> 24) & 0xFF; }
inline int colG(Color c) { return (c >> 16) & 0xFF; }
inline int colB(Color c) { return (c >> 8) & 0xFF; }
inline int colA(Color c) { return c & 0xFF; }
inline Color withAlpha(Color c, float a) { return (c & 0xFFFFFF00u) | (Color)clampv((int)(a * colA(c)), 0, 255); }
inline Color mixColor(Color a, Color b, float t) {
    return rgba((int)lerpf(colR(a), colR(b), t), (int)lerpf(colG(a), colG(b), t),
                (int)lerpf(colB(a), colB(b), t), (int)lerpf(colA(a), colA(b), t));
}
inline Color shade(Color c, float k) {
    return rgba(clampv((int)(colR(c) * k), 0, 255), clampv((int)(colG(c) * k), 0, 255),
                clampv((int)(colB(c) * k), 0, 255), colA(c));
}
