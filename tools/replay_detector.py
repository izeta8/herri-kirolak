"""Pasa las lecturas grabadas por la telemetría por una réplica del detector de input.cpp.

Sirve para probar cambios de tuning.h con gestos reales sin tener que volver a la Wii.
Si los gestos están etiquetados (entre dos pulsaciones de ← = izquierda, → = derecha, o + / −),
dice si cada uno se detectó y con qué lado.

Uso: python tools/replay_detector.py [fichero.csv ...]
"""
import math
import re
import sys

T = {}
for name, val in re.findall(r"constexpr float (\w+)(?:\[3\])? = \{?([-\d.f, ]+)\}?;", open("src/engine/tuning.h", encoding="utf-8").read()):
    vals = [float(v.strip().rstrip("f")) for v in val.split(",") if v.strip()]
    T[name] = vals if len(vals) > 1 else vals[0]
CHOP_MIN = T["CHOP_MIN_G"][1]


def load(path):
    """Devuelve las lecturas de cada conexión del fichero."""
    sessions, rows = [], []
    for line in open(path, encoding="utf-8"):
        if line.startswith("# conexion"):
            if rows:
                sessions.append(rows)
            rows = []
            continue
        if line.startswith("#"):
            continue
        p = line.strip().split(",")
        if len(p) >= 11 and p[0] == "0":
            rows.append((int(p[1]), float(p[2]), float(p[3]), float(p[4]), int(p[10], 16)))
    if rows:
        sessions.append(rows)
    return sessions


def angle(y, z):
    d = math.degrees(math.atan2(y * T["RAISE_SIGN"], z))
    return d + 360 if d < -90 else d


def detect(rows):
    g = [0.0, 0.0, 1.0]
    state, timer, peak, lat = "IDLE", 0.0, 0.0, 0.0
    prev_ms = rows[0][0]
    chops = []
    for ms, ax, ay, az, _ in rows:
        dt = max((ms - prev_ms) / 1000.0, 0.001)
        prev_ms = ms
        m = math.sqrt(ax * ax + ay * ay + az * az)
        if abs(m - 1) < 0.3:
            k = math.exp(-10 * dt)
            g = [ax + (g[0] - ax) * k, ay + (g[1] - ay) * k, az + (g[2] - az) * k]
        deg = angle(g[1], g[2])
        raw_up = T["RAISE_RAW_MIN_G"] < m < T["RAISE_RAW_MAX_G"] and angle(ay, az) > T["RAISE_ON_DEG"]
        up = raw_up or deg > T["RAISE_ON_DEG"]
        if state == "IDLE":
            if up:
                state, timer = "RAISING", 0
        elif state == "RAISING":
            if up:
                timer += dt
            elif m < T["RAISE_RAW_MAX_G"] and deg < T["RAISE_OFF_DEG"]:
                timer -= dt * 0.5
            if timer < -0.3:
                state = "IDLE"
            elif timer >= T["RAISE_HOLD"]:
                state = "ARMED"
        elif state == "ARMED":
            if m > T["SWING_START_G"]:
                state, timer, peak, lat = "SWING", 0, m, 0
            elif deg < T["RAISE_OFF_DEG"] and not raw_up and abs(m - 1) < 0.2:
                state = "IDLE"
        elif state == "SWING":
            timer += dt
            peak = max(peak, m)
            lat += ax * dt
            if (m < T["SWING_END_G"] and timer > 0.04) or timer > T["SWING_MAX_TIME"]:
                if peak >= CHOP_MIN:
                    tilt = max(-1, min(1, (T["LAT_MID_GS"] - lat) / T["LAT_SCALE_GS"])) * T["TILT_SIGN"]
                    side = "R" if tilt > T["TILT_STRAIGHT"] else "L" if tilt < -T["TILT_STRAIGHT"] else "Z"
                    chops.append((ms, peak, lat, tilt, side))
                    state, timer = "COOLDOWN", T["CHOP_COOLDOWN"]
                else:
                    state = "ARMED"
        elif state == "COOLDOWN":
            timer -= dt
            if timer <= 0 and m < T["SWING_END_G"]:
                state = "IDLE"
    return chops


def segments(rows):
    """Gestos etiquetados: entre dos pulsaciones de la misma flecha (L/R) o de + y − (?)."""
    segs, open_, prev = [], {}, 0
    for ms, *_, btn in rows:
        for bit, lab in ((0x0100, "L"), (0x0200, "R")):
            if btn & bit and not prev & bit:
                if lab in open_:
                    segs.append((lab, open_.pop(lab), ms))
                else:
                    open_[lab] = ms
        if btn & 0x1000 and not prev & 0x1000:
            open_["?"] = ms
        if btn & 0x0010 and not prev & 0x0010 and "?" in open_:
            segs.append(("?", open_.pop("?"), ms))
        prev = btn
    return segs


total = ok = missed = 0
for path in sys.argv[1:] or ["build/telemetria.csv"]:
    for rows in load(path):
        chops = detect(rows)
        segs = segments(rows)
        if not segs:
            continue
        print(f"\n{path}: {len(segs)} gestos, {len(chops)} hachazos")
        for lab, a, b in segs:
            inside = [c for c in chops if a <= c[0] <= b + 300]
            total += 1
            if not inside:
                missed += 1
                print(f"  {lab}  NO DETECTADO")
                continue
            ms, peak, lat, tilt, side = inside[0]
            good = lab == "?" or side == lab
            ok += good
            print(f"  {lab}  -> {side} {'ok' if good else 'MAL'}   pico {peak:4.1f}g  empuje X {lat:+.2f}  tilt {tilt:+.2f}")
print(f"\nTotal: {total} gestos, {missed} sin detectar, {ok} con el lado bien")
