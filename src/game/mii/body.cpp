#include "game/mii/body.h"
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "engine/tuning.h"
#include "game/mii/mii.h"

namespace miibody {
namespace {

struct BoneDef {
    char name[16];
    int parent;
    Xf rest;    // matriz de reposo en espacio del modelo (unidades de Resort)
    Vec3 pos;   // origen del hueso en reposo
};

struct BodyDef {
    std::vector<BoneDef> bones;
    std::vector<std::vector<Vert>> geo;  // vértices de cada hueso en su espacio; en el color va la zona (0..3)
    int find(const char* n) const {
        for (size_t i = 0; i < bones.size(); i++)
            if (strcmp(bones[i].name, n) == 0) return (int)i;
        return -1;
    }
};

BodyDef s_defs[NUM_KINDS];

inline u32 rd32(const u8* p) { return ((u32)p[0] << 24) | ((u32)p[1] << 16) | ((u32)p[2] << 8) | p[3]; }
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

// Mallas de un cuerpo con los colores de cada zona
void buildParts(const BodyDef& def, const Color zoneColors[4], std::vector<Mesh>& out) {
    out.resize(def.bones.size());
    for (size_t i = 0; i < def.bones.size(); i++) {
        if (def.geo[i].empty()) continue;
        std::vector<Vert> v = def.geo[i];
        for (Vert& x : v) x.c = zoneColors[x.c & 3];
        out[i].build(v);
    }
}

}  // namespace

bool init() {
    u32 size = 0;
    u8* d = mii::readAsset("hk_body.bin", &size);
    if (!d) return false;
    bool ok = size > 8 && memcmp(d, "HKB1", 4) == 0;
    u32 n = ok ? rd32(d + 4) : 0;
    const u8* p = d + 8;
    for (u32 k = 0; ok && k < n && k < NUM_KINDS; k++) {
        BodyDef& def = s_defs[k];
        u32 nb = rd32(p + 8), nv = rd32(p + 12);
        p += 16;
        if (p + nb * 72 + nv * 28 > d + size) {
            ok = false;
            break;
        }
        def.bones.resize(nb);
        def.geo.assign(nb, {});
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
        for (u32 i = 0; i < nv; i++) {
            u32 bi = p[0];
            Vert v;
            v.p = Vec3(rdf(p + 4), rdf(p + 8), rdf(p + 12));
            v.n = Vec3(rdf(p + 16), rdf(p + 20), rdf(p + 24));
            v.c = p[1];  // zona
            if (bi < nb) def.geo[bi].push_back(v);
            p += 28;
        }
        // tools/mii_body.py deja cada triángulo entero en un hueso: aquí se agrupan de tres en tres
        for (auto& g : def.geo) g.resize(g.size() / 3 * 3);
    }
    free(d);
    if (!ok)
        for (auto& def : s_defs) def = BodyDef();
    return ok;
}

bool available(int kind) { return kind >= 0 && kind < NUM_KINDS && !s_defs[kind].bones.empty(); }

void Body::build(int k, const Color zoneColors[4], u8 height, u8 build) {
    release();
    if (!available(k)) return;
    kind = k;
    buildParts(s_defs[k], zoneColors, parts);
    if (k == PLAYER && available(HAND)) {
        std::vector<Mesh> h;
        buildParts(s_defs[HAND], zoneColors, h);
        if (!h.empty()) hand = h[0];
    }
    sy = tuning::MII_BODY_SY0 + tuning::MII_BODY_SY_RANGE * (height / 127.0f);
    sxz = tuning::MII_BODY_SXZ0 + tuning::MII_BODY_SXZ_RANGE * (build / 127.0f);
}

void Body::release() {
    for (Mesh& m : parts) m.release();
    parts.clear();
    hand.release();
}

float headWidth() { return tuning::MII_HEAD_UNITS * tuning::MII_BODY_UNIT; }

void draw(const Body& b, const Pose& p, Xf* head, Xf* waist) {
    if (!b.valid()) return;
    const BodyDef& def = s_defs[b.kind];
    const size_t nb = def.bones.size();
    float sq = clampv(p.squash, -0.3f, 0.3f);
    const float u = tuning::MII_BODY_UNIT;
    // Modelo (unidades de Resort, pies en el origen, mirando a +Z) -> mundo
    Xf toWorld = p.root * Xf().scale(Vec3(u * b.sxz * (1 + sq * 0.3f), u * b.sy * (1 - sq), u * b.sxz * (1 + sq * 0.3f)));
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

    // Las mallas están en unidades de Resort: deshacer la escala en las normales (si no, la luz las quema a blanco)
    const float nscale = u * cbrtf(b.sxz * b.sxz * b.sy);
    gfx::use3DFormat();
    for (size_t i = 0; i < nb; i++) {
        if (!b.parts[i].valid()) continue;
        gfx::loadModelMtx(toWorld * P[i] * def.bones[i].rest, nscale);
        b.parts[i].call();
    }

    // Manos sueltas (jugadores): la bola de Resort, centrada en la posición de cada mano
    if (b.hand.valid() && p.handsVisible) {
        const float hs = u * tuning::MII_HAND_SCALE;
        for (int s = -1; s <= 1; s += 2) {
            Vec3 c = s < 0 ? p.handL : p.handR;
            // El modelo es de la mano izquierda (dedos hacia +X): la derecha, en espejo
            Xf m = translation(c) * rotationOf(p.root) * rotationOf(chest >= 0 ? P[chest] : Xf()) *
                   Xf().scale(Vec3(hs * (s < 0 ? -1.0f : 1.0f), hs, hs)) * translation(Vec3(-tuning::MII_HAND_CX, 0, 0));
            gfx::loadModelMtx(m, hs);
            if (s < 0) GX_SetCullMode(GX_CULL_BACK);  // el espejo da la vuelta a los triángulos (el motor usa FRONT)
            b.hand.call();
            if (s < 0) GX_SetCullMode(GX_CULL_FRONT);
        }
    }

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
