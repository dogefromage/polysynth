#pragma once
#include <stdint.h>

// LAYOUT FOR THIS SYNTH:

enum PatchFaders {
    FD_LFO_RATE,
    FD_LFO_DELAY,
    FD_VIBRATO,
    FD_PULSE_WIDTH,
    FD_SUB_OSCILLATOR,
    FD_CUTOFF,
    FD_RESONANCE,
    FD_FILTER_ENVELOPE,
    FD_FILTER_LFO,
    FD_FILTER_KEYTRACK,
    FD_ATTACK,
    FD_DECAY,
    FD_SUSTAIN,
    FD_RELEASE,
    PATCH_FD__COUNT__,
};

enum PatchSwitches {
    SW_AMP_SHAPE,
    SW_VCO_PWM_SOURCE,
    SW_LFO_SYNC,
    SW_VCO_SQUARE,
    SW_VCO_SAW,
    SW_CHORUS_I,
    SW_CHORUS_II,
    PATCH_SW__COUNT__
};

class Patch {
public:

    // faders range from 0 to 1023
    int16_t faders[PATCH_FD__COUNT__] = {};
    // switches range from 0 to n-1 for n position switches
    int8_t switches[PATCH_SW__COUNT__] = {};
};