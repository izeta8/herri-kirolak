"""Compara gestos de izquierda (marcados con ←) y derecha (marcados con →) grabados por telemetría.

Cada gesto va entre dos pulsaciones de la misma flecha. Saca rasgos para ver cuál distingue los lados.
Uso: python tools/compare_sides.py [build/telemetria.csv]
"""
import math
import sys

path = sys.argv[1] if len(sys.argv) > 1 else "build/telemetria.csv"
rows = []
for line in open(path, encoding="utf-8"):
    if line.startswith("# conexion"):
        rows = []
        continue
    if line.startswith("#"):
        continue
    p = line.strip().split(",")
    if len(p) >= 11 and p[0] == "0":
        rows.append(dict(ms=int(p[1]), a=tuple(map(float, p[2:5])), g=tuple(map(float, p[5:8])), btn=int(p[10], 16)))

LEFT, RIGHT = 0x0100, 0x0200
segs = []
open_ = {LEFT: None, RIGHT: None}
prev = 0
for i, r in enumerate(rows):
    for b in (LEFT, RIGHT):
        if r["btn"] & b and not prev & b:
            if open_[b] is None:
                open_[b] = i
            else:
                segs.append(("L" if b == LEFT else "R", open_[b], i))
                open_[b] = None
    prev = r["btn"]

mag = lambda v: math.sqrt(sum(c * c for c in v))

def deg(g):
    d = math.degrees(math.atan2(-g[1], g[2]))
    return d + 360 if d < -90 else d

def features(s, e):
    seg = rows[s:e + 1]
    t0 = seg[0]["ms"]
    # parte alta: la lectura de gravedad con mayor ángulo de subida
    top = max(seg, key=lambda r: deg(r["g"]))
    pk_i = max(range(len(seg)), key=lambda i: mag(seg[i]["a"]))
    # ventana de bajada: desde que |a| pasa de 2 g (antes del pico) hasta que baja de 1.4 g tras el pico
    st = pk_i
    while st > 0 and mag(seg[st - 1]["a"]) > 2.0:
        st -= 1
    en = pk_i
    while en < len(seg) - 1 and mag(seg[en + 1]["a"]) > 1.4:
        en += 1
    sw = seg[st:en + 1]
    dt = 0.01
    ix = sum(r["a"][0] for r in sw) * dt
    iy = sum(r["a"][1] for r in sw) * dt
    iz = sum(r["a"][2] for r in sw) * dt
    first = sw[: max(1, len(sw) // 3)]
    fx = sum(r["a"][0] for r in first) / len(first)
    # la subida: desde el inicio hasta lo más alto
    up = seg[: seg.index(top) + 1]
    upx = sum(r["a"][0] for r in up) * dt
    return dict(dur=seg[-1]["ms"] - t0, topdeg=deg(top["g"]), topg=top["g"], peak=mag(seg[pk_i]["a"]),
                ix=ix, iy=iy, iz=iz, fx=fx, upx=upx, endg=seg[-1]["g"], n=len(sw))

print(f"{len(segs)} gestos: {sum(1 for s in segs if s[0]=='L')} izquierda, {sum(1 for s in segs if s[0]=='R')} derecha\n")
print("lado  dur  arriba°  grav_arriba(x,y,z)      pico | bajada: ∫ax    ∫ay    ∫az   ax_inicio | subida ∫ax | grav_fin")
by = {"L": [], "R": []}
for side, s, e in segs:
    f = features(s, e)
    by[side].append(f)
    tg, eg = f["topg"], f["endg"]
    print(f" {side}  {f['dur']:5d} {f['topdeg']:6.0f}  ({tg[0]:+.2f},{tg[1]:+.2f},{tg[2]:+.2f})  {f['peak']:4.1f} |"
          f" {f['ix']:+.2f} {f['iy']:+.2f} {f['iz']:+.2f}  {f['fx']:+.2f}    | {f['upx']:+.2f}     | ({eg[0]:+.2f},{eg[1]:+.2f},{eg[2]:+.2f})")
print()
for k in ["topdeg", "ix", "iy", "iz", "fx", "upx"]:
    L = [f[k] for f in by["L"]]
    R = [f[k] for f in by["R"]]
    if L and R:
        print(f"{k:7s} izquierda media {sum(L)/len(L):+7.2f} [{min(L):+.2f}..{max(L):+.2f}]   derecha media {sum(R)/len(R):+7.2f} [{min(R):+.2f}..{max(R):+.2f}]")
for k, idx in [("grav_arriba_x", 0), ("grav_fin_x", 0)]:
    key = "topg" if "arriba" in k else "endg"
    L = [f[key][idx] for f in by["L"]]
    R = [f[key][idx] for f in by["R"]]
    if L and R:
        print(f"{k:13s} izquierda media {sum(L)/len(L):+.2f} [{min(L):+.2f}..{max(L):+.2f}]   derecha media {sum(R)/len(R):+.2f} [{min(R):+.2f}..{max(R):+.2f}]")
