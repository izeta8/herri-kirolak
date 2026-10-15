#pragma once
// Efectos de sonido con ASND (PCM 16 bits mono BE a 32 kHz, generados por tools/gen_sfx.py)

enum Sfx {
    SFX_CHOP,
    SFX_CHOP_WEAK,
    SFX_SPLIT,
    SFX_CHEER,
    SFX_CLAP,
    SFX_BELL,
    SFX_BEEP,
    SFX_GO,
    SFX_MOVE,
    SFX_OK,
    SFX_BACK,
    SFX_WHOOSH,
    SFX_BAD,
    SFX_PALA,
    SFX_FRONTIS,
    SFX_BOTE,
    SFX_COUNT
};

namespace audio {
void init();
// volume 0..1, pitch 1 = normal, pan -1 (izquierda) .. 1 (derecha)
void play(Sfx s, float volume = 1.0f, float pitch = 1.0f, float pan = 0.0f);
void stopAll();
}  // namespace audio
