"""Lector de MDL0 (NW4R, versión 11) de un BRRES de Wii: huesos, arrays, objetos con display lists y envolventes.
Uso suelto: python tools/brres_mdl0.py modelo.brres [salida.obj]  (lista huesos y objetos; exporta en pose de reposo)"""
import struct
import sys

import numpy as np


def u8(d, o): return d[o]
def u16(d, o): return struct.unpack_from('>H', d, o)[0]
def s16(d, o): return struct.unpack_from('>h', d, o)[0]
def u32(d, o): return struct.unpack_from('>I', d, o)[0]
def s32(d, o): return struct.unpack_from('>i', d, o)[0]
def f32(d, o): return struct.unpack_from('>f', d, o)[0]


def name_at(d, o):
    e = d.index(b'\0', o)
    return d[o:e].decode('ascii', 'replace')


def group(d, go):
    n = u32(d, go + 4)
    out = []
    for i in range(1, n + 1):
        e = go + 8 + i * 16
        out.append((name_at(d, go + s32(d, e + 8)), go + s32(d, e + 12)))
    return out


def find_mdl0(d):
    root = u16(d, 0xC)
    for nm, o in group(d, root + 8):
        if nm.startswith('3DModels'):
            return group(d, o)[0]
    raise SystemExit('sin MDL0')


def mtx34(d, o):
    m = np.array(struct.unpack_from('>12f', d, o), dtype=np.float64).reshape(3, 4)
    r = np.eye(4)
    r[:3, :] = m
    return r


class Model:
    def __init__(self, d, o):
        self.d, self.o = d, o
        secs = [s32(d, o + 0x10 + i * 4) for i in range(14)]
        (self.s_defs, self.s_bones, self.s_verts, self.s_norms, self.s_colors, self.s_uvs, _, _, self.s_mats,
         self.s_tev, self.s_objs, self.s_tex, _, _) = secs
        info = o + 0x10 + 14 * 4 + 4
        self.info = info
        mid = info + s32(d, info + 0x18)
        n = u32(d, mid)
        self.mtx_ids = [s32(d, mid + 4 + 4 * i) for i in range(n)]  # matriz -> hueso (o -1 si es envolvente)
        self.read_bones()
        self.read_defs()
        self.arrays = {}
        for kind, s in (('pos', self.s_verts), ('nrm', self.s_norms), ('clr', self.s_colors), ('uv', self.s_uvs)):
            self.arrays[kind] = [self.read_array(kind, a) for _, a in group(d, o + s)] if s else []
        self.objs = [self.read_obj(a, nm) for nm, a in group(d, o + self.s_objs)]

    def read_bones(self):
        d = self.d
        self.bones = []
        self.bone_by_off = {}
        for nm, b in group(d, self.o + self.s_bones):
            bone = dict(name=nm, off=b, index=u32(d, b + 0xC), mtx=u32(d, b + 0x10), flags=u32(d, b + 0x14),
                        scale=struct.unpack_from('>3f', d, b + 0x20), rot=struct.unpack_from('>3f', d, b + 0x2C),
                        trans=struct.unpack_from('>3f', d, b + 0x38), parent_rel=s32(d, b + 0x5C),
                        world=mtx34(d, b + 0x70), inv=mtx34(d, b + 0xA0))
            self.bones.append(bone)
            self.bone_by_off[b] = bone
        for bone in self.bones:
            p = bone['parent_rel']
            bone['parent'] = self.bone_by_off[bone['off'] + p]['index'] if p else -1
        self.bone_by_mtx = {b['mtx']: b for b in self.bones}

    def read_defs(self):
        """NodeMix: matrices envolventes = suma de pesos de huesos"""
        d = self.d
        self.envelopes = {}
        for nm, o in group(d, self.o + self.s_defs):
            if nm != 'NodeMix':
                continue
            p = o
            while True:
                op = d[p]
                if op == 1:
                    break
                if op == 3:
                    mid = u16(d, p + 1)
                    cnt = d[p + 3]
                    ws = []
                    for k in range(cnt):
                        q = p + 4 + k * 6
                        ws.append((u16(d, q), f32(d, q + 2)))
                    self.envelopes[mid] = ws
                    p += 4 + cnt * 6
                elif op == 5:
                    p += 5
                else:
                    raise SystemExit(f'NodeMix op {op}')

    def read_array(self, kind, a):
        d = self.d
        data = a + s32(d, a + 8)
        comp, fmt = u32(d, a + 0x14), u32(d, a + 0x18)
        if kind == 'clr':
            stride, cnt = d[a + 0x1C], u16(d, a + 0x1E)
            out = []
            for i in range(cnt):
                q = data + i * stride
                if fmt == 5:
                    out.append(tuple(d[q:q + 4]))
                elif fmt == 2:
                    out.append(tuple(d[q:q + 3]) + (255,))
                elif fmt == 1:
                    out.append(tuple(d[q:q + 3]) + (255,))
                else:
                    out.append((255, 255, 255, 255))
            return dict(fmt=fmt, comp=comp, data=out)
        div, stride, cnt = d[a + 0x1C], d[a + 0x1D], u16(d, a + 0x1E)
        ncomp = {'pos': 2 + comp, 'nrm': 3, 'uv': 1 + comp}[kind]
        if kind == 'nrm' and comp != 0:
            ncomp = 9
        size = {0: 1, 1: 1, 2: 2, 3: 2, 4: 4}[fmt]
        code = {0: 'B', 1: 'b', 2: 'H', 3: 'h', 4: 'f'}[fmt]
        sc = 1.0 / (1 << div) if fmt != 4 else 1.0
        out = []
        for i in range(cnt):
            vals = struct.unpack_from('>' + code * ncomp, d, data + i * stride)
            out.append(tuple(v * sc for v in vals))
        return dict(fmt=fmt, comp=comp, div=div, data=out)

    def read_obj(self, a, nm):
        d = self.d
        lo, hi = u32(d, a + 0x0C), u32(d, a + 0x10)
        dl_size = u32(d, a + 0x28)
        dl = a + 0x24 + s32(d, a + 0x2C)
        ids = [s16(d, a + 0x48 + 2 * i) for i in range(12)]
        node = s32(d, a + 0x08)
        wt = a + s32(d, a + 0x64)
        wcount = u16(d, wt)
        wtable = [u16(d, wt + 2 + 2 * i) for i in range(wcount)]
        attrs = []  # (nombre, tipo 0 nada / 1 directo / 2 idx8 / 3 idx16)
        if lo & 1:
            attrs.append(('pnmtx', 1))
        for t in range(8):
            if lo & (1 << (1 + t)):
                attrs.append((f'texmtx{t}', 1))
        for nm2, sh in (('pos', 9), ('nrm', 11), ('clr0', 13), ('clr1', 15)):
            v = (lo >> sh) & 3
            if v:
                attrs.append((nm2, v))
        for t in range(8):
            v = (hi >> (2 * t)) & 3
            if v:
                attrs.append((f'uv{t}', v))
        return dict(name=nm, attrs=attrs, dl=dl, dl_size=dl_size, ids=ids, node=node, wtable=wtable)

    def triangles(self, obj):
        """Devuelve lista de vértices [(slot de matriz, {attr: índice})] en triángulos"""
        d = self.d
        p, end = obj['dl'], obj['dl'] + obj['dl_size']
        slots = {}  # slot pnmtx (0..9) -> id de matriz
        tris = []
        while p < end:
            cmd = d[p]
            if cmd == 0:
                p += 1
                continue
            if cmd in (0x20, 0x28, 0x30, 0x38):
                idx, al = u16(d, p + 1), u16(d, p + 3)
                if cmd == 0x20:
                    slots[(al & 0xFFF) // 12] = idx
                p += 5
                continue
            prim = cmd & 0xF8
            n = u16(d, p + 1)
            p += 3
            vs = []
            for _ in range(n):
                v = {}
                for an, t in obj['attrs']:
                    if t == 1:
                        v[an] = d[p]
                        p += 1
                    elif t == 2:
                        v[an] = d[p]
                        p += 1
                    else:
                        v[an] = u16(d, p)
                        p += 2
                mid = slots.get(v['pnmtx'] // 3) if 'pnmtx' in v else (obj['node'] if obj['node'] >= 0 else None)
                vs.append((mid, v))
            if prim == 0x90:
                tris += [vs[i:i + 3] for i in range(0, n - 2, 3)]
            elif prim == 0x98:
                for i in range(n - 2):
                    tris.append([vs[i], vs[i + 1], vs[i + 2]] if i % 2 == 0 else [vs[i + 1], vs[i], vs[i + 2]])
            elif prim == 0xA0:
                tris += [[vs[0], vs[i], vs[i + 1]] for i in range(1, n - 1)]
            elif prim == 0x80:
                for i in range(0, n, 4):
                    tris += [[vs[i], vs[i + 1], vs[i + 2]], [vs[i], vs[i + 2], vs[i + 3]]]
            else:
                raise SystemExit(f'primitiva {cmd:#x} en {p:#x}')
        return tris

    def matrix(self, mid):
        """Matriz que lleva el vértice a espacio del modelo en pose de reposo"""
        if mid in self.envelopes:
            m = np.zeros((4, 4))
            for bmid, w in self.envelopes[mid]:
                b = self.bone_by_mtx[bmid]
                m += w * (b['world'] @ b['inv'])
            return m
        b = self.bone_by_mtx.get(mid)
        return b['world'] if b else np.eye(4)


def export_obj(m, path):
    lines, nv = [], 0
    pos = m.arrays['pos']
    for obj in m.objs:
        lines.append(f'o {obj["name"]}')
        tris = m.triangles(obj)
        arr = pos[obj['ids'][0]]['data']
        for tri in tris:
            for mid, v in tri:
                x = np.array(list(arr[v['pos']]) + [1.0])
                w = m.matrix(mid) @ x
                c = m.arrays['clr'][obj['ids'][2]]['data'][v['clr0']] if 'clr0' in v and m.arrays['clr'] else (128, 128, 128, 255)
                lines.append(f'v {w[0]:.4f} {w[1]:.4f} {w[2]:.4f} {c[0]/255:.3f} {c[1]/255:.3f} {c[3]/255:.3f}')
            lines.append(f'f {nv + 1} {nv + 2} {nv + 3}')
            nv += 3
    open(path, 'w').write('\n'.join(lines))


if __name__ == '__main__':
    d = open(sys.argv[1], 'rb').read()
    nm, o = find_mdl0(d)
    m = Model(d, o)
    print(nm, 'huesos', len(m.bones), 'envolventes', len(m.envelopes), 'objetos', len(m.objs))
    for b in m.bones:
        print(f"  {b['index']:2} {b['name']:12} mtx {b['mtx']:2} padre {b['parent']:2} t {tuple(round(x, 2) for x in b['trans'])}")
    for obj in m.objs:
        t = m.triangles(obj)
        print(' ', obj['name'], 'attrs', [a for a, _ in obj['attrs']], 'tris', len(t), 'node', obj['node'],
              'mtx usadas', sorted({mid for tri in t for mid, _ in tri if mid is not None})[:20])
    if len(sys.argv) > 2:
        export_obj(m, sys.argv[2])
