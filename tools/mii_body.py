"""Convierte los cuerpos de Mii de Wii Sports Resort (WS2_ava*.brres, MDL0 v11) al formato del juego (hk_body.bin).

Los modelos son de Nintendo y salen del disco del propio usuario (Common/Kokeshi/common.carc, ver docs/WII-NOTAS.md):
este script está en el repo, pero ni los .brres ni el .bin resultante se suben nunca.

Uso: python tools/mii_body.py <carpeta G3D> <salida hk_body.bin>

Formato (big endian, para la Wii):
  'HKB1', u32 numCuerpos
  por cuerpo: char[8] nombre, u32 numHuesos, u32 numVértices (múltiplo de 3: triángulos)
    huesos: char[16] nombre, s32 padre, u32 zona (no se usa), f32[12] matriz 3x4 de reposo en espacio del modelo
    vértices: u8 hueso, u8 zona (0 piel, 1 camiseta, 2 pantalón, 3 zapato), u8[2] relleno, f32[3] posición y f32[3]
              normal en el espacio del hueso
Cada vértice va pegado a un solo hueso (los cuerpos de Resort son casi todos rígidos; las pocas envolventes de dos huesos
se quedan con el primero), así el juego dibuja cada hueso como una malla con su matriz.

Como en Resort, los Miis no tienen brazos: el cuerpo de jugador es el de A1 sin brazos ni dedos (los del paracaidismo) y
las manos son un modelo suelto (hand1: la bola de los bolos; hand4, el puño que agarra, trae pegado un mango).
"""
import os
import struct
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from brres_mdl0 import Model, find_mdl0  # noqa: E402

# Cuerpos que usa el juego, en el orden de miibody::Kind
BODIES = [('player', 'WS2_avaA1'), ('crowd', 'WS2_avaC1'), ('hand', 'WS2_ava_hand1')]
ARM_BONES = ('clavicle', 'arm_', 'wrist', 'finger', 'thumb')
CROWD_BASE_Y = 3.4  # el público (torso de bala) tiene la base oscura, como en Resort


def zone(body, bone, model_y):
    n = bone.lower()
    if body == 'hand':
        return 0
    if body == 'crowd':
        if 'hand' in n:
            return 1  # manos del color de la camiseta
        return 2 if model_y < CROWD_BASE_Y else 1
    if 'head' in n:
        return 0
    if 'ankle' in n:
        return 3
    if 'leg' in n:  # la cadera (tripa) va con la camiseta, como en Resort
        return 2
    return 1


def convert(path):
    d = open(path, 'rb').read()
    _, o = find_mdl0(d)
    m = Model(d, o)
    bones = m.bones
    verts = []
    for obj in m.objs:
        pos = m.arrays['pos'][obj['ids'][0]]['data']
        nrm = m.arrays['nrm'][obj['ids'][1]]['data'] if obj['ids'][1] >= 0 else None
        for tri in m.triangles(obj):
            for mid, v in tri:
                p = np.array(list(pos[v['pos']]) + [1.0])
                n = np.array(nrm[v['nrm']]) if nrm is not None and 'nrm' in v else np.array([0, 1.0, 0])
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
                verts.append((b['index'], p[:3], n))
    # Triángulos entre dos huesos (dedos, cuello): todo al hueso más cercano a la raíz, en reposo. El juego no dobla
    # los dedos y el cuello apenas se mueve, así que se ven igual.
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
        for k, (bi, p, n) in enumerate(tri):
            w = bones[bi]['world']
            p2 = inv @ (w @ np.append(p, 1.0))
            n2 = inv[:3, :3] @ (w[:3, :3] @ n)
            verts[t + k] = (owner, p2[:3], n2 / (np.linalg.norm(n2) + 1e-9))
    return bones, verts


def main():
    src, out = sys.argv[1], sys.argv[2]
    blob = bytearray(b'HKB1' + struct.pack('>I', len(BODIES)))
    for name, file in BODIES:
        bones, verts = convert(f'{src}/{file}.brres')
        if name == 'player':  # sin brazos: fuera los triángulos de los huesos del brazo
            keep = []
            for t in range(0, len(verts), 3):
                if not any(k in bones[verts[t][0]]['name'] for k in ARM_BONES):
                    keep += verts[t:t + 3]
            verts = keep
        blob += name.encode().ljust(8, b'\0') + struct.pack('>II', len(bones), len(verts))
        for b in bones:
            blob += b['name'].encode()[:15].ljust(16, b'\0')
            blob += struct.pack('>iI', b['parent'], 0)
            blob += struct.pack('>12f', *b['world'][:3, :].flatten())
        for bi, p, n in verts:
            y = (bones[bi]['world'] @ np.append(p, 1.0))[1]
            blob += struct.pack('>BB2x6f', bi, zone(name, bones[bi]['name'], y), *p, *n)
        print(f'{name}: {file}, {len(bones)} huesos, {len(verts) // 3} triángulos')
    open(out, 'wb').write(blob)
    print(out, len(blob), 'bytes')


if __name__ == '__main__':
    main()
