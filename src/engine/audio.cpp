#include "engine/audio.h"
#include "engine/math.h"
#include <asndlib.h>
#include <gccore.h>

#include "sfx_back_bin.h"
#include "sfx_bad_bin.h"
#include "sfx_beep_bin.h"
#include "sfx_bote_bin.h"
#include "sfx_frontis_bin.h"
#include "sfx_pala_bin.h"
#include "sfx_bell_bin.h"
#include "sfx_cheer_bin.h"
#include "sfx_chop_bin.h"
#include "sfx_chop_weak_bin.h"
#include "sfx_clap_bin.h"
#include "sfx_go_bin.h"
#include "sfx_move_bin.h"
#include "sfx_ok_bin.h"
#include "sfx_split_bin.h"
#include "sfx_whoosh_bin.h"

namespace audio {

struct Sample {
    const u8* data;
    u32 size;
};

static Sample s_samples[SFX_COUNT];

void init() {
    ASND_Init();
    ASND_Pause(0);
    const Sample table[SFX_COUNT] = {
        {sfx_chop_bin, sfx_chop_bin_size},     {sfx_chop_weak_bin, sfx_chop_weak_bin_size},
        {sfx_split_bin, sfx_split_bin_size},   {sfx_cheer_bin, sfx_cheer_bin_size},
        {sfx_clap_bin, sfx_clap_bin_size},     {sfx_bell_bin, sfx_bell_bin_size},
        {sfx_beep_bin, sfx_beep_bin_size},     {sfx_go_bin, sfx_go_bin_size},
        {sfx_move_bin, sfx_move_bin_size},     {sfx_ok_bin, sfx_ok_bin_size},
        {sfx_back_bin, sfx_back_bin_size},     {sfx_whoosh_bin, sfx_whoosh_bin_size},
        {sfx_bad_bin, sfx_bad_bin_size},       {sfx_pala_bin, sfx_pala_bin_size},
        {sfx_frontis_bin, sfx_frontis_bin_size}, {sfx_bote_bin, sfx_bote_bin_size},
    };
    for (int i = 0; i < SFX_COUNT; i++) s_samples[i] = table[i];
}

void play(Sfx s, float volume, float pitch, float pan) {
    if (s < 0 || s >= SFX_COUNT) return;
    s32 voice = ASND_GetFirstUnusedVoice();
    if (voice < 0) return;
    int vol = clampv((int)(volume * 255), 0, 255);
    int l = clampv((int)(vol * (1.0f - fmaxf(0.0f, pan))), 0, 255);
    int r = clampv((int)(vol * (1.0f + fminf(0.0f, pan))), 0, 255);
    int freq = clampv((int)(32000 * pitch), 4000, 144000);
    ASND_SetVoice(voice, VOICE_MONO_16BIT_BE, freq, 0, (void*)s_samples[s].data, s_samples[s].size, l, r, nullptr);
}

void stopAll() {
    for (int i = 0; i < MAX_SND_VOICES; i++) ASND_StopVoice(i);
}

}  // namespace audio
