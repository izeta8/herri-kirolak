"""Convierte una fuente TTF en un atlas IA8 para GX (Wii) + métricas.

I (intensidad) = relleno de la letra, A (alfa) = relleno + contorno.
Así el juego dibuja texto con contorno en una sola pasada (TEV mezcla color de
contorno y de relleno según I).

Salida (big-endian, como la Wii):
  'FNT1' u16 texW u16 texH u16 size s16 ascent s16 descent s16 lineHeight u16 nGlyphs u16 pad
  nGlyphs × { u16 code, u16 x, u16 y, u16 w, u16 h, s16 xoff, s16 yoff, s16 advance }
  relleno hasta múltiplo de 32
  textura IA8 en bloques 4×4

Uso: python tools/bake_font.py assets/fonts/Nunito-Variable.ttf data/font.bin
"""
import struct
import sys

from PIL import Image, ImageDraw, ImageFont

SIZE = 44
WEIGHT = 850
STROKE = 4
PAD = 2
TEX_W = 1024

CHARS = [chr(c) for c in range(32, 127)] + list("¡¿ÁÉÍÓÚÑÜáéíóúñü·ªº×")


def main(src, dst):
    font = ImageFont.truetype(src, SIZE)
    font.set_variation_by_axes([WEIGHT])
    ascent, descent = font.getmetrics()

    glyphs = []
    for ch in CHARS:
        x0, y0, x1, y1 = font.getbbox(ch, anchor="ls", stroke_width=STROKE)
        w, h = max(0, x1 - x0), max(0, y1 - y0)
        adv = font.getlength(ch)
        full = Image.new("L", (w + 2 * PAD, h + 2 * PAD), 0)
        fill = Image.new("L", full.size, 0)
        if w and h:
            org = (PAD - x0, PAD - y0)
            ImageDraw.Draw(full).text(org, ch, font=font, fill=255, anchor="ls",
                                      stroke_width=STROKE, stroke_fill=255)
            ImageDraw.Draw(fill).text(org, ch, font=font, fill=255, anchor="ls")
        glyphs.append(dict(ch=ch, full=full, fill=fill, xoff=x0 - PAD, yoff=y0 - PAD, adv=adv))

    # Empaquetado por estanterías, de más alto a más bajo
    order = sorted(range(len(glyphs)), key=lambda i: -glyphs[i]["full"].size[1])
    x = y = shelf_h = 0
    for i in order:
        g = glyphs[i]
        w, h = g["full"].size
        if x + w > TEX_W:
            x, y = 0, y + shelf_h
            shelf_h = 0
        g["x"], g["y"] = x, y
        x += w
        shelf_h = max(shelf_h, h)
    tex_h = 1
    while tex_h < y + shelf_h:
        tex_h *= 2

    alpha = Image.new("L", (TEX_W, tex_h), 0)
    inten = Image.new("L", (TEX_W, tex_h), 0)
    for g in glyphs:
        alpha.paste(g["full"], (g["x"], g["y"]))
        inten.paste(g["fill"], (g["x"], g["y"]))

    out = bytearray()
    out += b"FNT1"
    out += struct.pack(">HHHhhhHH", TEX_W, tex_h, SIZE, ascent, descent, ascent + descent, len(glyphs), 0)
    for g in glyphs:
        w, h = g["full"].size
        out += struct.pack(">HHHHHhhh", ord(g["ch"]), g["x"], g["y"], w, h, g["xoff"], g["yoff"], round(g["adv"]))
    while len(out) % 32:
        out.append(0)

    a, i = alpha.load(), inten.load()
    for ty in range(0, tex_h, 4):
        for tx in range(0, TEX_W, 4):
            for yy in range(4):
                for xx in range(4):
                    out.append(a[tx + xx, ty + yy])
                    out.append(i[tx + xx, ty + yy])

    with open(dst, "wb") as f:
        f.write(out)
    print(f"{dst}: {len(glyphs)} glifos, textura {TEX_W}x{tex_h}, {len(out) // 1024} KB")


if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
