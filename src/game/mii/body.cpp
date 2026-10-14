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
    int zone;
    Xf rest;    // matriz de reposo en espacio del modelo (unidades de Resort)
    Vec3 pos;   // origen del hueso en reposo
};

struct BodyDef {
    std::vector<BoneDef> bones;
    std::vector<std::vector<Vert>> geo;  // vértices de cada hueso en su espacio (color: blanco)
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

// Rotación mínima que lleva la dirección a a la b (las dos normalizadas)
Xf rotFromTo(const Vec3& a, const Vec3& b) {
    Vec3 axis = cross(a, b);
    float s = length(axis), c = dot(a, b);
    if (s < 1e-5f) {
        if (c > 0) return Xf();
        Vec3 ortho = fabsf(a.y) < 0.9f ? Vec3(0, 1, 0) : Vec3(1, 0, 0);
        return Xf().rotateAxis(normalize(cross(a, ortho)), PI);
    }
    return Xf().rotateAxis(axis * (1.0f / s), atan2f(s, c));
}

// Solo la rotación de una matriz (sin traslación)
Xf rotationOf(const Xf& x) {
    Xf r = x;
    r.m[0][3] = r.m[1][3] = r.m[2][3] = 0;
    return r;
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
            b.zone = (int)rd32(p + 20);
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
            v.c = rgba(255, 255, 255);
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
    const BodyDef& def = s_defs[k];
    parts.resize(def.bones.size());
    for (size_t i = 0; i < def.bones.size(); i++) {
        if (def.geo[i].empty()) continue;
        std::vector<Vert> v = def.geo[i];
        Color c = zoneColors[def.bones[i].zone & 3];
        for (Vert& x : v) x.c = c;
        parts[i].build(v);
    }
    sy = tuning::MII_BODY_SY0 + tuning::MII_BODY_SY_RANGE * (height / 127.0f);
    sxz = tuning::MII_BODY_SXZ0 + tuning::MII_BODY_SXZ_RANGE * (build / 127.0f);
}

void Body::release() {
    for (Mesh& m : parts) m.release();
    parts.clear();
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
    // Cuerpo sin pecho (público): todo se inclina desde los pies
    if (chest < 0)
        for (size_t i = 0; i < nb; i++) P[i] = Xf().rotateX(p.lean * 0.5f).rotateZ(p.side * 0.5f) * P[i];

    // Brazos: IK de dos huesos hasta la mano (s = -1 mano de la izquierda de la pantalla = brazo "_r" de Resort)
    for (int s = -1; s <= 1; s += 2) {
        const char* sfx = s < 0 ? "_r" : "_l";
        char n1[16], n2[16], nw[16];
        snprintf(n1, sizeof(n1), "arm%s1", sfx);
        snprintf(n2, sizeof(n2), "arm%s2", sfx);
        snprintf(nw, sizeof(nw), "wrist%s", sfx);
        int a1 = def.find(n1), a2 = def.find(n2), wr = def.find(nw);
        Vec3 handWorld = s < 0 ? p.handL : p.handR;
        if (a1 < 0 || a2 < 0 || wr < 0) {
            // Público: manos sueltas (handL/handR o hand_l/hand_r)
            int h = def.find(s < 0 ? "handR" : "handL");
            if (h < 0) h = def.find(s < 0 ? "hand_r" : "hand_l");
            if (h >= 0) {
                Vec3 rest = def.bones[h].pos;
                Vec3 t = p.handsVisible ? toModel.apply(handWorld) : P[h].apply(rest);
                P[h] = translation(t) * rotationOf(P[h]) * translation(-rest);
            }
            continue;
        }
        const Xf& base = P[def.bones[a1].parent >= 0 ? def.bones[a1].parent : 0];
        Vec3 sR = def.bones[a1].pos, eR = def.bones[a2].pos, wR = def.bones[wr].pos;
        Vec3 d1R = normalize(eR - sR), d2R = normalize(wR - eR);
        float L1 = length(eR - sR), L2 = length(wR - eR) + tuning::MII_HAND_REACH;
        // Objetivo en el espacio de reposo del brazo (deshaciendo la inclinación del pecho)
        Vec3 t;
        if (p.handsVisible) {
            t = inverse(base).apply(toModel.apply(handWorld));
        } else {
            t = sR + Vec3(s * -0.2f, -1, 0.1f) * (L1 + L2 - 0.2f);  // brazos caídos
        }
        Vec3 dt = t - sR;
        float D = clampv(length(dt), 0.3f, L1 + L2 - 0.01f);
        Vec3 dir = normalize(dt);
        float ca = clampv((L1 * L1 + D * D - L2 * L2) / (2 * L1 * D), -1.0f, 1.0f);
        Vec3 pole(s * -tuning::MII_ELBOW_OUT, -1, -tuning::MII_ELBOW_BACK);
        Vec3 perp = normalize(pole - dir * dot(pole, dir));
        Vec3 elbow = sR + dir * (ca * L1) + perp * (sqrtf(1 - ca * ca) * L1);
        Vec3 hand = sR + dir * D;
        Xf R1 = rotFromTo(d1R, normalize(elbow - sR));
        Xf R2 = rotFromTo(normalize(R1.applyDir(d2R)), normalize(hand - elbow)) * R1;
        P[a1] = base * translation(sR) * R1 * translation(-sR);
        P[a2] = base * translation(elbow) * R2 * translation(-eR);
        // La mano y los dedos siguen al antebrazo
        for (size_t i = a2 + 1; i < nb; i++) {
            int q = def.bones[i].parent;
            while (q >= 0 && q != a2) q = def.bones[q].parent;
            if (q == a2) P[i] = P[a2];
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
