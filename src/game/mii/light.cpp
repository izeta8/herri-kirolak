#include "game/mii/light.h"
#include <malloc.h>
#include <string.h>
#include "engine/tuning.h"

namespace miilight {
namespace {

constexpr int N = tuning::MII_LM_SIZE;
constexpr int GRAD = 64;    // los degradados de EGG son texturas de 64 de ancho
constexpr int SLOTS = 4;    // juegos de lightmaps en rueda: la GPU puede estar leyendo el anterior
constexpr u32 TEX_BYTES = N * N * 2;

float s_grad[10][GRAD];     // degradados 0..9 de EGG::LightTexture::initialize
Vec3 s_nrm[N * N];          // normal de cada texel (la esfera de EGG::CpuTexture::fillNormalMapSphere)
u16* s_tex[SLOTS][2];
GXTexObj s_obj[SLOTS][2];
int s_slot = -1;
float s_lastView[9];

// EGG makeGradient: negro hasta `start`, rampa con la curva `op` y blanco desde `end`
void makeGradient(float* g, int op, int start, int end) {
    for (int i = 0; i < GRAD; i++) g[i] = i <= start ? 0.0f : (i >= end - 1 ? 1.0f : 0.0f);
    for (int i = start + 1; i < end - 1; i++) {
        float r = (float)(i - start) / (float)(end - start);
        switch (op) {
            case 1: r = r * r; break;
            case 2: r = r * r; r = r * r; break;
            case 3: r = r * r; r = r * r; r = r * r; break;
            case 4: r = r * r; r = r * r; r = r * r; r = r * r; break;
            case 5: r = 1.0f - (r - 1.0f) * (r - 1.0f); break;
            default: break;
        }
        g[i] = r;
    }
}

// Valor del degradado para n·L. EGG desplaza la coordenada con una textura indirecta (0.485·n·L) sobre un degradado de
// 64 texels girado media vuelta: el texel 32 + 31·(n·L), con filtro lineal y repetición
float sample(int gi, float d) {
    float x = 31.5f + 31.04f * d;
    int i0 = (int)floorf(x);
    float f = x - (float)i0;
    const float* g = s_grad[gi];
    return g[i0 & (GRAD - 1)] * (1 - f) + g[(i0 + 1) & (GRAD - 1)] * f;
}

// RGB565 en bloques de 4x4
void put(u16* tex, int x, int y, float r, float g, float b) {
    int ri = clampv((int)(r * 31.0f + 0.5f), 0, 31), gi = clampv((int)(g * 63.0f + 0.5f), 0, 63),
        bi = clampv((int)(b * 31.0f + 0.5f), 0, 31);
    tex[((y >> 2) * (N >> 2) + (x >> 2)) * 16 + (y & 3) * 4 + (x & 3)] = (u16)((ri << 11) | (gi << 5) | bi);
}

Vec3 rot(const Mtx v, const Vec3& d) {
    return Vec3(v[0][0] * d.x + v[0][1] * d.y + v[0][2] * d.z, v[1][0] * d.x + v[1][1] * d.y + v[1][2] * d.z,
                v[2][0] * d.x + v[2][1] * d.y + v[2][2] * d.z);
}

// EGG::LightTexture::draw de lm0 y lm1 con la cámara `view`
void paint(const Mtx view, u16* lm0, u16* lm1) {
    // Hacia dónde alumbra cada luz en el espacio de la cámara: el texel suma la luz con n·(-dir)
    constexpr int NL = sizeof(tuning::MII_LIGHTS) / sizeof(tuning::MII_LIGHTS[0]);
    Vec3 toLight[NL];
    Vec3 camZ(view[2][0], view[2][1], view[2][2]);  // eje Z de la cámara en el mundo (hacia quien mira)
    for (int i = 0; i < NL; i++) {
        const tuning::MiiLight& l = tuning::MII_LIGHTS[i];
        Vec3 dir = normalize(Vec3(-l.x, -l.y, -l.z));
        switch (l.type) {
            case 1: toLight[i] = rot(view, normalize(camZ - dir)); break;  // medio ángulo entre la luz y la cámara
            case 2: toLight[i] = -dir; break;
            case 4: toLight[i] = Vec3(0, 0, -1); break;                     // contraluz: alumbra el borde
            default: toLight[i] = rot(view, -dir); break;
        }
    }
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            const Vec3& n = s_nrm[y * N + x];
            float c[2][3] = {{0, 0, 0}, {0, 0, 0}};
            for (int i = 0; i < NL; i++) {
                const tuning::MiiLight& l = tuning::MII_LIGHTS[i];
                float k = sample(l.gradient, dot(n, toLight[i])) * l.intensity / 255.0f;
                float* o = c[l.map & 1];
                o[0] += k * l.r;
                o[1] += k * l.g;
                o[2] += k * l.b;
            }
            put(lm0, x, y, c[0][0], c[0][1], c[0][2]);
            put(lm1, x, y, c[1][0], c[1][1], c[1][2]);
        }
}

}  // namespace

void init() {
    if (s_tex[0][0]) return;
    // Degradados de EGG: curva y tramo [a+1, b-1] de cada uno
    static const int OP[10] = {0, 0, 0, 1, 2, 3, 4, 0, 5, 5};
    static const int A[10] = {32, 0, 16, 32, 32, 32, 32, 4, 10, 13};
    static const int B[10] = {64, 64, 64, 64, 64, 64, 64, 36, 42, 45};
    for (int i = 0; i < 10; i++) makeGradient(s_grad[i], OP[i], A[i] + 1, B[i] - 1);
    const float mid = N / 2.0f;
    for (int y = 0; y < N; y++)
        for (int x = 0; x < N; x++) {
            float fx = (x + 0.5f) - mid, fy = -((y + 0.5f) - mid);
            float fz = mid * mid - (fx * fx + fy * fy);
            s_nrm[y * N + x] = normalize(Vec3(fx, fy, fz > 0 ? sqrtf(fz) : 0.0f));
        }
    for (int s = 0; s < SLOTS; s++)
        for (int k = 0; k < 2; k++) {
            s_tex[s][k] = (u16*)memalign(32, TEX_BYTES);
            memset(s_tex[s][k], 0, TEX_BYTES);
            GX_InitTexObj(&s_obj[s][k], s_tex[s][k], N, N, GX_TF_RGB565, GX_CLAMP, GX_CLAMP, GX_FALSE);
            GX_InitTexObjFilterMode(&s_obj[s][k], GX_LINEAR, GX_LINEAR);
        }
}

void bind(bool vertexAlpha) {
    if (!s_tex[0][0]) return;
    MtxP view = gfx::viewMtx();
    float v[9] = {view[0][0], view[0][1], view[0][2], view[1][0], view[1][1], view[1][2], view[2][0], view[2][1], view[2][2]};
    if (s_slot < 0 || memcmp(v, s_lastView, sizeof(v)) != 0) {
        s_slot = (s_slot + 1) % SLOTS;
        memcpy(s_lastView, v, sizeof(v));
        paint(view, s_tex[s_slot][0], s_tex[s_slot][1]);
        DCFlushRange(s_tex[s_slot][0], TEX_BYTES);
        DCFlushRange(s_tex[s_slot][1], TEX_BYTES);
        GX_InvalidateTexAll();
    }
    GX_LoadTexObj(&s_obj[s_slot][0], GX_TEXMAP1);
    GX_LoadTexObj(&s_obj[s_slot][1], GX_TEXMAP2);

    // Normal en cámara (GX_TEXMTX0, cargada con cada modelo) → (0.5 + 0.5x, 0.5 - 0.5y), como el "env camera" de NW4R
    Mtx post = {{0.5f, 0, 0, 0.5f}, {0, -0.5f, 0, 0.5f}, {0, 0, 0, 1.0f}};
    GX_LoadTexMtxImm(post, GX_DTTMTX0, GX_MTX3x4);
    GX_SetTexCoordGen2(GX_TEXCOORD1, GX_TG_MTX3x4, GX_TG_NRM, GX_TEXMTX0, GX_TRUE, GX_DTTMTX0);

    // RASC = ambiente (luces de GX apagadas, como en el lightset de los Miis); RASA = alfa del vértice (oclusión)
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_REG, vertexAlpha ? GX_SRC_VTX : GX_SRC_REG, GX_LIGHTNULL, GX_DF_NONE,
                   GX_AF_NONE);
    const u8 a = (u8)tuning::MII_AMBIENT;
    GX_SetChanMatColor(GX_COLOR0A0, (GXColor){a, a, a, 255});
}

void loadModelMtx(const Xf& model, float normalScale) {
    gfx::loadModelMtx(model, normalScale);
    Mtx mv, nrm;
    guMtxConcat(gfx::viewMtx(), (MtxP)model.m, mv);
    guMtxInverse(mv, nrm);
    guMtxTranspose(nrm, nrm);
    nrm[0][3] = nrm[1][3] = nrm[2][3] = 0;
    GX_LoadTexMtxImm(nrm, GX_TEXMTX0, GX_MTX3x4);
}

}  // namespace miilight
