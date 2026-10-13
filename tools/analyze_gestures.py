"""Analiza los gestos marcados con + / − en build/telemetria.csv (última conexión)."""
import math
import sys

path = sys.argv[1] if len(sys.argv) > 1 else "build/telemetria.csv"
step = int(sys.argv[2]) if len(sys.argv) > 2 else 5
rows = []
for line in open(path, encoding="utf-8"):
    if line.startswith("# conexion"):
        rows = []
        continue
    if line.startswith("#"):
        continue
    p = line.strip().split(",")
    if len(p) < 11 or p[0] != "0":
        continue
    rows.append(dict(ms=int(p[1]), a=tuple(map(float, p[2:5])), g=tuple(map(float, p[5:8])), st=int(p[8]), chop=int(p[9]), btn=int(p[10], 16)))

segs, start, pp, pm = [], None, False, False
for i, r in enumerate(rows):
    plus, minus = bool(r["btn"] & 0x1000), bool(r["btn"] & 0x0010)
    if plus and not pp:
        start = i
    if minus and not pm and start is not None:
        segs.append((start, i))
        start = None
    pp, pm = plus, minus

mag = lambda v: math.sqrt(sum(c * c for c in v))
for k, (s, e) in enumerate(segs):
    seg = rows[s:e + 1]
    t0 = seg[0]["ms"]
    pk = max(seg, key=lambda r: mag(r["a"]))
    top = min(seg, key=lambda r: r["g"][1])
    print(f"\n=== Gesto {k+1}: {seg[-1]['ms']-t0} ms, {len(seg)} lecturas; pico {mag(pk['a']):.2f}g a {pk['ms']-t0} ms; "
          f"golpe detectado={sum(r['chop'] for r in seg)}")
    print(f"  grav inicio {seg[0]['g']}  arriba(min gy) {top['g']} a {top['ms']-t0} ms  fin {seg[-1]['g']}")
    print("   ms    |a|    ax     ay     az   |  gx    gy    gz  st")
    for r in seg[::step]:
        a, g = r["a"], r["g"]
        print(f"  {r['ms']-t0:5d} {mag(a):5.2f} {a[0]:+6.2f} {a[1]:+6.2f} {a[2]:+6.2f} | {g[0]:+5.2f} {g[1]:+5.2f} {g[2]:+5.2f}  {r['st']}")
