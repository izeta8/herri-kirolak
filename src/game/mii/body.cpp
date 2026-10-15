#include "game/mii/body.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "engine/tuning.h"
#include "game/mii/light.h"
#include "game/mii/mii.h"

namespace miibody {
namespace {

// Materiales de Resort (TEV de cada modelo, ver tools/mii_body.py)
enum Material { MAT_PLAYER = 0, MAT_CROWD = 1, MAT_HAND = 2 };

struct BoneDef {
    char name[16];
    int parent;
    Xf rest;    // matriz de reposo en espacio del modelo (unidades de Resort)
    Vec3 pos;   // origen del hueso en reposo
    void* dl = nullptr;
    u32 dlSize = 0;
};

struct BodyDef {
    int material = MAT_PLAYER;
    int texA = -1, texAWY = -1, texB = -1;
    u8 wrapS = GX_CLAMP, wrapT = GX_CLAMP;
    std::vector<BoneDef> bones;
    int find(const char* n) const {
        for (size_t i = 0; i < bones.size(); i++)
            if (strcmp(bones[i].name, n) == 0) return (int)i;
        return -1;
    }
};

struct Tex {
    void* data = nullptr;
    u32 fmt = 0;
    u16 w = 0, h = 0;
};

BodyDef s_defs[NUM_KINDS];
std::vector<Tex> s_tex;

inline u32 rd32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
inline u16 rd16(const u8* p) { return (u16)((p[0] << 8) | p[1]); }
inline float rdf(const u8* p) {
    u32 v = rd32(p);
    float f;
    memcpy(&f, &v, 4);
    return f;
}

Xf inverse(const Xf& x) {
    Xf r;
    guMtxInverse((MtxP)x.m, r.m);
    return r;
}
Xf translation(const Vec3& t) { return Xf().translate(t); }

// Solo la rotación de una matriz (sin traslación)
Xf rotationOf(const Xf& x) {
    Xf r = x;
    r.m[0][3] = r.m[1][3] = r.m[2][3] = 0;
    return r;
}

// Formato de vértice de los cuerpos: posición, normal, color (solo el alfa: oclusión) y UV
constexpr u8 VTXFMT = GX_VTXFMT4;
constexpr u32 VERT_BYTES = 12 + 12 + 4 + 8;

void setVtxFormat() {
    GX_ClearVtxDesc();
    GX_SetVtxDesc(GX_VA_POS, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_NRM, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_CLR0, GX_DIRECT);
    GX_SetVtxDesc(GX_VA_TEX0, GX_DIRECT);
}

// Display list con los triángulos de un hueso (vértices de 36 bytes del .bin)
void buildDL(BoneDef& b, const std::vector<const u8*>& verts) {
    if (verts.size() < 3) return;
    u32 n = verts.size() / 3 * 3;
    u32 cap = (3 + n * VERT_BYTES + 63) & ~31u;
    b.dl = memalign(32, cap);
    if (!b.dl) return;
    DCInvalidateRange(b.dl, cap);
    GX_BeginDispList(b.dl, cap);
    GX_Begin(GX_TRIANGLES, VTXFMT, (u16)n);
    for (u32 i = 0; i < n; i++) {
        const u8* p = verts[i];
        GX_Position3f32(rdf(p + 4), rdf(p + 8), rdf(p + 12));
        GX_Normal3f32(rdf(p + 16), rdf(p + 20), rdf(p + 24));
        GX_Color4u8(255, 255, 255, p[1]);
        GX_TexCoord2f32(rdf(p + 28), rdf(p + 32));
    }
    GX_End();
    b.dlSize = GX_EndDispList();
    if (b.dlSize == 0) {
        free(b.dl);
        b.dl = nullptr;
    }
}

void freeAll() {
    for (auto& def : s_defs) {
        for (auto& b : def.bones)
            if (b.dl) free(b.dl);
        def = BodyDef();
    }
    for (auto& t : s_tex)
        if (t.data) free(t.data);
    s_tex.clear();
}

void loadTex(int i, u8 map, u8 wrapS, u8 wrapT) {
    if (i < 0 || i >= (int)s_tex.size()) return;
    GXTexObj o;
    const Tex& t = s_tex[i];
    GX_InitTexObj(&o, t.data, t.w, t.h, (u8)t.fmt, wrapS, wrapT, GX_FALSE);
    GX_InitTexObjFilterMode(&o, GX_LINEAR, GX_LINEAR);
    GX_LoadTexObj(&o, map);
}

// Una etapa de TEV: color = d + lerp(a, b, c), alfa constante
void stage(u8 st, u8 coord, u32 map, u8 chan, u8 a, u8 b, u8 c, u8 d, u8 dest) {
    GX_SetTevDirect(st);
    GX_SetTevOrder(st, coord, map, chan);
    GX_SetTevSwapMode(st, GX_TEV_SWAP0, GX_TEV_SWAP0);
    GX_SetTevColorIn(st, a, b, c, d);
    GX_SetTevColorOp(st, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, dest);
    GX_SetTevKAlphaSel(st, GX_TEV_KASEL_1);
    GX_SetTevAlphaIn(st, GX_CA_ZERO, GX_CA_ZERO, GX_CA_ZERO, GX_CA_KONST);
    GX_SetTevAlphaOp(st, GX_TEV_ADD, GX_TB_ZERO, GX_CS_SCALE_1, GX_TRUE, GX_TEVPREV);
}

// El material del modelo, tal cual lo trae Resort (C0 = color favorito, C1 = piel, C2 = albedo)
void setMaterial(const BodyDef& def, const Body& b, const BodyDef& clothes) {
    setVtxFormat();
    // Los modelos de Nintendo van en orden horario (el frontal de GX, material "cull back"); el motor usa el contrario
    GX_SetCullMode(GX_CULL_BACK);
    miilight::bind(def.material != MAT_CROWD);
    // Las dos texturas de ropa tienen tamaños distintos: cada una necesita su coordenada (GX escala por coordenada)
    GX_SetNumTexGens(3);
    GX_SetTexCoordGen(GX_TEXCOORD0, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTexCoordGen(GX_TEXCOORD2, GX_TG_MTX2x4, GX_TG_TEX0, GX_IDENTITY);
    GX_SetTevSwapModeTable(GX_TEV_SWAP0, GX_CH_RED, GX_CH_GREEN, GX_CH_BLUE, GX_CH_ALPHA);
    GX_SetTevColor(GX_TEVREG0, b.shirt);
    GX_SetTevColor(GX_TEVREG1, b.skin);
    if (def.material == MAT_CROWD) {
        GX_SetTevColor(GX_TEVREG1, (GXColor){(u8)tuning::MII_CROWD_BASE_R, (u8)tuning::MII_CROWD_BASE_G,
                                             (u8)tuning::MII_CROWD_BASE_B, 255});
        loadTex(def.texA, GX_TEXMAP0, def.wrapS, def.wrapT);
        GX_SetNumTevStages(3);
        // base oscura o camiseta según la textura → C2; luego × (lm0 + ambiente) + lm1
        stage(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL, GX_CC_C1, GX_CC_C0, GX_CC_TEXC, GX_CC_ZERO, GX_TEVREG2);
        stage(GX_TEVSTAGE1, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_ONE, GX_CC_RASC, GX_TEVPREV);
        stage(GX_TEVSTAGE2, GX_TEXCOORD1, GX_TEXMAP2, GX_COLORNULL, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_TEXC, GX_TEVPREV);
        return;
    }
    // Jugador y manos (con la ropa del cuerpo): dibujos de la camiseta, luego la piel → C2; luego la luz
    loadTex(b.lightShirt && clothes.texAWY >= 0 ? clothes.texAWY : clothes.texA, GX_TEXMAP0, clothes.wrapS, clothes.wrapT);
    loadTex(clothes.texB, GX_TEXMAP3, GX_CLAMP, GX_CLAMP);
    GX_SetNumTevStages(5);
    stage(GX_TEVSTAGE0, GX_TEXCOORD0, GX_TEXMAP0, GX_COLORNULL, GX_CC_TEXC, GX_CC_C0, GX_CC_TEXA, GX_CC_ZERO, GX_TEVPREV);
    stage(GX_TEVSTAGE1, GX_TEXCOORD2, GX_TEXMAP3, GX_COLORNULL, GX_CC_CPREV, GX_CC_C1, GX_CC_TEXC, GX_CC_ZERO, GX_TEVREG2);
    stage(GX_TEVSTAGE2, GX_TEXCOORD1, GX_TEXMAP1, GX_COLOR0A0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASA, GX_CC_RASC, GX_TEVPREV);
    stage(GX_TEVSTAGE3, GX_TEXCOORDNULL, GX_TEXMAP_NULL, GX_COLORNULL, GX_CC_ZERO, GX_CC_C2, GX_CC_CPREV, GX_CC_ZERO,
          GX_TEVPREV);
    stage(GX_TEVSTAGE4, GX_TEXCOORD1, GX_TEXMAP2, GX_COLOR0A0, GX_CC_ZERO, GX_CC_TEXC, GX_CC_RASA, GX_CC_CPREV, GX_TEVPREV);
}

}  // namespace

bool init() {
    miilight::init();
    freeAll();
    u32 size = 0;
    u8* d = mii::readAsset("hk_body.bin", &size);
    if (!d) return false;
    const u8* end = d + size;
    bool ok = size > 12 && memcmp(d, "HKB2", 4) == 0;
    u32 ntex = ok ? rd32(d + 4) : 0, nbody = ok ? rd32(d + 8) : 0;
    const u8* p = d + 12;
    for (u32 i = 0; ok && i < ntex; i++) {
        if (p + 36 > end) { ok = false; break; }
        Tex t;
        t.fmt = rd32(p + 24);
        t.w = rd16(p + 28);
        t.h = rd16(p + 30);
        u32 bytes = rd32(p + 32);
        p += 36;
        if (p + bytes > end) { ok = false; break; }
        t.data = memalign(32, bytes);
        memcpy(t.data, p, bytes);
        DCFlushRange(t.data, bytes);
        s_tex.push_back(t);
        p += (bytes + 31) & ~31u;
    }
    GX_SetVtxAttrFmt(VTXFMT, GX_VA_POS, GX_POS_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(VTXFMT, GX_VA_NRM, GX_NRM_XYZ, GX_F32, 0);
    GX_SetVtxAttrFmt(VTXFMT, GX_VA_CLR0, GX_CLR_RGBA, GX_RGBA8, 0);
    GX_SetVtxAttrFmt(VTXFMT, GX_VA_TEX0, GX_TEX_ST, GX_F32, 0);
    for (u32 k = 0; ok && k < nbody && k < NUM_KINDS; k++) {
        if (p + 36 > end) { ok = false; break; }
        BodyDef& def = s_defs[k];
        def.material = (int)rd32(p + 8);
        def.texA = (int)rd32(p + 12);
        def.texAWY = (int)rd32(p + 16);
        def.texB = (int)rd32(p + 20);
        def.wrapS = p[24];
        def.wrapT = p[25];
        u32 nb = rd32(p + 28), nv = rd32(p + 32);
        p += 36;
        if (p + nb * 72 + nv * VERT_BYTES > end) { ok = false; break; }
        def.bones.resize(nb);
        for (u32 i = 0; i < nb; i++) {
            BoneDef& b = def.bones[i];
            memcpy(b.name, p, 16);
            b.name[15] = 0;
            b.parent = (int)rd32(p + 16);
            for (int r = 0; r < 3; r++)
                for (int c = 0; c < 4; c++) b.rest.m[r][c] = rdf(p + 24 + (r * 4 + c) * 4);
            b.pos = Vec3(b.rest.m[0][3], b.rest.m[1][3], b.rest.m[2][3]);
            p += 72;
        }
        // tools/mii_body.py deja cada triángulo entero en un hueso: una display list por hueso
        std::vector<std::vector<const u8*>> geo(nb);
        for (u32 i = 0; i < nv; i++, p += VERT_BYTES)
            if (p[0] < nb) geo[p[0]].push_back(p);
        for (u32 i = 0; i < nb; i++) buildDL(def.bones[i], geo[i]);
    }
    free(d);
    if (!ok) freeAll();
    return ok;
}

bool available(int kind) { return kind >= 0 && kind < NUM_KINDS && !s_defs[kind].bones.empty(); }

void Body::build(int k, Color shirtCol, Color skinCol, int favColor) {
    kind = available(k) ? k : -1;
    shirt = (GXColor){(u8)colR(shirtCol), (u8)colG(shirtCol), (u8)colB(shirtCol), 255};
    skin = (GXColor){(u8)colR(skinCol), (u8)colG(skinCol), (u8)colB(skinCol), 255};
    if (favColor >= 0) {
        lightShirt = favColor == 10 || favColor == 2 || favColor == 3;  // blanco, amarillo, verde lima (main.dol)
    } else {
        float luma = (0.299f * shirt.r + 0.587f * shirt.g + 0.114f * shirt.b) / 255.0f;
        lightShirt = luma > tuning::MII_SHIRT_LIGHT_LUMA;
    }
}

float headWidth() { return tuning::MII_HEAD_UNITS * tuning::MII_BODY_UNIT; }

void draw(const Body& b, const Pose& p, Xf* head, Xf* waist) {
    if (!b.valid()) return;
    const BodyDef& def = s_defs[b.kind];
    const size_t nb = def.bones.size();
    float sq = clampv(p.squash, -0.3f, 0.3f);
    const float u = tuning::MII_BODY_UNIT;
    // Modelo (unidades de Resort, pies en el origen, mirando a +Z) -> mundo
    Xf toWorld = p.root * Xf().scale(Vec3(u * (1 + sq * 0.3f), u * (1 - sq), u * (1 + sq * 0.3f)));
    Xf toModel = inverse(toWorld);

    // Pose de cada hueso como transformación en espacio del modelo (P), de padres a hijos (vienen en ese orden)
    std::vector<Xf> P(nb);
    int chest = def.find("chest"), headB = def.find("head");
    for (size_t i = 0; i < nb; i++) {
        const BoneDef& bd = def.bones[i];
        Xf parent = bd.parent >= 0 ? P[bd.parent] : Xf();
        P[i] = parent;
        if ((int)i == chest) {
            P[i] = parent * translation(bd.pos) * Xf().rotateX(p.lean).rotateZ(p.side) * translation(-bd.pos);
        } else if ((int)i == headB) {
            P[i] = parent * translation(bd.pos) * Xf().rotateX(p.headPitch) * translation(-bd.pos);
        }
    }
    // Público (sin pecho): todo se inclina desde la base, y sus manos van a handL/handR
    if (chest < 0) {
        for (size_t i = 0; i < nb; i++) P[i] = Xf().rotateX(p.lean * 0.5f).rotateZ(p.side * 0.5f) * P[i];
        for (int s = -1; s <= 1; s += 2) {
            int h = def.find(s < 0 ? "handR" : "handL");
            if (h < 0 || !p.handsVisible) continue;
            Vec3 rest = def.bones[h].pos;
            Vec3 t = toModel.apply(s < 0 ? p.handL : p.handR);
            P[h] = translation(t) * rotationOf(P[h]) * translation(-rest);
        }
    }

    // Las mallas están en unidades de Resort: deshacer la escala en las normales
    const float nscale = u;
    setMaterial(def, b, def);
    for (size_t i = 0; i < nb; i++) {
        if (!def.bones[i].dl) continue;
        miilight::loadModelMtx(toWorld * P[i] * def.bones[i].rest, nscale);
        GX_CallDispList(def.bones[i].dl, def.bones[i].dlSize);
    }

    // Manos sueltas (jugadores): la bola de Resort, centrada en la posición de cada mano y con la ropa del cuerpo
    const BodyDef& hand = s_defs[HAND];
    if (def.material == MAT_PLAYER && available(HAND) && hand.bones[0].dl && p.handsVisible) {
        // Su material (MAT_HAND) es el del jugador con un último × blanco: vale el que ya está puesto
        const float hs = u * tuning::MII_HAND_SCALE;
        for (int s = -1; s <= 1; s += 2) {
            Vec3 c = s < 0 ? p.handL : p.handR;
            // El modelo es de la mano izquierda (dedos hacia +X): la derecha, en espejo
            Xf m = translation(c) * rotationOf(p.root) * rotationOf(chest >= 0 ? P[chest] : Xf()) *
                   Xf().scale(Vec3(hs * (s < 0 ? -1.0f : 1.0f), hs, hs)) * translation(Vec3(-tuning::MII_HAND_CX, 0, 0));
            miilight::loadModelMtx(m, hs);
            if (s < 0) GX_SetCullMode(GX_CULL_FRONT);  // el espejo da la vuelta a los triángulos
            GX_CallDispList(hand.bones[0].dl, hand.bones[0].dlSize);
            if (s < 0) GX_SetCullMode(GX_CULL_BACK);
        }
    }
    gfx::restore3D();

    if (head) {
        int hs = def.find("head_set");
        Vec3 hp = hs >= 0 ? def.bones[hs].pos : Vec3(0, 9.7f, 0);
        Xf hb = hs >= 0 ? P[hs] : Xf();
        Vec3 w = toWorld.apply(hb.apply(hp + Vec3(0, tuning::MII_HEAD_DY, tuning::MII_HEAD_DZ)));
        *head = translation(w) * rotationOf(p.root) * rotationOf(hb);
    }
    if (waist) {
        // Sigue al pecho (se inclina con él)
        Vec3 wp(0, tuning::MII_SASH_Y, 0);
        Xf pc = chest >= 0 ? P[chest] : Xf();
        *waist = translation(toWorld.apply(pc.apply(wp))) * rotationOf(p.root) * rotationOf(pc);
    }
}

}  // namespace miibody
