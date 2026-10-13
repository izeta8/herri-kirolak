// Herri Kirolak — Fase 1: hola mundo 3D (cubo girando con GX).
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#define FIFO_SIZE (256 * 1024)

static void* g_xfb[2] = {nullptr, nullptr};
static GXRModeObj* g_rmode = nullptr;

static void initVideo() {
    VIDEO_Init();
    g_rmode = VIDEO_GetPreferredMode(nullptr);

    // Respeta el ajuste 16:9 de la Wii
    if (CONF_GetAspectRatio() == CONF_ASPECT_16_9) {
        g_rmode->viWidth = 678;
        g_rmode->viXOrigin = (VI_MAX_WIDTH_NTSC - 678) / 2;
    }

    for (int i = 0; i < 2; i++)
        g_xfb[i] = MEM_K0_TO_K1(SYS_AllocateFramebuffer(g_rmode));

    VIDEO_Configure(g_rmode);
    VIDEO_SetNextFramebuffer(g_xfb[0]);
    VIDEO_SetBlack(false);
    VIDEO_Flush();
    VIDEO_WaitVSync();
    if (g_rmode->viTVMode & VI_NON_INTERLACE) VIDEO_WaitVSync();

    void* fifo = memalign(32, FIFO_SIZE);
    memset(fifo, 0, FIFO_SIZE);
    GX_Init(fifo, FIFO_SIZE);

    // Cielo azul estilo Wii Sports
    GXColor sky = {0x7C, 0xC4, 0xF4, 0xFF};
    GX_SetCopyClear(sky, 0x00FFFFFF);

    f32 yscale = GX_GetYScaleFactor(g_rmode->efbHeight, g_rmode->xfbHeight);
    u32 xfbHeight = GX_SetDispCopyYScale(yscale);
    GX_SetViewport(0, 0, g_rmode->fbWidth, g_rmode->efbHeight, 0, 1);
    GX_SetScissor(0, 0, g_rmode->fbWidth, g_rmode->efbHeight);
    GX_SetDispCopySrc(0, 0, g_rmode->fbWidth, g_rmode->efbHeight);
    GX_SetDispCopyDst(g_rmode->fbWidth, xfbHeight);
    GX_SetCopyFilter(g_rmode->aa, g_rmode->sample_pattern, GX_TRUE, g_rmode->vfilter);
    GX_SetFieldMode(g_rmode->field_rendering,
                    (g_rmode->viHeight == 2 * g_rmode->xfbHeight) ? GX_ENABLE : GX_DISABLE);
    GX_SetPixelFmt(g_rmode->aa ? GX_PF_RGB565_Z16 : GX_PF_RGB8_Z24, GX_ZC_LINEAR);
    // GX toma como cara frontal el orden horario; nuestros modelos van en antihorario (estilo OpenGL)
    GX_SetCullMode(GX_CULL_FRONT);
    GX_CopyDisp(g_xfb[0], GX_TRUE);
    GX_SetDispCopyGamma(GX_GM_1_0);
}

static void setupCubePipeline() {
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT0, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);

    // Luz difusa + ambiente, color de material = color de vértice
    GX_SetNumChans(1);
    GX_SetNumTexGens(0);
    GX_SetChanCtrl(GX_COLOR0A0, GX_ENABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHT0, GX_DF_CLAMP, GX_AF_NONE);
    GX_SetChanAmbColor(GX_COLOR0A0, (GXColor){0x60, 0x60, 0x70, 0xFF});
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevOp(GX_TEVSTAGE0, GX_PASSCLR);
    GX_SetNumTevStages(1);
}

static void setLight(Mtx view) {
    guVector dir = {-0.4f, 0.8f, 0.6f};
    guVecNormalize(&dir);
    guVector pos = {dir.x * 10000.0f, dir.y * 10000.0f, dir.z * 10000.0f};
    guVecMultiply(view, &pos, &pos);
    GXLightObj light;
    GX_InitLightPos(&light, pos.x, pos.y, pos.z);
    GX_InitLightColor(&light, (GXColor){0xFF, 0xFF, 0xF0, 0xFF});
    GX_LoadLightObj(&light, GX_LIGHT0);
}

static void face(float nx, float ny, float nz, u32 rgba, const float v[4][3]) {
    for (int i = 0; i < 4; i++) {
        GX_Position3f32(v[i][0], v[i][1], v[i][2]);
        GX_Normal3f32(nx, ny, nz);
        GX_Color1u32(rgba);
    }
}

static void drawCube() {
    const float s = 1.0f;
    GX_Begin(GX_QUADS, GX_VTXFMT0, 24);
    { const float v[4][3] = {{-s, s, s}, {-s,-s, s}, { s,-s, s}, { s, s, s}}; face( 0, 0, 1, 0xE83A3AFFu, v); } // gorria
    { const float v[4][3] = {{ s, s,-s}, { s,-s,-s}, {-s,-s,-s}, {-s, s,-s}}; face( 0, 0,-1, 0x2E9E44FFu, v); } // berdea
    { const float v[4][3] = {{-s, s,-s}, {-s, s, s}, { s, s, s}, { s, s,-s}}; face( 0, 1, 0, 0xFFFFFFFF, v); }  // zuria
    { const float v[4][3] = {{-s,-s, s}, {-s,-s,-s}, { s,-s,-s}, { s,-s, s}}; face( 0,-1, 0, 0xFFFFFFFF, v); }
    { const float v[4][3] = {{ s, s, s}, { s,-s, s}, { s,-s,-s}, { s, s,-s}}; face( 1, 0, 0, 0xE83A3AFFu, v); }
    { const float v[4][3] = {{-s, s,-s}, {-s,-s,-s}, {-s,-s, s}, {-s, s, s}}; face(-1, 0, 0, 0x2E9E44FFu, v); }
    GX_End();
}

int main(int, char**) {
    initVideo();
    WPAD_Init();
    setupCubePipeline();

    Mtx44 proj;
    guPerspective(proj, 45.0f, (CONF_GetAspectRatio() == CONF_ASPECT_16_9) ? 16.0f / 9.0f : 4.0f / 3.0f, 0.1f, 300.0f);
    GX_LoadProjectionMtx(proj, GX_PERSPECTIVE);

    Mtx view;
    guVector cam = {0.0f, 2.0f, 6.0f}, up = {0.0f, 1.0f, 0.0f}, look = {0.0f, 0.0f, 0.0f};
    guLookAt(view, &cam, &up, &look);

    float angle = 0.0f;
    int fb = 0;
    while (true) {
        WPAD_ScanPads();
        if (WPAD_ButtonsDown(0) & WPAD_BUTTON_HOME) break;

        GX_SetViewport(0, 0, g_rmode->fbWidth, g_rmode->efbHeight, 0, 1);
        setLight(view);

        Mtx model, rx, ry, mv, nrm;
        guMtxRotDeg(ry, 'y', angle);
        guMtxRotDeg(rx, 'x', angle * 0.5f);
        guMtxConcat(ry, rx, model);
        guMtxConcat(view, model, mv);
        GX_LoadPosMtxImm(mv, GX_PNMTX0);
        guMtxInverse(mv, nrm);
        guMtxTranspose(nrm, nrm);
        GX_LoadNrmMtxImm(nrm, GX_PNMTX0);
        drawCube();

        GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);
        GX_SetColorUpdate(GX_TRUE);
        GX_CopyDisp(g_xfb[fb], GX_TRUE);
        GX_DrawDone();
        VIDEO_SetNextFramebuffer(g_xfb[fb]);
        VIDEO_Flush();
        VIDEO_WaitVSync();
        fb ^= 1;

        angle += 1.0f;
    }
    return 0;  // libogc vuelve al Homebrew Channel
}
