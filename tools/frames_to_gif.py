"""Junta los fotogramas volcados por Dolphin (PNG) en un GIF animado para enseñarlo en el móvil.

Uso: python tools/frames_to_gif.py <carpeta> <salida.gif> [primero] [último] [paso] [ancho]
  paso 4 sobre fotogramas de 60 Hz = 15 imágenes por segundo de juego.
"""
import glob
import os
import re
import sys

from PIL import Image


def main():
    src, out = sys.argv[1], sys.argv[2]
    first = int(sys.argv[3]) if len(sys.argv) > 3 else 0
    last = int(sys.argv[4]) if len(sys.argv) > 4 else -1
    step = int(sys.argv[5]) if len(sys.argv) > 5 else 4
    width = int(sys.argv[6]) if len(sys.argv) > 6 else 480
    frames = sorted(glob.glob(os.path.join(src, '**', '*.png'), recursive=True),
                    key=lambda p: int(re.findall(r'(\d+)\.png', p)[0]))
    if last < 0:
        last = len(frames)
    images = []
    for path in frames[first:last:step]:
        im = Image.open(path).convert('RGB')
        w, h = im.size
        im = im.resize((width, int(width * h / w)), Image.LANCZOS)
        images.append(im.quantize(colors=128, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.NONE))
    images[0].save(out, save_all=True, append_images=images[1:], duration=int(1000 * step / 60), loop=0, optimize=True)
    print(f'{len(images)} imágenes, {os.path.getsize(out) // 1024} KB -> {out}')


if __name__ == '__main__':
    main()
