"""Convierte los cuerpos de Mii de Wii Sports Resort (WS2_ava*.brres, MDL0 v11) al formato del juego (hk_body.bin).

Los modelos y texturas son de Nintendo y salen del disco del propio usuario (Common/Kokeshi/common.carc, ver
docs/WII-NOTAS.md): este script está en el repo, pero ni los .brres ni el .bin resultante se suben nunca.

Uso: python tools/mii_body.py <carpeta G3D> <salida hk_body.bin>

Formato (big endian, para la Wii):
  'HKB2', u32 numTexturas, u32 numCuerpos
  por textura: char[24] nombre, u32 formato GX, u16 ancho, u16 alto, u32 bytes, datos GX tal cual (relleno a 32)
  por cuerpo: char[8] nombre, u32 material, s32 texA, s32 texAWY, s32 texB (índices de textura o -1),
              u8 repetición S y T de texA (GX: 0 recorta, 1 repite, 2 espejo), u8[2] relleno,
              u32 numHuesos, u32 numVértices (múltiplo de 3: triángulos)
    huesos: char[16] nombre, s32 padre, u32 0, f32[12] matriz 3x4 de reposo en espacio del modelo
    vértices: u8 hueso, u8 alfa (oclusión: pesa las lightmaps), u8[2] relleno, f32[3] posición y f32[3] normal en el
              espacio del hueso, f32[2] UV
Cada vértice va pegado a un solo hueso (los cuerpos de Resort son casi todos rígidos; las pocas envolventes de dos huesos
se quedan con el primero), así el juego dibuja cada hueso como una malla con su matriz.

Materiales (el TEV de cada modelo, ver docs/WII-NOTAS.md "Material de los Miis"):
  0 jugador: ropa = lerp(texA.rgb, favorito, texA.a), luego lerp(·, piel, texB.i)
  1 público: lerp(piel, favorito, texA.i)
  2 mano: como el jugador, con la ropa del cuerpo (la UV de la mano cae en la piel)
Ropa de Resort: en los bolos, A6 (hombre) y B6 (mujer), sin brazos; texA "_awy" es la de camiseta blanca o amarilla.
"""
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from brres_mdl0 import Model, find_mdl0  # noqa: E402
from brres_tex0 import BLOCKS, textures  # noqa: E402

CLOTHES = 'clothes.brres'
# (nombre en el juego, modelo, material, (texA, texAWY, texB) y de qué brres salen), en el orden de miibody::Kind
BODIES = [
    ('player_m', 'WS2_avaA6', 0, ('WS2bodyA6_tex01_a', 'WS2bodyA6_tex01_awy', 'WS2bodyA6_tex01_b'), CLOTHES),
    ('player_f', 'WS2_avaB6', 0, ('WS2bodyB6_tex01_a', 'WS2bodyB6_tex01_awy', 'WS2bodyB6_tex01_b'), CLOTHES),
    ('crowd', 'WS2_avaC1', 1, ('bodyC1_tex01_a', None, None), None),
    ('hand', 'WS2_ava_hand1', 2, (None, None, None), None),
]


def tex_bytes(d, off):
    w, h = struct.unpack_from('>HH', d, off + 0x1C)
    fmt = struct.unpack_from('>I', d, off + 0x20)[0]
    bw, bh, bb = BLOCKS[fmt]
    size = ((w + bw - 1) // bw) * ((h + bh - 1) // bh) * bb
    p = off + struct.unpack_from('>i', d, off + 0x10)[0]
    return fmt, w, h, d[p:p + size]


def wrap_tex0(d, m):
    """Repetición (S, T) de la primera textura del material (la de la ropa)"""
    from brres_mdl0 import group, s32, u32
    _, a = group(d, m.o + m.s_mats)[0]
    t = a + s32(d, a + 0x30)
    return u32(d, t + 0x18), u32(d, t + 0x1C)


def convert(path):
    d = open(path, 'rb').read()
    _, o = find_mdl0(d)
    m = Model(d, o)
    wrap = wrap_tex0(d, m)
    bones = m.bones
    verts = []
    for obj in m.objs:
        ids = obj['ids']
        pos = m.arrays['pos'][ids[0]]['data']
        nrm = m.arrays['nrm'][ids[1]]['data'] if ids[1] >= 0 else None
        clr = m.arrays['clr'][ids[2]]['data'] if ids[2] >= 0 and m.arrays['clr'] else None
        uvs = m.arrays['uv'][ids[4]]['data'] if ids[4] >= 0 and m.arrays['uv'] else None
        for tri in m.triangles(obj):
            for mid, v in tri:
                p = np.array(list(pos[v['pos']]) + [1.0])
                n = np.array(nrm[v['nrm']]) if nrm is not None and 'nrm' in v else np.array([0, 1.0, 0])
                alpha = clr[v['clr0']][3] if clr is not None and 'clr0' in v else 255
                uv = uvs[v['uv0']] if uvs is not None and 'uv0' in v else (0.0, 0.0)
                if mid in m.envelopes:  # en espacio del modelo: pasar al del primer hueso
                    b = m.bone_by_mtx[m.envelopes[mid][0][0]]
                    inv = np.linalg.inv(b['world'])
                    p = inv @ p
                    n = inv[:3, :3] @ n
                else:
                    b = m.bone_by_mtx.get(mid) if mid is not None else bones[0]
                    if b is None:
                        b = bones[0]
                n = n / (np.linalg.norm(n) + 1e-9)
                verts.append((b['index'], p[:3], n, alpha, uv))
    # Triángulos entre dos huesos (cuello, tobillos): todo al hueso más cercano a la raíz, en reposo. El juego apenas
    # dobla esas articulaciones, así que se ven igual.
    def depth(i):
        k = 0
        while bones[i]['parent'] >= 0:
            i, k = bones[i]['parent'], k + 1
        return k
    for t in range(0, len(verts), 3):
        tri = verts[t:t + 3]
        owner = min((v[0] for v in tri), key=depth)
        if all(v[0] == owner for v in tri):
            continue
        inv = np.linalg.inv(bones[owner]['world'])
        for k, (bi, p, n, a, uv) in enumerate(tri):
            w = bones[bi]['world']
            p2 = inv @ (w @ np.append(p, 1.0))
            n2 = inv[:3, :3] @ (w[:3, :3] @ n)
            verts[t + k] = (owner, p2[:3], n2 / (np.linalg.norm(n2) + 1e-9), a, uv)
    return bones, verts, wrap


def main():
    src, out = sys.argv[1], sys.argv[2]
    texs, tex_index = [], {}

    def add_tex(brres, name):
        if name is None:
            return -1
        if name not in tex_index:
            d = open(f'{src}/{brres}', 'rb').read()
            offs = dict(textures(d))
            if name not in offs:
                raise SystemExit(f'falta la textura {name} en {brres}')
            tex_index[name] = len(texs)
            texs.append((name,) + tex_bytes(d, offs[name]))
        return tex_index[name]

    bodies = []
    for name, file, mat, tnames, tsrc in BODIES:
        ti = [add_tex(tsrc or f'{file}.brres', t) for t in tnames]
        bones, verts, wrap = convert(f'{src}/{file}.brres')
        bodies.append((name, mat, ti, wrap, bones, verts))
        print(f'{name}: {file}, {len(bones)} huesos, {len(verts) // 3} triángulos, texturas {tnames}')

    blob = bytearray(b'HKB2' + struct.pack('>II', len(texs), len(bodies)))
    for name, fmt, w, h, data in texs:
        blob += name.encode()[:23].ljust(24, b'\0') + struct.pack('>IHHI', fmt, w, h, len(data))
        blob += data + b'\0' * (-len(data) % 32)
    for name, mat, ti, wrap, bones, verts in bodies:
        blob += name.encode().ljust(8, b'\0') + struct.pack('>I3iBB2xII', mat, *ti, *wrap, len(bones), len(verts))
        for b in bones:
            blob += b['name'].encode()[:15].ljust(16, b'\0')
            blob += struct.pack('>iI', b['parent'], 0)
            blob += struct.pack('>12f', *b['world'][:3, :].flatten())
        for bi, p, n, a, uv in verts:
            blob += struct.pack('>BB2x8f', bi, a, *p, *n, *uv)
    open(out, 'wb').write(blob)
    print(out, len(blob), 'bytes,', len(texs), 'texturas')


if __name__ == '__main__':
    main()
