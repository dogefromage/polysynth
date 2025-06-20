#pragma once
#include "config.h"
#include "led.h"
#include "patch.h"

#define VOICE_COUNT 8

#define MIXER_SAW 1
#define MIXER_SQR 2

enum EnvelopeState {
    ENVELOPE_ATTACK,
    ENVELOPE_DECAY,
    ENVELOPE_RELEASE,
};

enum InstrumentSettings {
    INS_PITCHBEND,
    INS_MODWHEEL,
    INS_AFTERTOUCH,
    INS_PORTAMENTO,
    INS_VOLUME,
    INS_MOD_VCO,
    INS_MOD_VCF,
    INS_BEND_OCTAVE,
    INS__COUNT__,
};

class Envelope {
   public:
    float attack = 0, decay = 0, sustain = 1, release = 0, level = 0;
    EnvelopeState state = ENVELOPE_RELEASE;
    bool lastGate = false;
    void update(float dt, bool gate);
};

class Lfo {
   public:
    bool previousGate = false;
    float delayTime = 0, time = 0, x = 0, level = 0, frequency = 1, drift = 0, amplitude = 1;
    void update(float dt, bool gate);
};

struct TuningCorrection {
    // float slope = 1, intersept = 0;
    float parabolic[3] = {0, 1, 0};
};

void reset_correction(TuningCorrection& corr);

struct Voice {
    // scheduling
    // no two voices should be gated and contain the same note
    uint8_t note = 60, velocity = 127;
    bool gate = false;
    int schedulingTag = -1;  // higher signifies recent change

    Envelope env;
    Lfo lfo;

    // final values, uncorrected
    float out_pitch, out_cutoff, out_pulse,
        out_sub, out_resonance, out_amp;

    TuningCorrection pitch_correction, cutoff_correction;
    float volume_correction = 1;

    void update(float dt, Patch* patch, int16_t* settings, float syncedLfoLevel, float pitchBend, float modWheel);
};

class Instrument {
    Voice voices[VOICE_COUNT];
    int mixer = MIXER_SAW;
    int chorusType = 0;
    int schedulingTagCounter = 0;
    Lfo chorusLfoLeft, chorusLfoRight, syncedLfo;
    float mainVolume = 1;
    float modCenter, pitchBendCenter;

    int unisonDivisor = 1;

    PanelLedController& leds;

    int16_t settings[INS__COUNT__];
    Patch patch;

    float measureFrequency(int voiceIndex, float semis, bool isFilter);
    int findTuningProfile(int voiceIndex, float semis_a, float semis_b, bool isFilter);

   public:
    Instrument(PanelLedController& leds);
    Patch& getPatch();
    Voice& getVoice(int i);
    int16_t* getSettings();

    void load_tuning();
    void tune();
    void testTuning();
    void update(float dt);
    void write();
    void scheduleNoteOn(int note, int velocity);
    void scheduleNoteOff(int note);
    void allNotesOff();

    void test();
    void testChorus();

    int getUnisonDivisor();
    void setUnisonDivisor(int newDivisor);

    Instrument(const Instrument&) = delete;

    friend void dacs_write(Instrument* inst);
};
