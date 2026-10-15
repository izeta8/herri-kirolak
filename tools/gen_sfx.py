"""Genera los efectos de sonido del juego por síntesis (sin samples externos).

Salida: data/sfx_<nombre>.bin  — PCM 16 bits mono big-endian a 32000 Hz,
rellenado a múltiplo de 32 bytes (lo que pide ASND).
Uso: python tools/gen_sfx.py data
"""
import os
import sys

import numpy as np

SR = 32000
rng = np.random.default_rng(1234)


def t(dur):
    return np.arange(int(SR * dur)) / SR


def env(n, attack=0.005, decay=0.2):
    x = np.arange(n) / SR
    a = np.clip(x / max(attack, 1e-6), 0, 1)
    return a * np.exp(-x / decay)


def noise(dur):
    return rng.uniform(-1, 1, int(SR * dur))


def bandpass(x, lo, hi):
    f = np.fft.rfft(x)
    freqs = np.fft.rfftfreq(len(x), 1 / SR)
    f[(freqs < lo) | (freqs > hi)] = 0
    return np.fft.irfft(f, len(x))


def norm(x, peak=0.9):
    m = np.max(np.abs(x))
    return x * (peak / m) if m > 0 else x


def mix(*parts):
    n = max(len(p) for p in parts)
    out = np.zeros(n)
    for p in parts:
        out[: len(p)] += p
    return out


def sweep(f0, f1, dur):
    x = t(dur)
    f = np.linspace(f0, f1, len(x))
    return np.sin(2 * np.pi * np.cumsum(f) / SR)


def chop(strength=1.0):
    d = 0.45
    thump = np.sin(2 * np.pi * 95 * t(d)) * env(int(SR * d), 0.002, 0.07)
    knock = np.sin(2 * np.pi * 310 * t(d)) * env(int(SR * d), 0.001, 0.04)
    crack = bandpass(noise(d), 1200, 5000) * env(int(SR * d), 0.0005, 0.025)
    splinter = bandpass(noise(d), 2500, 9000) * env(int(SR * d), 0.01, 0.08) * 0.35
    return norm(thump * 1.0 + knock * 0.6 + crack * 1.4 * strength + splinter * strength, 0.95 * (0.55 + 0.45 * strength))


def split():
    d = 1.1
    crack = bandpass(noise(d), 600, 7000) * env(int(SR * d), 0.001, 0.12)
    creak = sweep(220, 90, d) * env(int(SR * d), 0.05, 0.25) * 0.4
    thud = np.sin(2 * np.pi * 70 * t(d)) * env(int(SR * d), 0.003, 0.18)
    return norm(crack * 1.2 + creak + thud, 0.95)


def cheer(dur=2.8):
    n = int(SR * dur)
    out = np.zeros(n)
    # Muchas "voces": bandas de ruido moduladas
    for _ in range(40):
        center = rng.uniform(300, 1800)
        v = bandpass(noise(dur), center * 0.8, center * 1.25)
        start = rng.uniform(0, 0.4)
        x = t(dur)
        e = np.clip((x - start) / 0.15, 0, 1) * np.clip((dur - x) / 1.2, 0, 1)
        wob = 1 + 0.3 * np.sin(2 * np.pi * rng.uniform(3, 7) * x + rng.uniform(0, 6))
        out += v * e * wob
    # Aplausos
    for _ in range(int(dur * 60)):
        p = int(rng.uniform(0, n - 2000))
        c = bandpass(noise(0.03), 800, 6000) * env(int(SR * 0.03), 0.0005, 0.006)
        out[p:p + len(c)] += c * rng.uniform(0.5, 1.5) * 3 * np.clip(1 - p / n, 0.2, 1)
    return norm(out, 0.8)


def clap(dur=1.6):
    n = int(SR * dur)
    out = np.zeros(n)
    for _ in range(int(dur * 45)):
        p = int(rng.uniform(0, n - 2000))
        c = bandpass(noise(0.03), 800, 6000) * env(int(SR * 0.03), 0.0005, 0.006)
        out[p:p + len(c)] += c * rng.uniform(0.5, 1.5) * np.clip(1.2 - p / n, 0, 1)
    return norm(out, 0.7)


def bell():
    d = 1.6
    x = t(d)
    parts = [(1.0, 1.0), (2.76, 0.5), (5.4, 0.25), (8.9, 0.12)]
    out = sum(a * np.sin(2 * np.pi * 660 * r * x) * np.exp(-x / (0.7 / r)) for r, a in parts)
    return norm(out * np.clip(x / 0.002, 0, 1), 0.8)


def beep(f, d):
    x = t(d)
    s = np.sin(2 * np.pi * f * x) + 0.3 * np.sin(2 * np.pi * 2 * f * x)
    return norm(s * np.clip(x / 0.004, 0, 1) * np.clip((d - x) / 0.04, 0, 1), 0.6)


def blip(f0, f1, d=0.07):
    x = t(d)
    return norm(sweep(f0, f1, d) * np.sin(np.pi * x / d) ** 0.5, 0.5)


def ok():
    return np.concatenate([blip(700, 900, 0.06), blip(1050, 1300, 0.09)])


def back():
    return np.concatenate([blip(900, 700, 0.06), blip(600, 450, 0.08)])


def whoosh():
    d = 0.3
    x = t(d)
    n = noise(d)
    e = np.sin(np.pi * x / d) ** 2
    lo = bandpass(n, 300, 1500)
    hi = bandpass(n, 1500, 4000)
    mixw = x / d
    return norm((lo * (1 - mixw) + hi * mixw) * e, 0.5)


def bad():
    d = 0.35
    thud = np.sin(2 * np.pi * 80 * t(d)) * env(int(SR * d), 0.002, 0.06)
    clank = (np.sin(2 * np.pi * 1700 * t(d)) + np.sin(2 * np.pi * 2350 * t(d))) * env(int(SR * d), 0.001, 0.05)
    return norm(thud + clank * 0.25, 0.7)


def pala():
    # Golpe seco de madera contra la pelota
    d = 0.18
    tok = (np.sin(2 * np.pi * 950 * t(d)) + 0.5 * np.sin(2 * np.pi * 1900 * t(d))) * env(int(SR * d), 0.0005, 0.022)
    click = bandpass(noise(d), 1500, 7000) * env(int(SR * d), 0.0003, 0.006)
    body = np.sin(2 * np.pi * 320 * t(d)) * env(int(SR * d), 0.001, 0.03) * 0.5
    return norm(tok + click * 1.2 + body, 0.9)


def frontis():
    # La pelota contra la piedra del frontis, con algo de eco del frontón
    d = 0.6
    slap = bandpass(noise(d), 300, 3500) * env(int(SR * d), 0.0005, 0.025)
    thump = np.sin(2 * np.pi * 180 * t(d)) * env(int(SR * d), 0.001, 0.04)
    echo = np.zeros(int(SR * d))
    lag = int(SR * 0.09)
    echo[lag:] = (slap * 0.35 + thump * 0.25)[: len(echo) - lag]
    tail = bandpass(noise(d), 200, 1500) * env(int(SR * d), 0.01, 0.15) * 0.12
    return norm(slap * 1.2 + thump + echo + tail, 0.85)


def bote():
    d = 0.12
    tk = np.sin(2 * np.pi * 420 * t(d)) * env(int(SR * d), 0.0005, 0.015)
    sn = bandpass(noise(d), 800, 4000) * env(int(SR * d), 0.0003, 0.005)
    return norm(tk + sn * 0.6, 0.6)


def write(path, x):
    pcm = (np.clip(x, -1, 1) * 32767).astype(">i2").tobytes()
    pcm += b"\0" * ((-len(pcm)) % 32)
    with open(path, "wb") as f:
        f.write(pcm)


SOUNDS = {
    "chop": lambda: chop(1.0),
    "chop_weak": lambda: chop(0.35),
    "split": split,
    "cheer": cheer,
    "clap": clap,
    "bell": bell,
    "beep": lambda: beep(880, 0.15),
    "go": lambda: beep(1320, 0.45),
    "move": lambda: blip(600, 850),
    "ok": ok,
    "back": back,
    "whoosh": whoosh,
    "bad": bad,
    "pala": pala,
    "frontis": frontis,
    "bote": bote,
}

if __name__ == "__main__":
    outdir = sys.argv[1]
    for name, fn in SOUNDS.items():
        p = os.path.join(outdir, f"sfx_{name}.bin")
        write(p, fn())
        print(p, os.path.getsize(p) // 1024, "KB")
