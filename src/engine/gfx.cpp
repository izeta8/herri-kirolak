#include "engine/gfx.h"
#include "engine/mesh.h"
#include <malloc.h>
#include <string.h>

#define FIFO_SIZE (512 * 1024)

// ------------------------------------------------------------------ Xf
Xf::Xf() { guMtxIdentity(m); }

Xf& Xf::translate(const Vec3& t) {
    Mtx tm;
    guMtxTrans(tm, t.x, t.y, t.z);
    guMtxConcat(m, tm, m);
    return *this;
}
Xf& Xf::rotateX(float rad) { Mtx r; guMtxRotRad(r, 'x', rad); guMtxConcat(m, r, m); return *this; }
Xf& Xf::rotateY(float rad) { Mtx r; guMtxRotRad(r, 'y', rad); guMtxConcat(m, r, m); return *this; }
Xf& Xf::rotateZ(float rad) { Mtx r; guMtxRotRad(r, 'z', rad); guMtxConcat(m, r, m); return *this; }
Xf& Xf::rotateAxis(const Vec3& axis, float rad) {
    Mtx r;
    guVector a = {axis.x, axis.y, axis.z};
    guMtxRotAxisRad(r, &a, rad);
    guMtxConcat(m, r, m);
    return *this;
}
Xf& Xf::scale(const Vec3& s) { Mtx sm; guMtxScale(sm, s.x, s.y, s.z); guMtxConcat(m, sm, m); return *this; }
Xf Xf::operator*(const Xf& o) const { Xf r; guMtxConcat((MtxP)m, (MtxP)o.m, r.m); return r; }
Xf Xf::fromBasis(const Vec3& o, const Vec3& x, const Vec3& y, const Vec3& z) {
    Xf r;
    r.m[0][0] = x.x; r.m[0][1] = y.x; r.m[0][2] = z.x; r.m[0][3] = o.x;
    r.m[1][0] = x.y; r.m[1][1] = y.y; r.m[1][2] = z.y; r.m[1][3] = o.y;
    r.m[2][0] = x.z; r.m[2][1] = y.z; r.m[2][2] = z.z; r.m[2][3] = o.z;
    return r;
}
Vec3 Xf::apply(const Vec3& p) const {
    return {m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z + m[0][3],
            m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z + m[1][3],
            m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z + m[2][3]};
}
Vec3 Xf::applyDir(const Vec3& p) const {
    return {m[0][0] * p.x + m[0][1] * p.y + m[0][2] * p.z,
            m[1][0] * p.x + m[1][1] * p.y + m[1][2] * p.z,
            m[2][0] * p.x + m[2][1] * p.y + m[2][2] * p.z};
}

namespace gfx {

static void* s_xfb[2];
static int s_fb = 0;
static GXRModeObj* s_rmode;
static bool s_wide = false;
static Mtx s_view;
static Mtx44 s_proj;

enum Mode { MODE_NONE, MODE_3D, MODE_2D, MODE_2D_TEX };
static Mode s_mode = MODE_NONE;
static bool s_lit = true;

bool widescreen() { return s_wide; }
float screenW() { return s_wide ? 854.0f : 640.0f; }
bool isPAL50() { return s_rmode && (s_rmode->viTVMode >> 2) == VI_PAL; }

void init() {
    VIDEO_Init();
    s_rmode = VIDEO_GetPreferredMode(nullptr);
    s_wide = CONF_GetAspectRatio() == CONF_ASPECT_16_9;
    if (s_wide) {
        s_rmode->viWidth = 678;
        s_rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678) / 2;
    }
    for (int i = 0; i < 2; i++) s_xfb[i] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(s_rmode));
    VIDEO_Configure(s_rmode);
    VIDEO_SetNextFramebuffer(s_xfb[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (s_rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    void* fifo = memalign(32, FIFO_SIZE);
    memset(fifo, 0, FIFO_SIZE);
    GX_Init(fifo, FIFO_SIZE);
    GX_SetCopyClear((GXColor){0x7C, 0xC4, 0xF4, 0xFF}, 0x00FFFFFF);

    f32 yscale = GX_GetYScaleFactor(s_rmode->efbHeight, s_rmode->xfbHeight);
    u32 xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetViewport(0, 0, s_rmode->fbWidth, s_rmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, s_rmode->fbWidth, s_rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, s_rmode->fbWidth, s_rmode->efbHeight);
    GX_SetDispCopyDst(s_rmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(s_rmode->aa, s_rmode->sample_pattern, GX_TRUE, s_rmode->vfilter);
    GX_SetFieldMode(s_rmode->field_rendering,
                    (s_rmode->viHeight == 2 * s_rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE);
    GX_SetPixelFmt(s_rmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_CopyDisp(s_xfb[0], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);

    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT1, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    GX_InvVtxCache();
    GX_InvalidateTexAll();
}

static void (*s_frameStart)() = nullptr;
static const GXColor CLEAR_COLOR = {0x7C, 0xC4, 0xF4, 0xFF};

void setFrameStartCallback(void (*cb)()) { s_frameStart = cb; }

void beginFrame() {
    GX_SetViewport(0, 0, s_rmode->fbWidth, s_rmode->efbHeight, 0, 1);
    s_mode = MODE_NONE;
    if (s_frameStart) {
        s_frameStart();
        s_mode = MODE_NONE;
    }
}

void offscreenBegin(u16 size) {
    static u8* scratch = nullptr;
    if (!scratch) scratch = (u8*)memalign(32, 256 * 256 * 2);
    GX_SetPixelFmt(GX_PF_RGBA6_Z24, GX_ZC_LINEAR);
    GX_SetCopyFilter(GX_FALSE, nullptr, GX_FALSE, nullptr);
    GX_SetViewport(0, 0, s_rmode->fbWidth, s_rmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, size, size);
    Mtx44 ortho;
    guOrtho(ortho, 0, s_rmode->efbHeight, 0, s_rmode->fbWidth, -1, 1);
    GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);
    GX_SetZMode(GX_FALSE, GX_LEQUAL, GX_FALSE);
    GX_SetCullMode(GX_CULL_NONE);
    GX_SetFog(GX_FOG_NONE, 0, 0, 0, 0, (GXColor){0, 0, 0, 0});
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_TRUE);
    // Borrar la esquina a (0,0,0,0) copiándola con borrado a un búfer de usar y tirar
    GX_SetCopyClear((GXColor){0, 0, 0, 0}, 0x00FFFFFF);
    GX_SetTexCopySrc(0, 0, size, size);
    GX_SetTexCopyDst(size, size, GX_TF_RGB5A3, GX_FALSE);
    DCInvalidateRange(scratch, size * size * 2);
    GX_CopyTex(scratch, GX_TRUE);
    GX_PixModeSync();
    s_mode = MODE_NONE;
}

void offscreenCopy(void* dst, u16 size, u8 texFmt) {
    GX_SetTexCopySrc(0, 0, size, size);
    GX_SetTexCopyDst(size, size, texFmt, GX_FALSE);
    DCInvalidateRange(dst, size * size * 2);
    GX_CopyTex(dst, GX_TRUE);
    GX_PixModeSync();
}

void offscreenEnd() {
    GX_DrawDone();
    GX_SetPixelFmt(s_rmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    GX_SetCopyFilter(s_rmode->aa, s_rmode->sample_pattern, GX_TRUE, s_rmode->vfilter);
    GX_SetCopyClear(CLEAR_COLOR, 0x00FFFFFF);
    GX_SetScissor(0, 0, s_rmode->fbWidth, s_rmode->efbHeight);
    GX_SetViewport(0, 0, s_rmode->fbWidth, s_rmode->efbHeight, 0, 1);
    GX_SetColorUpdate(GX_TRUE);
    GX_SetAlphaUpdate(GX_FALSE);
    GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GX_InvalidateTexAll();
    GX_InvVtxCache();
    s_mode = MODE_NONE;
}

void endFrame() {
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetColorUpdate(GX_TRUE);
    GX_CopyDisp(s_xfb[s_fb], GX_TRUE);
    GX_DrawDone();
    VIDEO_SetNextFramebuffer(s_xfb[s_fb]);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    s_fb ^= 1;
}

// ------------------------------------------------------------------ 3D
static void setChannels(bool lit) {
    GX_SetNumChans(1);
    if (lit)
        GX_SetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0 | GX_LIGHT1, GX_DF_CLAMP, GX_AF_NONE);
    else
        GX_SetChanCtrl(GX_COLOR0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
}

void use3DFormat() {
    if (s_mode == MODE_3D) return;
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetNumTexGens(0);
    GX_SetNumTevStages(1);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    s_mode = MODE_3D;
}

static void loadLight(u8 id, Vec3 dir, GXColor color) {
    dir = normalize(dir) * 100000.0f;
    guVector p = {dir.x, dir.y, dir.z};
    guVecMultiply(s_view, &p, &p);
    GXLightObj l;
    GX_InitLightPos(&l, p.x, p.y, p.z);
    GX_InitLightColor(&l, color);
    GX_InitLightSpot(&l, 0.0f, GX_SP_OFF);
    GX_InitLightDistAttn(&l, 1.0f, 1.0f, GX_DA_OFF);
    GX_LoadLightObj(&l, id);
}

void begin3D(const Camera& cam) {
    s_mode = MODE_NONE;
    use3DFormat();
    float aspect = s_wide ? 16.0f / 9.0f : 4.0f / 3.0f;
    guPerspective(s_proj, cam.fov, aspect, cam.znear, cam.zfar);
    GX_LoadProjectionMtx(s_proj, GX_PERSPECTIVE);
    guVector eye = {cam.eye.x, cam.eye.y, cam.eye.z};
    guVector up = {cam.up.x, cam.up.y, cam.up.z};
    guVector at = {cam.target.x, cam.target.y, cam.target.z};
    guLookAt(s_view, &eye, &up, &at);

    // Luz principal cálida desde arriba-delante y luz de relleno azulada desde el lado contrario
    loadLight(GX_LIGHT0, Vec3(0.45f, 1.0f, 0.65f), (GXColor){0xF0, 0xEC, 0xE2, 0xFF});
    loadLight(GX_LIGHT1, Vec3(-0.6f, 0.2f, -0.7f), (GXColor){0x44, 0x52, 0x70, 0xFF});
    GX_SetChanAmbColor(GX_COLOR0, (GXColor){0x6E, 0x74, 0x84, 0xFF});
    s_lit = true;
    setChannels(true);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GX_SetAlphaUpdate(GX_FALSE);
    // Nuestros modelos van en orden antihorario (estilo OpenGL); GX considera frontal el horario
    GX_SetCullMode(GX_CULL_FRONT);
}

void setLighting(bool lit) {
    if (lit == s_lit) return;
    s_lit = lit;
    setChannels(lit);
}

void setDepth(bool test, bool write) { GX_SetZMode(test ? GX_TRUE : GX_FALSE, GX_LEQUAL, write ? GX_TRUE : GX_FALSE); }
void setBlend(bool on) {
    if (on) GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    else GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
}
void setCull(bool on) { GX_SetCullMode(on ? GX_CULL_FRONT : GX_CULL_NONE); }

void setFog(bool on, float start, float end, Color c) {
    if (!on) {
        GX_SetFog(GX_FOG_NONE, 0, 0, 0, 0, (GXColor){0, 0, 0, 0});
        return;
    }
    GX_SetFog(GX_FOG_LIN, start, end, 0.1f, 400.0f,
              (GXColor){(u8)colR(c), (u8)colG(c), (u8)colB(c), 0xFF});
}

void drawMesh(const Mesh& mesh, const Xf& model) {
    if (!mesh.valid()) return;
    use3DFormat();
    Mtx mv, nrm;
    guMtxConcat(s_view, (MtxP)model.m, mv);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    guMtxInverse(mv, nrm);
    guMtxTranspose(nrm, nrm);
    GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
    mesh.call();
}

void loadModelMtx(const Xf& model, float normalScale) {
    Mtx mv, nrm;
    guMtxConcat(s_view, (MtxP)model.m, mv);
    GX_LoadPosMtxImm(mv, GX_PNMTX0);
    guMtxInverse(mv, nrm);
    guMtxTranspose(nrm, nrm);
    if (normalScale != 1.0f)
        for (int r = 0; r < 3; r++)
            for (int c = 0; c < 3; c++) nrm[r][c] *= normalScale;
    GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
}

void restore3D() {
    s_mode = MODE_NONE;
    use3DFormat();
    setChannels(s_lit);
    for (int i = 0; i < 3; i++) GX_SetTevSwapMode((u8)(GX_TEVSTAGE0 + i), GX_TEV_SWAP0, GX_TEV_SWAP0);
    GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    GX_SetAlphaCompare(GX_ALWAYS, 0, GX_AOP_AND, GX_ALWAYS, 0);
    GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
    GX_SetCullMode(GX_CULL_FRONT);
    GX_SetCurrentMtx(GX_PNMTX0);
}

bool project(const Vec3& p, float& sx, float& sy) {
    guVector v = {p.x, p.y, p.z};
    guVecMultiply(s_view, &v, &v);
    if (v.z > -0.01f) return false;
    float cx = s_proj[0][0] * v.x + s_proj[0][2] * v.z;
    float cy = s_proj[1][1] * v.y + s_proj[1][2] * v.z;
    float w = -v.z;
    sx = (cx / w * 0.5f + 0.5f) * screenW();
    sy = (0.5f - cy / w * 0.5f) * SCREEN_H;
    return true;
}

// ------------------------------------------------------------------ 2D
static void setup2D(bool textured) {
    Mode want = textured ? MODE_2D_TEX : MODE_2D;
    if (s_mode == want) return;
    if (s_mode != MODE_2D && s_mode != MODE_2D_TEX) {
        Mtx44 ortho;
        guOrtho(ortho, 0, SCREEN_H, 0, screenW(), 0.1f, 10.0f);
        GX_LoadProjectionMtx(ortho, GX_ORTHOGRAPHIC);
        Mtx id;
        guMtxIdentity(id);
        guMtxTransApply(id, id, 0, 0, -1.0f);
        GX_LoadPosMtxImm(id, GX_PNMTX0);
        GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
        GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
        GX_SetCullMode(GX_CULL_NONE);
        GX_SetFog(GX_FOG_NONE, 0, 0, 0, 0, (GXColor){0, 0, 0, 0});
        GX_SetNumChans(1);
        GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
        s_lit = false;
    }
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    if (textured) {
        GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
        GX_SetNumTexGens(1);
        GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
        GX_SetNumTevStages(1);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
        // color = mezcla(contorno, color de vértice, intensidad de la textura)
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_C1, GX_CC_RASC, GX_CC_TEXC, GX_CC_ZERO);
        GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
        // alfa = alfa de la textura × alfa de vértice
        GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_TEXA, GX_CA_RASA, GX_CA_ZERO);
        GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    } else {
        GX_SetNumTexGens(0);
        GX_SetNumTevStages(1);
        GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
        GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    }
    s_mode = want;
}

void begin2D() {
    s_mode = MODE_NONE;
    setup2D(false);
}

void clearDepth() {
    setup2D(false);
    // Un rectángulo a pantalla completa casi en el plano lejano que solo escribe profundidad
    GX_SetColorUpdate(GX_FALSE);
    GX_SetZMode(GX_TRUE, GX_ALWAYS, GX_TRUE);
    const float w = screenW(), z = -8.95f;  // con la traslación −1 de la vista queda a −9.95 (lejos = 10)
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    GX_Position3f32(0, 0, z); GX_Color1u32(0);
    GX_Position3f32(w, 0, z); GX_Color1u32(0);
    GX_Position3f32(w, SCREEN_H, z); GX_Color1u32(0);
    GX_Position3f32(0, SCREEN_H, z); GX_Color1u32(0);
    GX_End();
    GX_SetZMode(GX_FALSE, GX_ALWAYS, GX_FALSE);
    GX_SetColorUpdate(GX_TRUE);
}

static inline void v2(float x, float y, Color c) {
    GX_Position3f32(x, y, 0);
    GX_Color1u32(c);
}

void rect(float x, float y, float w, float h, Color c) { rectGrad(x, y, w, h, c, c); }

void rectGrad(float x, float y, float w, float h, Color top, Color bottom) {
    setup2D(false);
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    v2(x, y, top);
    v2(x + w, y, top);
    v2(x + w, y + h, bottom);
    v2(x, y + h, bottom);
    GX_End();
}

static const int CORNER_SEGS = 6;

void roundRectGrad(float x, float y, float w, float h, float r, Color top, Color bottom) {
    setup2D(false);
    r = fminf(r, fminf(w, h) * 0.5f);
    const int n = 4 * (CORNER_SEGS + 1);
    GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT1, n + 2);
    v2(x + w * 0.5f, y + h * 0.5f, mixColor(top, bottom, 0.5f));
    const float cxs[4] = {x + w - r, x + w - r, x + r, x + r};
    const float cys[4] = {y + r, y + h - r, y + h - r, y + r};
    float firstX = 0, firstY = 0;
    for (int corner = 0; corner < 4; corner++) {
        float a0 = -PI * 0.5f + corner * PI * 0.5f;
        for (int i = 0; i <= CORNER_SEGS; i++) {
            float a = a0 + (PI * 0.5f) * i / CORNER_SEGS;
            float px = cxs[corner] + cosf(a) * r, py = cys[corner] + sinf(a) * r;
            if (corner == 0 && i == 0) { firstX = px; firstY = py; }
            v2(px, py, mixColor(top, bottom, (py - y) / h));
        }
    }
    v2(firstX, firstY, mixColor(top, bottom, (firstY - y) / h));
    GX_End();
}

void roundRect(float x, float y, float w, float h, float r, Color c) { roundRectGrad(x, y, w, h, r, c, c); }

void circle(float cx, float cy, float r, Color c, int segs) {
    setup2D(false);
    GX_Begin(GX_TRIANGLEFAN, GX_VTXFMT1, segs + 2);
    v2(cx, cy, c);
    for (int i = 0; i <= segs; i++) {
        float a = 2 * PI * i / segs;
        v2(cx + cosf(a) * r, cy + sinf(a) * r, c);
    }
    GX_End();
}

void ring(float cx, float cy, float r0, float r1, Color c, int segs) {
    setup2D(false);
    GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT1, (segs + 1) * 2);
    for (int i = 0; i <= segs; i++) {
        float a = 2 * PI * i / segs;
        v2(cx + cosf(a) * r1, cy + sinf(a) * r1, c);
        v2(cx + cosf(a) * r0, cy + sinf(a) * r0, c);
    }
    GX_End();
}

void arc(float cx, float cy, float r0, float r1, float a0, float a1, Color c, int segs) {
    setup2D(false);
    GX_Begin(GX_TRIANGLESTRIP, GX_VTXFMT1, (segs + 1) * 2);
    for (int i = 0; i <= segs; i++) {
        float a = a0 + (a1 - a0) * i / segs;
        v2(cx + cosf(a) * r1, cy + sinf(a) * r1, c);
        v2(cx + cosf(a) * r0, cy + sinf(a) * r0, c);
    }
    GX_End();
}

void triangle(float x0, float y0, float x1, float y1, float x2, float y2, Color c) {
    setup2D(false);
    GX_Begin(GX_TRIANGLES, GX_VTXFMT1, 3);
    v2(x0, y0, c);
    v2(x1, y1, c);
    v2(x2, y2, c);
    GX_End();
}

void beginTextured(GXTexObj* tex, Color outline, bool outlined) {
    setup2D(true);
    GX_LoadTexObj(tex, GX_TEXMAP0);
    // Tabla de intercambio 1: el alfa de la textura se lee del canal rojo (= relleno, sin contorno)
    GX_SetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_RED);
    GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, outlined ? GX_TEV_SWAP0 : GX_TEV_SWAP1);
    GX_SetTevColor(GX_TEVREG1, (GXColor){(u8)colR(outline), (u8)colG(outline), (u8)colB(outline), 0xFF});
}

void texQuad(float x, float y, float w, float h, float u0, float v0, float u1, float v1, Color c) {
    GX_Begin(GX_QUADS, GX_VTXFMT1, 4);
    GX_Position3f32(x, y, 0); GX_Color1u32(c); GX_TexCoord2f32(u0, v0);
    GX_Position3f32(x + w, y, 0); GX_Color1u32(c); GX_TexCoord2f32(u1, v0);
    GX_Position3f32(x + w, y + h, 0); GX_Color1u32(c); GX_TexCoord2f32(u1, v1);
    GX_Position3f32(x, y + h, 0); GX_Color1u32(c); GX_TexCoord2f32(u0, v1);
    GX_End();
}

void endTextured() {
    GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    setup2D(false);
}

}  // namespace gfx
