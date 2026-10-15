"""Lector de texturas TEX0 (NW4R) de un BRRES de Wii: I4, I8, IA4, IA8, RGB565, RGB5A3, RGBA8 (sin paletas ni CMPR).
Uso suelto: python tools/brres_tex0.py fichero.brres carpeta_png  (vuelca cada textura en PNG, con alfa)"""
import os
import struct
import sys

import numpy as np

# formato GX -> (ancho de bloque, alto de bloque, bytes por bloque)
BLOCKS = {0: (8, 8, 32), 1: (8, 4, 32), 2: (8, 4, 32), 3: (4, 4, 32), 4: (4, 4, 32), 5: (4, 4, 32), 6: (4, 4, 64)}


def textures(d):
    """[(nombre, offset del TEX0)] de un BRRES"""
    from brres_mdl0 import group, u16
    for nm, o in group(d, u16(d, 0xC) + 8):
        if nm.startswith('Textures'):
            return group(d, o)
    return []


def decode(d, o):
    """Devuelve (formato, array RGBA uint8 alto x ancho x 4) del TEX0 en el offset o"""
    w, h = struct.unpack_from('>HH', d, o + 0x1C)
    fmt = struct.unpack_from('>I', d, o + 0x20)[0]
    p = o + struct.unpack_from('>i', d, o + 0x10)[0]
    if fmt not in BLOCKS:
        raise ValueError(f'formato {fmt} no soportado')
    bw, bh, bb = BLOCKS[fmt]
    img = np.zeros((h, w, 4), np.uint8)
    for by in range(0, h, bh):
        for bx in range(0, w, bw):
            blk = d[p:p + bb]
            p += bb
            for i in range(bw * bh):
                x, y = bx + i % bw, by + i // bw
                if fmt == 0:
                    v = (blk[i // 2] >> (4 if i % 2 == 0 else 0)) & 0xF
                    px = (v * 17,) * 3 + (255,)
                elif fmt == 1:
                    px = (blk[i],) * 3 + (255,)
                elif fmt == 2:
                    v = blk[i]
                    px = ((v & 0xF) * 17,) * 3 + ((v >> 4) * 17,)
                elif fmt == 3:
                    px = (blk[2 * i + 1],) * 3 + (blk[2 * i],)
                elif fmt == 4:
                    v = struct.unpack_from('>H', blk, 2 * i)[0]
                    px = ((v >> 11) * 255 // 31, ((v >> 5) & 63) * 255 // 63, (v & 31) * 255 // 31, 255)
                elif fmt == 5:
                    v = struct.unpack_from('>H', blk, 2 * i)[0]
                    if v & 0x8000:
                        px = (((v >> 10) & 31) * 255 // 31, ((v >> 5) & 31) * 255 // 31, (v & 31) * 255 // 31, 255)
                    else:
                        px = (((v >> 8) & 15) * 17, ((v >> 4) & 15) * 17, (v & 15) * 17, ((v >> 12) & 7) * 255 // 7)
                else:
                    px = (blk[2 * i + 1], blk[32 + 2 * i], blk[32 + 2 * i + 1], blk[2 * i])
                if x < w and y < h:
                    img[y, x] = px
    return fmt, img


if __name__ == '__main__':
    from PIL import Image
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    data = open(sys.argv[1], 'rb').read()
    os.makedirs(sys.argv[2], exist_ok=True)
    for name, off in textures(data):
        try:
            f, im = decode(data, off)
        except ValueError as e:
            print(name, e)
            continue
        Image.fromarray(im, 'RGBA').save(os.path.join(sys.argv[2], name + '.png'))
        print(name, f, im.shape[1], 'x', im.shape[0])
