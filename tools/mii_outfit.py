"""Ropa vasca tradicional para los cuerpos de jugador de Resort (A6 hombre, B6 mujer), pintada aquí: arte propio.

Los cuerpos de Resort pintan pantalón, piernas y cadera con una sola UV por hueso (un color plano), así que les damos UV
propias: u = ángulo alrededor del cuerpo (en piernas y pies, alrededor de cada pierna; el frente en u = 0.5 y la costura
detrás) y v = altura. La textura tiene dos mitades: arriba el cuerpo, abajo la chaqueta abierta del hombre (es una pieza
aparte del modelo, por encima de la camisa). El material sigue siendo el de Resort:
  texA.rgb = la ropa, texA.a = 1 donde va el color del jugador (pañuelo, dibujo del chal), texB = 1 donde va la piel.
"""
import math

import numpy as np

H = 11.0                # altura del modelo (unidades de Resort): pies en 0, cuello del Mii en 11
TEX_W, TEX_H = 256, 512  # texA: 256 de vuelta x 2 x 256 de altura
SKIN_W, SKIN_H = 64, 128  # texB
LEG_BONES = ('leg_l', 'leg_r', 'ankle_l', 'ankle_r')
HAND_UV = (0.5, 0.004)  # la fila de arriba (altura 11) es piel en texB


def uv(bone, rest_pos, obj_index, model_pos, jacket_obj):
    """UV de un vértice: bone = nombre del hueso, rest_pos = origen de la pierna, model_pos = posición en reposo"""
    x, y, z = model_pos
    cx, cz = 0.0, 0.0
    if bone.startswith(LEG_BONES):
        cx = 0.75 if x > 0 else -0.75  # eje de cada pierna (las piernas de A6/B6 están en x = ±0.75)
    ang = math.atan2(x - cx, z - cz) / (2 * math.pi)  # 0 = de frente
    u = 0.5 + ang
    v = 0.5 * (1.0 - max(0.0, min(H, y)) / H)
    if obj_index == jacket_obj:
        v += 0.5
    return u, v


def fix_seams(uvs):
    """Triángulos que cruzan la costura de detrás (u de 0.9 a 0.1): +1 a los pequeños (la textura se repite en u)"""
    for t in range(0, len(uvs), 3):
        us = [uvs[t + k][0] for k in range(3)]
        if max(us) - min(us) > 0.5:
            for k in range(3):
                if uvs[t + k][0] < 0.5:
                    uvs[t + k] = (uvs[t + k][0] + 1.0, uvs[t + k][1])
    return uvs


def _grid():
    """Ángulo (−0.5..0.5 vueltas, 0 = frente), altura y mitad (0 cuerpo, 1 chaqueta) de cada texel de texA"""
    u = (np.arange(TEX_W) + 0.5) / TEX_W
    v = (np.arange(TEX_H) + 0.5) / TEX_H
    U, V = np.meshgrid(u, v)
    half = (V >= 0.5).astype(int)
    y = H * (1.0 - (V - 0.5 * half) * 2.0)
    return U - 0.5, y, half


def _rgb(c):
    return np.array([(c >> 16) & 255, (c >> 8) & 255, c & 255], float)


def _paint(img, mask, color, player=False):
    img[mask, :3] = _rgb(color)
    img[mask, 3] = 255 if player else 0


def male():
    """Aizkolari: chaqueta azul marino abierta, camisa blanca, pañuelo de cuadros (del color del jugador), pantalón
    azul claro a rayas hasta la rodilla, calcetines blancos con las cintas cruzadas de las abarkas y zapatos negros"""
    a, y, half = _grid()
    img = np.zeros((TEX_H, TEX_W, 4), float)
    skin = np.zeros((TEX_H, TEX_W), bool)
    body = half == 0
    front = np.abs(a)  # 0 de frente, 0.5 detrás
    # Cuerpo de abajo arriba
    _paint(img, body, 0x1C1C20)                                              # zapatos
    sock = body & (y > 0.55)
    _paint(img, sock, 0xF2EEE2)                                              # calcetín blanco
    # Cintas cruzadas: dos hélices en sentidos contrarios por la pierna
    turns = a * 2.0
    for sgn in (1, -1):
        ph = (turns * sgn + y / 0.62) % 1.0
        _paint(img, sock & (y < 2.55) & (np.minimum(ph, 1 - ph) < 0.07), 0x16161A)
    breeches = body & (y > 2.45)
    _paint(img, breeches, 0x9EAFD6)                                          # pantalón azul claro
    stripes = breeches & (((a * 64) % 1.0) < 0.3)
    _paint(img, stripes, 0x8495C2)                                           # rayas finas
    shirt = body & (y > 5.0)
    _paint(img, shirt, 0xF6F6F2)                                             # camisa blanca
    _paint(img, shirt & (front < 0.012) & (y < 8.7), 0xDCDCD6)              # tapeta de la camisa
    # Pañuelo al cuello: cuadros del color del jugador y blanco, con el nudo en pico por delante
    scarf = body & (y > 8.75) & (y < 9.6) | body & (front < 0.07) & (y > 7.9 + front * 10) & (y < 9.6)
    checks = ((np.floor(a * 48) + np.floor(y / 0.18)) % 2) == 0
    _paint(img, scarf, 0xB4BAC8)                                             # cuadros gris azulado
    _paint(img, scarf & checks, 0x000000, player=True)
    _paint(img, body & (y > 5.0) & (y < 6.0), 0xC4202A)                      # gerriko rojo
    skin |= body & (y > 9.6)                                                 # cuello
    # Chaqueta (la otra mitad): azul marino, con el borde algo más oscuro
    jacket = half == 1
    _paint(img, jacket, 0x1E2A4E)
    _paint(img, jacket & (y < 5.65), 0x17203C)                               # bajo de la chaqueta
    _paint(img, jacket & (front < 0.035), 0x17203C)                          # borde de la abertura
    return img, skin


def female():
    """Traje de casera: blusa blanca, chal oscuro con dibujo (del color del jugador) y flecos, delantal blanco con
    puntillas por delante, falda negra, medias negras y alpargatas blancas"""
    a, y, half = _grid()
    img = np.zeros((TEX_H, TEX_W, 4), float)
    skin = np.zeros((TEX_H, TEX_W), bool)
    body = half == 0
    front = np.abs(a)
    _paint(img, body, 0xF0EEE6)                                              # alpargatas blancas
    _paint(img, body & (y > 0.62) & (y < 0.75), 0x1A1A1E)                    # cinta de la alpargata
    _paint(img, body & (y > 0.75), 0x1A1A1E)                                 # medias y falda negras
    dress = body & (y > 4.9)
    # Delantal blanco por delante, con el bajo festoneado
    scallop = 5.45 + 0.12 * np.abs(np.sin(a * 2 * math.pi * 9))
    apron = dress & (front < 0.17) & (y > scallop)
    _paint(img, apron, 0xF7F5EE)
    _paint(img, apron & (front > 0.155), 0xE2DED2)                           # orilla del delantal
    # Blusa blanca: solo asoma en el escote
    _paint(img, dress & (y > 8.4), 0xF4F2EC)
    # Chal sobre hombros y espalda, cerrado en pico por delante, con dibujo del color del jugador y flecos
    bottom = 6.7 + 1.0 * front / 0.5
    neckline = (y > 8.4) & (front < (y - 8.4) * 0.09)
    shawl = dress & (y > bottom) & (y < 9.6) & ~neckline
    _paint(img, shawl, 0x26242C)
    dots = shawl & (((np.floor(a * 40) + np.floor(y / 0.22)) % 2) == 0) &         (((a * 40) % 1.0 - 0.5) ** 2 + ((y / 0.22) % 1.0 - 0.5) ** 2 < 0.06)
    _paint(img, dots, 0x000000, player=True)
    _paint(img, shawl & (y < bottom + 0.18), 0x3A3640)                       # orilla del chal
    fringe = dress & (y < bottom) & (y > bottom - 0.5) & (((a * 90) % 1.0) < 0.45)
    _paint(img, fringe, 0x2E2A30)                                            # flecos
    skin |= body & (y > 9.6)                                                 # cuello
    return img, skin


def skin_mask(skin):
    """texB a su tamaño: 255 donde va la piel"""
    small = skin.reshape(SKIN_H, TEX_H // SKIN_H, SKIN_W, TEX_W // SKIN_W).max(axis=(1, 3))
    m = np.zeros((SKIN_H, SKIN_W, 4), np.uint8)
    m[small] = 255
    return m


OUTFITS = {'player_m': male, 'player_f': female}
