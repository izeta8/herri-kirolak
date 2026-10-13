"""Pasa las lecturas grabadas por la telemetría por una réplica del detector de input.cpp.

Sirve para probar cambios de tuning.h con gestos reales sin tener que volver a la Wii.
Uso: python tools/replay_detector.py [build/telemetria.csv]
"""
import math
import re
import sys

path = sys.argv[1] if len(sys.argv) > 1 else "build/telemetria.csv"
T = {}
for name, val in re.findall(r"constexpr float (\w+)(?:\[3\])? = \{?([-\d.f, ]+)\}?;", open("src/engine/tuning.h", encoding="utf-8").read()):
    vals = [float(v.strip().rstrip("f")) for v in val.split(",") if v.strip()]
    T[name] = vals if len(vals) > 1 else vals[0]
CHOP_MIN = T["CHOP_MIN_G"][1]

rows = []
for line in open(path, encoding="utf-8"):
    if line.startswith("# conexion"):
        rows = []
        continue
    if line.startswith("#"):
        continue
    p = line.strip().split(",")
    if len(p) >= 11 and p[0] == "0":
        rows.append((int(p[1]), float(p[2]), float(p[3]), float(p[4]), int(p[10], 16)))

g = [0.0, 0.0, 1.0]
state, timer, peak, lat, rollTop, gxTop = "IDLE", 0.0, 0.0, 0.0, 0.0, 0.0
prev_ms = rows[0][0] if rows else 0
gesture, pp, pm, inside = 0, False, False, False
chops = []
for ms, ax, ay, az, btn in rows:
    dt = max((ms - prev_ms) / 1000.0, 0.001)
    prev_ms = ms
    plus, minus = bool(btn & 0x1000), bool(btn & 0x0010)
    if plus and not pp:
        gesture += 1
        inside = True
    if minus and not pm:
        inside = False
    pp, pm = plus, minus
    m = math.sqrt(ax * ax + ay * ay + az * az)
    if abs(m - 1) < 0.3:
        k = math.exp(-10 * dt)
        g = [ax + (g[0] - ax) * k, ay + (g[1] - ay) * k, az + (g[2] - az) * k]
    deg = math.degrees(math.atan2(g[1] * T["RAISE_SIGN"], g[2]))
    if deg < -90:
        deg += 360
    roll = max(-1, min(1, g[0] / T["TILT_FULL_G"])) * T["TILT_SIGN"]
    if state == "IDLE":
        if deg > T["RAISE_ON_DEG"]:
            state, timer = "RAISING", 0
    elif state == "RAISING":
        timer += dt
        if deg < T["RAISE_ON_DEG"] - 15:
            state = "IDLE"
        elif timer >= T["RAISE_HOLD"]:
            state = "ARMED"
    elif state == "ARMED":
        if m > T["SWING_START_G"]:
            state, timer, peak, lat, rollTop, gxTop = "SWING", 0, m, 0, roll, g[0]
        elif deg < T["RAISE_OFF_DEG"]:
            state = "IDLE"
    elif state == "SWING":
        timer += dt
        peak = max(peak, m)
        lat += (ax - gxTop) * dt
        if (m < T["SWING_END_G"] and timer > 0.04) or timer > T["SWING_MAX_TIME"]:
            if peak >= CHOP_MIN:
                latPart = max(-1, min(1, lat / T["LAT_FULL_GS"])) * T["TILT_SIGN"] * T["LAT_SIGN"]
                tilt = max(-1, min(1, rollTop * T["ROLL_WEIGHT"] + latPart * (1 - T["ROLL_WEIGHT"])))
                side = "ESKUINA" if tilt > T["TILT_STRAIGHT"] else "EZKERRA" if tilt < -T["TILT_STRAIGHT"] else "ZUZEN"
                chops.append((gesture if inside else None, ms, peak, rollTop, latPart, tilt, side))
                state, timer = "COOLDOWN", T["CHOP_COOLDOWN"]
            else:
                state = "ARMED"
    elif state == "COOLDOWN":
        timer -= dt
        if timer <= 0 and m < T["SWING_END_G"]:
            state = "IDLE"

print(f"{gesture} gestos marcados, {len(chops)} hachazos detectados")
for gi, ms, peak, r, l, t, side in chops:
    where = f"gesto {gi}" if gi else "FUERA de un gesto"
    print(f"  {where:18s} pico {peak:4.1f}g  inclinacion {r:+.2f}  lateral {l:+.2f}  -> {t:+.2f} {side}")
