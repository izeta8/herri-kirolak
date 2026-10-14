#include "game/mii/mii.h"
#include <malloc.h>
#include <string.h>
#include "engine/nand.h"

// Montaje de la cabeza basado en cómo lo hace la librería de Miis de la Wii (RVLFaceLib), documentada por la
// decompilación pública de Super Mario Galaxy (SMGCommunity/Petari). Las piezas (modelos y texturas) NO están en
// este repositorio: se leen en tiempo de ejecución de la NAND de la consola del usuario.

namespace {

const char* RES_PATH = "/shared2/FaceLib/RFL_Res.dat";
const char* DB_PATH = "/shared2/menu/FaceLib/RFL_DB.dat";

// Secciones del fichero de piezas (en el orden en que vienen)
enum Arc {
    ARC_BEARD, ARC_EYE, ARC_EYEBROW, ARC_FACELINE, ARC_FACETEX, ARC_FOREHEAD, ARC_GLASS, ARC_GLASSTEX, ARC_HAIR,
    ARC_MASK, ARC_MOLE, ARC_MOUTH, ARC_MUSTACHE, ARC_NOSE, ARC_NLINE, ARC_NLINETEX, ARC_CAP, ARC_CAPTEX, ARC_COUNT
};

u8* s_res = nullptr;
u32 s_resSize = 0;
struct ArcInfo { u16 numFiles; u32 offset; } s_arc[ARC_COUNT];
std::vector<MiiInfo> s_users;
std::vector<MiiHead*> s_pending;

inline u16 rd16(const u8* p) { return (u16)((p[0] << 8) | p[1]); }
inline u32 rd32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }

// Fichero `idx` de una sección: puntero dentro de RFL_Res.dat y tamaño
const u8* resFile(int arc, int idx, u32* size) {
    if (!s_res || arc < 0 || arc >= ARC_COUNT || idx < 0 || idx >= s_arc[arc].numFiles) return nullptr;
    const u8* table = s_res + s_arc[arc].offset;
    if (s_arc[arc].offset + (s_arc[arc].numFiles + 1) * 4 > s_resSize) return nullptr;
    u32 self = rd32(table + idx * 4), next = rd32(table + (idx + 1) * 4);
    u32 start = s_arc[arc].offset + s_arc[arc].numFiles * 4 + 4 + self;
    if (next < self || start + (next - self) > s_resSize) return nullptr;
    *size = next - self;
    return s_res + start;
}

// Campos de bits empaquetados de mayor a menor (como los guarda la Wii)
struct Bits {
    u16 v;
    int left = 16;
    explicit Bits(const u8* p) : v(rd16(p)) {}
    u8 take(int w) {
        left -= w;
        return (u8)((v >> left) & ((1 << w) - 1));
    }
};

bool decodeRaw(const u8* d, MiiInfo& m) {
    bool any = false;
    for (int i = 0; i < 8; i++) any |= d[0x18 + i] != 0;  // createID vacío = hueco libre
    if (!any) return false;
    Bits p0(d);
    p0.take(1);
    m.female = p0.take(1);
    p0.take(4);
    p0.take(5);
    m.favColor = p0.take(4);
    int n = 0;
    for (int i = 0; i < 10; i++) {
        u16 c = rd16(d + 2 + i * 2);
        if (c == 0) break;
        if (c >= 256) c = '?';
        if (c < 0x80) {
            m.name[n++] = (char)c;
        } else {
            m.name[n++] = (char)(0xC0 | (c >> 6));
            m.name[n++] = (char)(0x80 | (c & 0x3F));
        }
    }
    m.name[n] = 0;
    m.height = d[0x16];
    m.build = d[0x17];
    Bits f(d + 0x20);
    m.faceType = f.take(3); m.faceColor = f.take(3); m.faceTex = f.take(4);
    Bits h(d + 0x22);
    m.hairType = h.take(7); m.hairColor = h.take(3); m.hairFlip = h.take(1);
    Bits b1(d + 0x24);
    m.browType = b1.take(5); m.browRotate = b1.take(5);
    Bits b2(d + 0x26);
    m.browColor = b2.take(3); m.browScale = b2.take(4); m.browY = b2.take(5); m.browX = b2.take(4);
    Bits e1(d + 0x28);
    m.eyeType = e1.take(6); m.eyeRotate = e1.take(5); m.eyeY = e1.take(5);
    Bits e2(d + 0x2A);
    m.eyeColor = e2.take(3); m.eyeScale = e2.take(4); m.eyeX = e2.take(4);
    Bits ns(d + 0x2C);
    m.noseType = ns.take(4); m.noseScale = ns.take(4); m.noseY = ns.take(5);
    Bits mo(d + 0x2E);
    m.mouthType = mo.take(5); m.mouthColor = mo.take(2); m.mouthScale = mo.take(4); m.mouthY = mo.take(5);
    Bits g(d + 0x30);
    m.glassType = g.take(4); m.glassColor = g.take(3); m.glassScale = g.take(4); m.glassY = g.take(5);
    Bits bd(d + 0x32);
    m.mustache = bd.take(2); m.beardType = bd.take(2); m.beardColor = bd.take(3); m.beardScale = bd.take(4); m.beardY = bd.take(5);
    Bits ml(d + 0x34);
    m.moleType = ml.take(1); m.moleScale = ml.take(4); m.moleY = ml.take(5); m.moleX = ml.take(5);
    return true;
}

// ------------------------------------------------------------------ constantes de la cara (de la RVLFaceLib)
const float TEX_SCALE_X = 0.88961464f, TEX_SCALE_Y = 0.9276675f;
inline float scale2dim(int s) { return 1.0f + 0.4f * s; }
inline float rot2ang(int r) { return (360.0f / 32.0f) * (r % 32); }
const u8 EYE_ROT_OFFSET[50] = {29, 28, 28, 28, 29, 28, 28, 28, 29, 28, 28, 28, 28, 29, 29, 28, 28,
                               28, 29, 29, 28, 29, 28, 29, 29, 28, 29, 28, 28, 29, 28, 28, 28, 29,
                               29, 29, 28, 28, 29, 29, 29, 28, 28, 29, 29, 29, 29, 29, 28, 28};
const u8 EYEBROW_ROT_OFFSET[24] = {26, 26, 27, 25, 26, 25, 26, 25, 28, 25, 26, 24,
                                   27, 27, 26, 26, 25, 25, 26, 26, 27, 26, 25, 27};
const GXColor FACELINE[6] = {{240, 216, 196, 255}, {255, 188, 128, 255}, {216, 136, 80, 255},
                             {255, 176, 144, 255}, {152, 80, 48, 255},   {82, 46, 28, 255}};
const GXColor HAIR[8] = {{30, 26, 24, 255},   {56, 32, 21, 255},    {85, 38, 23, 255}, {112, 64, 36, 255},
                         {114, 114, 120, 255}, {73, 54, 26, 255},   {122, 89, 40, 255}, {193, 159, 100, 255}};
const GXColor GLASS[6] = {{16, 16, 16, 255}, {96, 56, 16, 255}, {152, 24, 16, 255},
                          {32, 48, 96, 255}, {144, 88, 0, 255}, {96, 88, 80, 255}};
const GXColor FAVORITE[12] = {{184, 64, 48, 255},  {240, 120, 40, 255}, {248, 216, 32, 255}, {128, 200, 40, 255},
                              {0, 116, 40, 255},   {32, 72, 152, 255},  {64, 160, 216, 255}, {232, 96, 120, 255},
                              {112, 44, 168, 255}, {72, 56, 24, 255},   {224, 224, 224, 255}, {24, 24, 20, 255}};
const GXColor EYE_COLOR1[6] = {{0, 0, 0, 255},      {124, 128, 128, 255}, {112, 80, 64, 255},
                               {112, 110, 64, 255}, {88, 104, 184, 255},  {72, 128, 104, 255}};
const GXColor MOUTH_COLOR0[3] = {{190, 78, 38, 255}, {216, 48, 40, 255}, {207, 68, 71, 255}};
const GXColor MOUTH_COLOR1[3] = {{113, 42, 4, 255}, {120, 21, 16, 255}, {126, 37, 40, 255}};
const GXColor MOLE_COLOR = {18, 15, 15, 255};

template <typename T> T clampi(T v, T lo, T hi) { return v < lo ? lo : (v > hi ? hi : v); }

// Textura suelta de RFL_Res.dat (cabecera de 32 bytes + imagen) copiada a un búfer alineado
struct LooseTex {
    u8* buf = nullptr;
    GXTexObj obj;
    bool load(int arc, int file) {
        u32 size;
        const u8* f = resFile(arc, file, &size);
        if (!f || size < 32) return false;
        u8 fmt = f[0];
        u16 w = rd16(f + 2), h = rd16(f + 4);
        u32 img = rd32(f + 0x1C);
        u32 bytes = GX_GetTexBufferSize(w, h, fmt, GX_FALSE, 0);
        if (img + bytes > size || w == 0 || h == 0) return false;
        buf = (u8*)memalign(32, (bytes + 31) & ~31u);
        if (!buf) return false;
        memcpy(buf, f + img, bytes);
        DCFlushRange(buf, (bytes + 31) & ~31u);
        GX_InitTexObj(&obj, buf, w, h, fmt, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GX_InitTexObjLOD(&obj, GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
        return true;
    }
    void release() {
        free(buf);
        buf = nullptr;
    }
};

enum Origin { ORIGIN_CENTER, ORIGIN_LEFT, ORIGIN_RIGHT };

// Dibuja una pieza de la cara (ojo, ceja, boca…) en el EFB, como RFLiDrawQuad
void drawQuad(GXTexObj* tex, float x, float y, float w, float h, float angleDeg, Origin origin) {
    GX_LoadTexObj(tex, GX_TEXMAP0);
    Mtx pos, rot;
    guMtxScale(pos, w, h, 1);
    guMtxRotRad(rot, 'z', angleDeg * (PI / 180.0f));
    guMtxConcat(rot, pos, pos);
    Mtx s;
    guMtxScale(s, TEX_SCALE_X, TEX_SCALE_Y, 1);
    guMtxConcat(s, pos, pos);
    guMtxTransApply(pos, pos, x, y, 0);
    GX_LoadPosMtxImm(pos, GX_PNMTX0);
    GX_SetCurrentMtx(GX_PNMTX0);
    float baseX = origin == ORIGIN_CENTER ? -0.5f : (origin == ORIGIN_RIGHT ? -1.0f : 0.0f);
    s16 s0 = origin == ORIGIN_LEFT ? 0 : 256, s1 = origin == ORIGIN_LEFT ? 256 : 0;
    GX_Begin(GX_QUADS, GX_VTXFMT3, 4);
    GX_Position2f32(1.0f + baseX, -0.5f); GX_Color1u32(0x000000FF); GX_TexCoord2s16(s0, 0);
    GX_Position2f32(1.0f + baseX, 0.5f); GX_Color1u32(0x000000FF); GX_TexCoord2s16(s0, 256);
    GX_Position2f32(baseX, 0.5f); GX_Color1u32(0x000000FF); GX_TexCoord2s16(s1, 256);
    GX_Position2f32(baseX, -0.5f); GX_Color1u32(0x000000FF); GX_TexCoord2s16(s1, 0);
    GX_End();
}

void stage(u8 st, u8 rasSwap, u8 texSwap, u8 a, u8 b, u8 c, u8 d, u8 alphaD) {
    GX_SetTevSwapMode(st, rasSwap, texSwap);
    GX_SetTevOrder(st, GX_TEXCOORD0, GX_TEXMAP0, GX_COLOR0A0);
    GX_SetTevColorIn(st, a, b, c, d);
    GX_SetTevAlphaIn(st, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, alphaD);
    GX_SetTevColorOp(st, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_SetTevAlphaOp(st, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

void tevFlat(GXColor c) {  // un color, alfa de la textura (cejas, bigote, lunar)
    GX_SetNumTevStages(1);
    stage(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_C0, GX_CA_TEXA);
    GX_SetTevColor(GX_TEVREG0, c);
}

void tev3(GXColor c0, GXColor c1, u8 swap1Tex, u8 swap2Tex) {  // ojos y boca: rojo→c0, otro canal→c1, otro→blanco
    GX_SetNumTevStages(3);
    stage(GX_TEVSTAGE0, GX_TEV_SWAP1, GX_TEV_SWAP1, GX_CC_ZERO, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO, GX_CA_TEXA);
    stage(GX_TEVSTAGE1, GX_TEV_SWAP3, swap1Tex, GX_CC_ZERO, GX_CC_C1, GX_CC_TEXC, GX_CC_CPREV, GX_CA_APREV);
    stage(GX_TEVSTAGE2, GX_TEV_SWAP2, swap2Tex, GX_CC_ZERO, GX_CC_ONE, GX_CC_TEXC, GX_CC_CPREV, GX_CA_APREV);
    GX_SetTevColor(GX_TEVREG0, c0);
    GX_SetTevColor(GX_TEVREG1, c1);
}

// Pinta la cara (sin piel) de un Mii en la esquina del EFB y la copia a `dst` (RGB5A3, size×size)
void captureFace(const MiiInfo& m, u16 size, u8* dst) {
    LooseTex eye, brow, mouth, mus, mole;
    eye.load(ARC_EYE, m.eyeType);
    brow.load(ARC_EYEBROW, m.browType);
    mouth.load(ARC_MOUTH, m.mouthType);
    mus.load(ARC_MUSTACHE, m.mustache);
    mole.load(ARC_MOLE, m.moleType);

    const float r = size / 64.0f;
    float eyeX = TEX_SCALE_X * m.eyeX, eyeY = 18.451525f + 1.16f * TEX_SCALE_Y * m.eyeY;
    float eyeW = 342 / 64.0f * scale2dim(m.eyeScale), eyeH = 288 / 64.0f * scale2dim(m.eyeScale);
    float eyeA = rot2ang(m.eyeRotate + EYE_ROT_OFFSET[m.eyeType % 50]);
    float browX = TEX_SCALE_X * m.browX, browY = 16.549807f + 1.16f * TEX_SCALE_Y * m.browY;
    float browW = 324 / 64.0f * scale2dim(m.browScale), browH = 288 / 64.0f * scale2dim(m.browScale);
    float browA = rot2ang(m.browRotate + EYEBROW_ROT_OFFSET[m.browType % 24]);
    float mouthY = 29.25885f + 1.16f * TEX_SCALE_Y * m.mouthY;
    float mouthW = 396 / 64.0f * scale2dim(m.mouthScale), mouthH = 288 / 64.0f * scale2dim(m.mouthScale);
    float musY = 31.763554f + 1.16f * TEX_SCALE_Y * m.beardY;
    float musW = 288 / 64.0f * scale2dim(m.beardScale), musH = 576 / 64.0f * scale2dim(m.beardScale);
    float moleX = 17.766165f + 2.0f * TEX_SCALE_X * m.moleX, moleY = 17.95986f + 1.16f * TEX_SCALE_Y * m.moleY;
    float moleS = scale2dim(m.moleScale);

    GXColor eye0 = m.eyeType == 9 ? (GXColor){255, 130, 0, 255} : (m.eyeType == 20 ? (GXColor){0, 255, 255, 255} : (GXColor){0, 0, 0, 255});

    for (int pass = 0; pass < 2; pass++) {
        if (pass == 0) {
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_INVDSTALPHA, GX_BL_DSTALPHA, GX_LO_SET);
            GX_SetColorUpdate(GX_TRUE);
        } else {
            // Segunda pasada solo para el alfa (el borrado de la copia respeta la máscara de color)
            GX_SetColorUpdate(GX_FALSE);
            gfx::offscreenCopy(dst, size, GX_TF_RGB5A3);
            GX_SetBlendMode(GX_BM_BLEND, GX_BL_INVDSTALPHA, GX_BL_ONE, GX_LO_SET);
        }
        if (mus.buf) {
            tevFlat(HAIR[m.beardColor % 8]);
            drawQuad(&mus.obj, r * 32, musY * r, musW * r, musH * r, 0, ORIGIN_RIGHT);
            drawQuad(&mus.obj, r * 32, musY * r, musW * r, musH * r, 0, ORIGIN_LEFT);
        }
        if (mouth.buf) {
            tev3(MOUTH_COLOR0[m.mouthColor % 3], MOUTH_COLOR1[m.mouthColor % 3], GX_TEV_SWAP2, GX_TEV_SWAP3);
            drawQuad(&mouth.obj, r * 32, mouthY * r, mouthW * r, mouthH * r, 0, ORIGIN_CENTER);
        }
        if (brow.buf) {
            tevFlat(HAIR[m.browColor % 8]);
            drawQuad(&brow.obj, r * (32 - browX), browY * r, browW * r, browH * r, browA, ORIGIN_RIGHT);
            drawQuad(&brow.obj, r * (32 + browX), browY * r, browW * r, browH * r, 360 - browA, ORIGIN_LEFT);
        }
        if (eye.buf) {
            tev3(eye0, EYE_COLOR1[m.eyeColor % 6], GX_TEV_SWAP3, GX_TEV_SWAP2);
            drawQuad(&eye.obj, r * (32 - eyeX), eyeY * r, eyeW * r, eyeH * r, eyeA, ORIGIN_RIGHT);
            drawQuad(&eye.obj, r * (32 + eyeX), eyeY * r, eyeW * r, eyeH * r, 360 - eyeA, ORIGIN_LEFT);
        }
        if (mole.buf) {
            tevFlat(MOLE_COLOR);
            drawQuad(&mole.obj, moleX * r, moleY * r, moleS * r, moleS * r, 0, ORIGIN_CENTER);
        }
    }
    GX_SetColorUpdate(GX_TRUE);
    GX_DrawDone();
    gfx::offscreenCopy(dst, size, GX_TF_RGB5A3);
    GX_DrawDone();
    eye.release();
    brow.release();
    mouth.release();
    mus.release();
    mole.release();
}

void setup2DCapture() {
    GX_ClearVtxDesc();
    GX_InvVtxCache();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
    GX_SetVtxAttrFmt(GX_VTXFMT3, GX_VA_POS, GX_POS_XY, GX_F32, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT3, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(GX_VTXFMT3, GX_VA_TEX0, GX_TEX_ST, GX_S16, 8);
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0A0, GX_DISABLE, GX_SRC_REG, GX_SRC_VTX, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetNumTexGens(1);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    GX_SetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED, GX_CH_RED, GX_CH_RED, GX_CH_ALPHA);
    GX_SetTevSwapModeTable(GX_TEV_SWAP2, GX_CH_GREEN, GX_CH_GREEN, GX_CH_GREEN, GX_CH_ALPHA);
    GX_SetTevSwapModeTable(GX_TEV_SWAP3, GX_CH_BLUE, GX_CH_BLUE, GX_CH_BLUE, GX_CH_ALPHA);
    for (int i = 0; i < 3; i++) GX_SetTevDirect((u8)(GX_TEVSTAGE0 + i));
    GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_OR, GX_NEVER, 0);
    GX_SetDither(GX_DISABLE);
}

// Formato de los vértices de las piezas: índices de 8 bits a arrays de s16 (posición .8, normal .14, uv .13)
void setHeadVtx(bool txc) {
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_NRM, GX_INDEX8);
    GX_SetVtxDesc(GX_VA_TEX0, txc ? GX_INDEX8 : GX_NONE);
    GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_POS, GX_POS_XYZ, GX_S16, 8);
    GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_NRM, GX_NRM_XYZ, GX_S16, 14);
    GX_SetVtxAttrFmt(GX_VTXFMT2, GX_VA_TEX0, GX_TEX_ST, GX_S16, 13);
}

}  // namespace

// ------------------------------------------------------------------ API
namespace mii {

bool init() {
    if (s_res) return true;
    u32 size = 0;
    u8* res = nand::readFile(RES_PATH, &size);
    if (res && size > (ARC_COUNT + 1) * 4) {
        bool ok = true;
        for (int i = 0; i < ARC_COUNT && ok; i++) {
            u32 off = rd32(res + (i + 1) * 4);
            if (off + 4 > size) ok = false;
            else {
                s_arc[i].numFiles = rd16(res + off);
                s_arc[i].offset = off + 4;
            }
        }
        if (ok) {
            s_res = res;
            s_resSize = size;
        } else {
            free(res);
        }
    } else if (res) {
        free(res);
    }
    u8* db = nand::readFile(DB_PATH, &size);
    if (db && size >= 4 + 100 * 74 && memcmp(db, "RNOD", 4) == 0) {
        for (int i = 0; i < 100; i++) {
            MiiInfo m;
            if (decodeRaw(db + 4 + i * 74, m)) s_users.push_back(m);
        }
    }
    free(db);
    return s_res != nullptr;
}

bool available() { return s_res != nullptr; }
int userCount() { return (int)s_users.size(); }
const MiiInfo& user(int i) { return s_users[i]; }

MiiInfo guest(int i) {
    // Los invitados de la Wii: misma cara de base con distinto pelo, color y piel
    static const u8 hair[6] = {68, 55, 33, 24, 14, 12}, hairCol[6] = {0, 6, 1, 0, 7, 1}, skin[6] = {4, 0, 1, 2, 0, 0};
    static const u8 fav[6] = {4, 5, 0, 2, 6, 7}, eyeCol[6] = {0, 4, 0, 0, 5, 0};
    MiiInfo m;
    i = clampi(i, 0, 5);
    m.female = i >= 3;
    m.hairType = hair[i];
    m.hairColor = hairCol[i];
    m.browColor = hairCol[i];
    m.faceColor = skin[i];
    m.favColor = fav[i];
    m.eyeColor = eyeCol[i];
    if (m.female) {
        m.browType = 0;
        m.eyeType = 4;
        m.eyeRotate = 3;
    }
    return m;
}

MiiInfo random(u32 seed, bool female) {
    auto rnd = [&seed](int n) {
        seed = seed * 1103515245u + 12345u;
        return (int)((seed >> 16) % (u32)n);
    };
    MiiInfo m = guest(female ? 3 : 0);
    m.faceType = rnd(8);
    static const u8 skins[8] = {0, 0, 0, 1, 1, 2, 3, 4};
    m.faceColor = skins[rnd(8)];
    m.faceTex = rnd(5) == 0 ? rnd(12) : 0;
    m.hairType = rnd(72);
    m.hairColor = rnd(8);
    m.hairFlip = rnd(2);
    m.eyeType = rnd(48);
    m.eyeColor = rnd(3) == 0 ? rnd(6) : 0;
    m.eyeY = 10 + rnd(5);
    m.browType = rnd(24);
    m.browColor = m.hairColor;
    m.browY = 9 + rnd(3);
    m.noseType = rnd(12);
    m.mouthType = rnd(24);
    m.mouthColor = rnd(3);
    if (rnd(6) == 0) {
        m.glassType = 1 + rnd(8);
        m.glassColor = rnd(6);
    }
    if (!female && rnd(5) == 0) {
        m.mustache = 1 + rnd(3);
        m.beardColor = m.hairColor;
    }
    if (!female && rnd(7) == 0) {
        m.beardType = 1 + rnd(3);
        m.beardColor = m.hairColor;
    }
    m.favColor = rnd(12);
    return m;
}

Color favoriteColor(int i) {
    GXColor c = FAVORITE[clampi(i, 0, 11)];
    return rgba(c.r, c.g, c.b);
}

Color skinColor(int i) {
    GXColor c = FACELINE[clampi(i, 0, 5)];
    return rgba(c.r, c.g, c.b);
}

void processPending() {
    if (s_pending.empty()) return;
    gfx::offscreenBegin(256);
    setup2DCapture();
    for (MiiHead* h : s_pending) h->bakeMasks();
    s_pending.clear();
    gfx::offscreenEnd();
}

}  // namespace mii

// ------------------------------------------------------------------ cabeza
bool MiiHead::loadShape(Part& p, int arc, int file, bool transform, bool flipX, float scale, const float* trans,
                        float* extra) {
    u32 size;
    const u8* f = resFile(arc, file, &size);
    if (!f || size < 8) return false;
    const u8* end = f + size;
    const u8* q = f + 4;
    if (arc == ARC_FACELINE) {  // la cara trae dónde van la nariz, la barba y el pelo
        if (q + 36 > end) return false;
        memcpy(extra, q, 36);
        q += 36;
    }
    bool txc = !(arc == ARC_FOREHEAD || arc == ARC_HAIR || arc == ARC_BEARD || arc == ARC_NOSE);
    u16 nPos = rd16(q);
    q += 2;
    if (nPos == 0) return false;
    if (q + nPos * 6 > end) return false;
    p.pos = (s16*)memalign(32, (nPos * 6 + 31) & ~31);
    p.nPos = nPos;
    s32 s = (s32)(256.0f * scale);
    s32 t[3] = {0, 0, 0};
    if (transform)
        for (int k = 0; k < 3; k++) t[k] = (s32)(256.0f * trans[k]);
    for (int i = 0; i < nPos; i++) {
        s16 v[3] = {(s16)rd16(q), (s16)rd16(q + 2), (s16)rd16(q + 4)};
        q += 6;
        if (flipX) v[0] = -v[0];
        if (transform)
            for (int k = 0; k < 3; k++) v[k] = (s16)(t[k] + ((v[k] * s) >> 8));
        memcpy(&p.pos[i * 3], v, 6);
    }
    DCFlushRange(p.pos, (nPos * 6 + 31) & ~31);
    if (q + 2 > end) return false;
    u16 nNrm = rd16(q);
    q += 2;
    if (q + nNrm * 6 > end) return false;
    p.nrm = (s16*)memalign(32, (nNrm * 6 + 31) & ~31);
    for (int i = 0; i < nNrm; i++) {
        s16 v[3] = {(s16)rd16(q), (s16)rd16(q + 2), (s16)rd16(q + 4)};
        q += 6;
        if (flipX) v[0] = -v[0];
        memcpy(&p.nrm[i * 3], v, 6);
    }
    DCFlushRange(p.nrm, (nNrm * 6 + 31) & ~31);
    p.hasTxc = txc;
    if (txc) {
        if (q + 2 > end) return false;
        u16 nTxc = rd16(q);
        q += 2;
        if (q + nTxc * 4 > end) return false;
        p.txc = (s16*)memalign(32, (nTxc * 4 + 31) & ~31);
        memcpy(p.txc, q, nTxc * 4);
        q += nTxc * 4;
        DCFlushRange(p.txc, (nTxc * 4 + 31) & ~31);
    }
    // Lista de primitivas → display list propia (formato de vértices 2)
    if (q >= end) return false;
    int prims = *q++;
    u32 need = 64;
    const u8* scan = q;
    for (int i = 0; i < prims; i++) {
        if (scan + 2 > end) return false;
        int n = scan[0];
        scan += 2 + n * (txc ? 3 : 2);
        need += 3 + n * (txc ? 3 : 2);
    }
    if (scan > end) return false;
    u32 cap = (need + 63) & ~31u;
    p.dl = (u8*)memalign(32, cap);
    DCInvalidateRange(p.dl, cap);
    setHeadVtx(txc);
    GX_BeginDispList(p.dl, cap);
    for (int i = 0; i < prims; i++) {
        int n = *q++;
        u8 prim = *q++;
        GX_Begin(prim, GX_VTXFMT2, n);
        for (int j = 0; j < n; j++) {
            GX_Position1x8(*q++);
            GX_Normal1x8(*q++);
            if (txc) GX_TexCoord1x8(*q++);
        }
        GX_End();
    }
    p.dlSize = GX_EndDispList();
    return p.dlSize > 0;
}

bool MiiHead::loadTex(GXTexObj& obj, u8*& buf, int arc, int file) {
    LooseTex t;
    if (!t.load(arc, file)) return false;
    buf = t.buf;
    obj = t.obj;
    return true;
}

void MiiHead::releasePart(Part& p) {
    free(p.pos);
    free(p.nrm);
    free(p.txc);
    free(p.dl);
    p = Part();
}

bool MiiHead::build(const MiiInfo& info, u16 maskSize) {
    release();
    if (!mii::available()) return false;
    info_ = info;
    maskSize_ = maskSize;
    float extra[9] = {0};  // nariz, barba, pelo (x, y, z)
    if (!loadShape(face_, ARC_FACELINE, info.faceType, false, false, 1, nullptr, extra)) {
        release();
        return false;
    }
    loadShape(mask_, ARC_MASK, info.faceType, false, false, 1, nullptr, nullptr);
    const float* noseT = extra;
    const float* beardT = extra + 3;
    const float* hairT = extra + 6;
    loadShape(cap_, ARC_CAP, info.hairType, true, info.hairFlip, 1, hairT, nullptr);
    loadShape(hair_, ARC_HAIR, info.hairType, true, info.hairFlip, 1, hairT, nullptr);
    loadShape(forehead_, ARC_FOREHEAD, info.hairType, true, info.hairFlip, 1, hairT, nullptr);
    if (info.beardType) loadShape(beard_, ARC_BEARD, info.beardType, true, false, 1, beardT, nullptr);
    float noseScale = 0.4f + 0.175f * info.noseScale;
    float nt[3] = {noseT[0], noseT[1] - 1.5f * (info.noseY - 8), noseT[2]};
    loadShape(nose_, ARC_NOSE, info.noseType, true, false, noseScale, nt, nullptr);
    loadShape(noseline_, ARC_NLINE, info.noseType, true, false, noseScale, nt, nullptr);
    if (info.glassType) {
        float gt[3] = {noseT[0], 5.0f + noseT[1] - 1.5f * (info.glassY - 11), 2.0f + noseT[2]};
        loadShape(glass_, ARC_GLASS, 0, true, false, 0.15f * info.glassScale + 0.4f, gt, nullptr);
        loadTex(glassTex_, glassTexBuf_, ARC_GLASSTEX, info.glassType);
    }
    loadTex(faceTex_, faceTexBuf_, ARC_FACETEX, info.faceTex);
    if (cap_.dl) loadTex(capTex_, capTexBuf_, ARC_CAPTEX, info.hairType);
    if (noseline_.dl) loadTex(nlineTex_, nlineTexBuf_, ARC_NLINETEX, info.noseType);

    // Escala: la cara (contorno) mide 0.43 m de ancho; origen en la barbilla
    s16 mn[3] = {32767, 32767, 32767}, mx[3] = {-32768, -32768, -32768};
    for (u32 i = 0; i < face_.nPos; i++)
        for (int k = 0; k < 3; k++) {
            s16 v = face_.pos[i * 3 + k];
            if (v < mn[k]) mn[k] = v;
            if (v > mx[k]) mx[k] = v;
        }
    float width = (mx[0] - mn[0]) / 256.0f;
    if (width <= 0.1f) width = 30;
    scale_ = 0.43f / width;
    center_[0] = (mx[0] + mn[0]) / 512.0f;
    center_[1] = mn[1] / 256.0f;
    center_[2] = (mx[2] + mn[2]) / 512.0f;
    s16 topY = mx[1];
    for (const Part* p : {&hair_, &cap_, &forehead_})
        for (u32 i = 0; i < p->nPos; i++) topY = p->pos[i * 3 + 1] > topY ? p->pos[i * 3 + 1] : topY;
    top_ = (topY / 256.0f - center_[1]) * scale_;

    for (int e = 0; e < 2; e++) {
        u32 bytes = maskSize_ * maskSize_ * 2;
        maskBuf_[e] = (u8*)memalign(32, bytes);
        memset(maskBuf_[e], 0, bytes);
        DCFlushRange(maskBuf_[e], bytes);
        GX_InitTexObj(&maskObj_[e], maskBuf_[e], maskSize_, maskSize_, GX_TF_RGB5A3, GX_CLAMP, GX_CLAMP, GX_FALSE);
        GX_InitTexObjLOD(&maskObj_[e], GX_LINEAR, GX_LINEAR, 0, 0, 0, GX_FALSE, GX_FALSE, GX_ANISO_1);
    }
    gfx::restore3D();
    built_ = true;
    masksDone_ = false;
    s_pending.push_back(this);
    return true;
}

void MiiHead::release() {
    for (size_t i = 0; i < s_pending.size(); i++)
        if (s_pending[i] == this) {
            s_pending.erase(s_pending.begin() + i);
            break;
        }
    for (Part* p : {&face_, &mask_, &hair_, &cap_, &forehead_, &nose_, &noseline_, &beard_, &glass_}) releasePart(*p);
    for (u8** b : {&faceTexBuf_, &capTexBuf_, &nlineTexBuf_, &glassTexBuf_, &maskBuf_[0], &maskBuf_[1]}) {
        free(*b);
        *b = nullptr;
    }
    built_ = masksDone_ = false;
}

void MiiHead::bakeMasks() {
    if (!built_) return;
    MiiInfo blink = info_;
    blink.eyeType = 26;
    int rot = (int)info_.eyeRotate + EYE_ROT_OFFSET[info_.eyeType % 50] - EYE_ROT_OFFSET[26];
    blink.eyeRotate = (u8)clampi(rot, 0, 7);
    captureFace(info_, maskSize_, maskBuf_[0]);
    captureFace(blink, maskSize_, maskBuf_[1]);
    masksDone_ = true;
}

void MiiHead::draw(const Xf& model, bool blink) const {
    if (!ready()) return;
    gfx::loadModelMtx(model * Xf().scale(scale_).translate(Vec3(-center_[0], -center_[1], -center_[2])), scale_);
    GX_InvVtxCache();
    // Luz del motor (luces 0 y 1 y el ambiente de begin3D) sobre un material blanco; el color sale de la TEV
    GX_SetNumChans(1);
    GX_SetChanCtrl(GX_COLOR0, GX_ENABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHT0 | GX_LIGHT1, GX_DF_CLAMP, GX_AF_NONE);
    GX_SetChanCtrl(GX_ALPHA0, GX_DISABLE, GX_SRC_REG, GX_SRC_REG, GX_LIGHTNULL, GX_DF_NONE, GX_AF_NONE);
    GX_SetChanMatColor(GX_COLOR0, (GXColor){255, 255, 255, 255});
    GX_SetNumTexGens(1);
    GX_SetNumTevStages(2);
    GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    GX_SetTevSwapModeTable(GX_TEV_SWAP1, GX_CH_RED, GX_CH_ALPHA, GX_CH_BLUE, GX_CH_GREEN);
    GX_SetTevDirect(GX_TEVSTAGE0);
    GX_SetTevDirect(GX_TEVSTAGE1);
    // Etapa 1: multiplicar por la luz
    GX_SetTevSwapMode(GX_TEVSTAGE1, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GX_SetTevOrder(GX_TEVSTAGE1, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLOR0A0);
    GX_SetTevColorIn(GX_TEVSTAGE1, GX_CC_ZERO, GX_CC_CPREV, GX_CC_RASC, GX_CC_ZERO);
    GX_SetTevAlphaIn(GX_TEVSTAGE1, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_APREV);
    GX_SetTevColorOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_SetTevAlphaOp(GX_TEVSTAGE1, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    // Etapa 0: color constante de cada pieza
    GX_SetTevKColorSel(GX_TEVSTAGE0, GX_TEV_KCSEL_K0);
    GX_SetTevKAlphaSel(GX_TEVSTAGE0, GX_TEV_KASEL_1);
    GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_SetTevAlphaOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
    GX_SetZMode(GX_TRUE, GX_LEQUAL, GX_TRUE);

    const u8 cullOn = GX_CULL_BACK, cullFlip = GX_CULL_FRONT;
    GX_SetCullMode(cullOn);
    auto drawPart = [](const Part& p) {
        if (!p.dl) return;
        setHeadVtx(p.hasTxc);
        GX_SetArray(GX_VA_POS, p.pos, 6);
        GX_SetArray(GX_VA_NRM, p.nrm, 6);
        if (p.hasTxc) GX_SetArray(GX_VA_TEX0, p.txc, 4);
        GX_CallDispList(p.dl, p.dlSize);
    };

    // Piezas opacas sin textura
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_POS, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLORNULL);
    GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST);
    if (beard_.dl) {
        GX_SetTevKColor(GX_KCOLOR0, HAIR[info_.beardColor % 8]);
        drawPart(beard_);
    }
    GX_SetTevKColor(GX_KCOLOR0, FACELINE[info_.faceColor % 6]);
    drawPart(nose_);
    if (info_.hairFlip) GX_SetCullMode(cullFlip);
    drawPart(forehead_);
    GX_SetTevKColor(GX_KCOLOR0, HAIR[info_.hairColor % 8]);
    drawPart(hair_);
    // Piezas con textura: gorra (color favorito) y cara (piel + maquillaje)
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevOrder(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL);
    if (cap_.dl && capTexBuf_) {
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC, GX_CC_KONST);
        GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_DIVIDE_2, GX_TRUE, GX_TEVPREV);
        GX_SetTevKColor(GX_KCOLOR0, FAVORITE[info_.favColor % 12]);
        GX_LoadTexObj((GXTexObj*)&capTex_, GX_TEXMAP0);
        drawPart(cap_);
        GX_SetTevColorOp(GX_TEVSTAGE0, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
    }
    if (info_.hairFlip) GX_SetCullMode(cullOn);
    if (faceTexBuf_) {
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_KONST, GX_CC_TEXC, GX_CC_TEXA, GX_CC_ZERO);
        GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP1);
        GX_LoadTexObj((GXTexObj*)&faceTex_, GX_TEXMAP0);
    } else {
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_KONST);
    }
    GX_SetTevKColor(GX_KCOLOR0, FACELINE[info_.faceColor % 6]);
    drawPart(face_);
    GX_SetTevSwapMode(GX_TEVSTAGE0, GX_TEV_SWAP0, GX_TEV_SWAP0);

    // Transparentes: la máscara con la cara pintada, la raya de la nariz y las gafas
    GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GX_SetAlphaCompare(GX_GREATER, 0, GX_AOP_OR, GX_NEVER, 0);
    GX_SetTevAlphaIn(GX_TEVSTAGE0, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_TEXA);
    GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_TEXC);
    GX_LoadTexObj((GXTexObj*)&maskObj_[blink ? 1 : 0], GX_TEXMAP0);
    drawPart(mask_);
    if (noseline_.dl && nlineTexBuf_) {
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO, GX_CC_ZERO);
        GX_LoadTexObj((GXTexObj*)&nlineTex_, GX_TEXMAP0);
        drawPart(noseline_);
    }
    if (glass_.dl && glassTexBuf_) {
        GX_SetCullMode(GX_CULL_NONE);
        GX_SetTevKColor(GX_KCOLOR0, GLASS[info_.glassColor % 6]);
        GX_SetTevColorIn(GX_TEVSTAGE0, GX_CC_ZERO, GX_CC_KONST, GX_CC_TEXC, GX_CC_ZERO);
        GX_LoadTexObj((GXTexObj*)&glassTex_, GX_TEXMAP0);
        drawPart(glass_);
    }
    gfx::restore3D();
}
