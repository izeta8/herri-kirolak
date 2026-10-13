"""Recibe la telemetría del Wii Remote que envía el juego por WiFi (puerto 4300).

Uso: python tools/telemetry.py [IP de la Wii] [fichero de salida]
Guarda todas las lecturas en CSV y muestra un resumen cada medio segundo.
Se reconecta solo si el juego se reinicia.
"""
import socket
import sys
import time

ip = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.155"
out = sys.argv[2] if len(sys.argv) > 2 else "build/telemetria.csv"
STATES = ["idle", "subiendo", "ARMADO", "BAJANDO", "enfria"]

with open(out, "a", encoding="utf-8") as f:
    while True:
        try:
            s = socket.create_connection((ip, 4300), timeout=5)
        except OSError:
            print(f"esperando a la Wii en {ip}:4300 ...", flush=True)
            time.sleep(2)
            continue
        print("conectado", flush=True)
        f.write(f"# conexion {time.strftime('%H:%M:%S')}\n")
        s.settimeout(10)
        buf = b""
        last = time.time()
        peak = 0.0
        g = (0, 0, 0)
        st = 0
        chops = 0
        prev_plus = prev_minus = False
        mark_start = None
        gestures = 0
        prev_plus = prev_minus = False
        mark_start = None
        gestures = 0
        try:
            while True:
                data = s.recv(65536)
                if not data:
                    break
                buf += data
                *lines, buf = buf.split(b"\n")
                for line in lines:
                    t = line.decode(errors="ignore")
                    f.write(t + "\n")
                    if t.startswith("#"):
                        continue
                    p = t.split(",")
                    if len(p) < 10 or p[0] != "0":
                        continue
                    ax, ay, az = float(p[2]), float(p[3]), float(p[4])
                    peak = max(peak, (ax * ax + ay * ay + az * az) ** 0.5)
                    g = (float(p[5]), float(p[6]), float(p[7]))
                    st = int(p[8])
                    chops += int(p[9])
                    if len(p) > 10:
                        btn = int(p[10], 16)
                        plus, minus = bool(btn & 0x1000), bool(btn & 0x0010)
                        if plus and not prev_plus:
                            mark_start = int(p[1])
                            print(f"  + inicio gesto (ms {mark_start})", flush=True)
                        if minus and not prev_minus and mark_start is not None:
                            gestures += 1
                            print(f"  - fin gesto #{gestures} ({int(p[1]) - mark_start} ms)", flush=True)
                            mark_start = None
                        prev_plus, prev_minus = plus, minus
                    if len(p) > 10:
                        btn = int(p[10], 16)
                        plus, minus = bool(btn & 0x1000), bool(btn & 0x0010)
                        if plus and not prev_plus:
                            mark_start = int(p[1])
                            print(f"  + inicio gesto (ms {mark_start})", flush=True)
                        if minus and not prev_minus and mark_start is not None:
                            gestures += 1
                            print(f"  - fin gesto #{gestures} ({int(p[1]) - mark_start} ms)", flush=True)
                            mark_start = None
                        prev_plus, prev_minus = plus, minus
                now = time.time()
                if now - last > 0.5:
                    f.flush()
                    print(f"{time.strftime('%H:%M:%S')} grav x{g[0]:+.2f} y{g[1]:+.2f} z{g[2]:+.2f}  "
                          f"pico {peak:4.1f}g  estado {STATES[st] if st < 5 else st}  golpes {chops}", flush=True)
                    peak = 0.0
                    last = now
        except OSError as e:
            print("desconectado:", e, flush=True)
        s.close()
