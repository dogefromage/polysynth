#include <Arduino.h>

#include "config.h"
#include "dacs.h"
#include "instrument.h"
#include "linalg.h"
#include "memory.h"
#include "utils.h"

static float idealFrequencyToSemis(float freq) {
    float C0 = 16.35160;
    return 12.0 / M_LN2 * logf(freq / C0);
}

volatile static unsigned long microsTuningStart, microsTuningEnd, tuningCycles, totalTuningCycles;
volatile static bool isTuning;

// #define TUNING_CYCLES 100

static void measurePitchSubroutine() {
    if (tuningCycles == 0) {
        microsTuningStart = micros();
    }
    if (tuningCycles >= totalTuningCycles) {
        microsTuningEnd = micros();
        detachInterrupt(digitalPinToInterrupt(PIN_AUDIO_LOOPBACK));
        isTuning = false;
    }
    tuningCycles++;

    // serialPrintf("tuningCycles %u, isTuning %d\n", tuningCycles);
}

float Instrument::measureFrequency(int voiceIndex, float semis, bool isFilter) {
    if (isFilter) {
        voices[voiceIndex].out_cutoff = semis;
    } else {
        voices[voiceIndex].out_pitch = semis;
    }

    totalTuningCycles = (int)(0.302 * semis + 18.99);

    write();

    delay(2);  // waiting for voice card to settle on pitch

    tuningCycles = 0;
    isTuning = true;
    // serialPrintf("attached interupt %u\n", digitalPinToInterrupt(PIN_AUDIO_LOOPBACK));
    attachInterrupt(digitalPinToInterrupt(PIN_AUDIO_LOOPBACK), measurePitchSubroutine, RISING);

    unsigned long maxWaitTime = millis() + 2000;
    // unsigned long maxWaitTime = millis() + 1000;

    while (isTuning) {
        delayMicroseconds(500);  // waiting for tuning to finish
        if (millis() > maxWaitTime) {
            detachInterrupt(digitalPinToInterrupt(PIN_AUDIO_LOOPBACK));
#ifdef VERBOSE_TUNING
            serialPrintf("[%d] (%s) Tuning timeout, semis=%.2f\n",
                         voiceIndex, isFilter ? "cutoff" : "pitch", semis);
#endif
            return -1;
        }
    }

    unsigned long elapsedMicros = microsTuningEnd - microsTuningStart;
    float freq = totalTuningCycles / (elapsedMicros / 1000000.0);
    return freq;
}

#define TUNING_SAMPLES 20

int Instrument::findTuningProfile(int voiceIndex, float semis_min, float semis_max, bool isFilter) {
    TuningCorrection* corr;
    if (isFilter) {
        corr = &voices[voiceIndex].cutoff_correction;
    } else {
        corr = &voices[voiceIndex].pitch_correction;
    }
    // reset
    corr->parabolic[0] = 0;
    corr->parabolic[1] = 1;
    corr->parabolic[2] = 0;

    float x[TUNING_SAMPLES] = {0};
    float y[TUNING_SAMPLES] = {0};

    float step = (semis_max - semis_min) / (TUNING_SAMPLES - 1);

    for (int i = 0; i < TUNING_SAMPLES; i++) {
        float test_semis = semis_min + i * step;
        float test_freq = measureFrequency(voiceIndex, test_semis, isFilter);
        if (test_freq < 0.0) {
            return 1;  // tuning timeout (maybe make it such that some can fail TODO)
        }
        float ideal_semis = idealFrequencyToSemis(test_freq);

        x[i] = ideal_semis;
        y[i] = test_semis;

        printf("[%d] (%s) %-10.3f%-10.3f%-10.3f\n",
               voiceIndex, isFilter ? "cutoff" : "pitch", test_semis, test_freq, ideal_semis);
    }

    // solve LSQ
    fit_parabola(x, y, TUNING_SAMPLES, corr->parabolic);

#ifdef VERBOSE_TUNING
    printf("%-6d%-7s a+bx+cx^2: %-10.3f%-10.3f%-10.3f\n",
           voiceIndex, isFilter ? "cutoff" : "pitch", corr->parabolic[0], corr->parabolic[1], corr->parabolic[2]);
#endif

    return 0;
}

Patch& Instrument::getPatch() {
    return patch;
}

Voice& Instrument::getVoice(int i) {
    return voices[i];
}

int16_t* Instrument::getSettings() {
    return settings;
}

void Instrument::load_tuning() {
    TuningCorrection corrections[2 * ACTIVE_VOICES];

    memory_load_buffer((uint8_t*)&corrections, MEMORY_TUNING_START_ADDRESS, sizeof(corrections));

    for (int i = 0; i < ACTIVE_VOICES; i++) {
        Voice& voice = voices[i];
        voice.pitch_correction = corrections[2 * i + 0];
        voice.cutoff_correction = corrections[2 * i + 1];
    }
}

void Instrument::tune() {
    TuningCorrection newCorrections[2 * ACTIVE_VOICES];

    mainVolume = 0;

#ifdef VERBOSE_TUNING
    printf("Tuning:\n");
#endif

    leds.setAllNumbers(LedModes::LED_MODE_OFF);
    leds.write();

    for (int i = 0; i < ACTIVE_VOICES; i++) {
        // mute all voices

        for (int j = 0; j < VOICE_COUNT; j++) {
            voices[j].out_amp = 0.0;
        }
        Voice& voice = voices[i];

        reset_correction(voice.pitch_correction);
        reset_correction(voice.cutoff_correction);

        // OSCILLATOR PITCH
        mixer = MIXER_SQR;
        voice.out_cutoff = 120;
        voice.out_pulse = 0.5;
        voice.out_resonance = 0;
        voice.out_sub = 0;
        voice.out_amp = 1;
        int errored = findTuningProfile(i, 30, 110, false);  // dac write is called later on

        leds.setSingle(
            (PanelLeds)(PanelLeds::LED_PATCH_01 + 2 * i),
            errored ? LedModes::LED_MODE_OFF : LedModes::LED_MODE_ON);
        leds.write();

        // FILTER RESONANCE TRACKING
        mixer = 0;
        voice.out_resonance = 0.6;  // TODO test this
        voice.out_amp = 1;
        errored = findTuningProfile(i, 30, 60, true);  // dac write is called later on

        leds.setSingle(
            (PanelLeds)(PanelLeds::LED_PATCH_01 + 2 * i + 1),
            errored ? LedModes::LED_MODE_OFF : LedModes::LED_MODE_ON);
        leds.write();

        newCorrections[2 * i + 0] = voice.pitch_correction;
        newCorrections[2 * i + 1] = voice.cutoff_correction;
    }

    // save tuning
    memory_save_buffer((uint8_t*)&newCorrections, MEMORY_TUNING_START_ADDRESS, sizeof(newCorrections));
}

void Instrument::testTuning() {
    mainVolume = 0;
    totalTuningCycles = 100;

    for (int j = 5; j < 150; j += 5) {
        printf("%-10d", j);
    }
    printf("\n");

    for (int i = 0; i < ACTIVE_VOICES; i++) {
        // mute all voices

        for (int j = 0; j < VOICE_COUNT; j++) {
            voices[j].out_amp = 0.0;
        }
        Voice& voice = voices[i];

        // OSCILLATOR PITCH
        mixer = MIXER_SQR;
        voice.out_cutoff = 120;
        voice.out_pulse = 0.5;
        voice.out_resonance = 0;
        voice.out_sub = 0;
        voice.out_amp = 1;

        mixer = 0;
        voice.out_resonance = 0.6;
        voice.out_amp = 1;

        for (int j = 5; j < 150; j += 5) {
            // reset
            reset_correction(voice.pitch_correction);
            reset_correction(voice.cutoff_correction);

            float freq = measureFrequency(i, (float)j, true);
            printf("%-10.1f", freq);
        }
        printf("\n");
    }

    while (1);
}

// void Instrument::testTuning() {
//     mainVolume = 0;

//     int testVoice = 0;

//     printf("%-10s", "cycles");
//     for (int j = 5; j < 150; j += 5) {
//         printf("%-10d", j);
//     }
//     printf("\n");

//     for (totalTuningCycles = 10 ; totalTuningCycles <= 1000; totalTuningCycles *= 2) {

//         printf("%-10d", totalTuningCycles);

//         // mute all voices
//         for (int j = 0; j < VOICE_COUNT; j++) {
//             voices[j].out_amp = 0.0;
//         }
//         Voice& voice = voices[testVoice];

//         // OSCILLATOR PITCH
//         mixer = MIXER_SQR;
//         voice.out_cutoff = 120;
//         voice.out_pulse = 0.5;
//         voice.out_resonance = 0;
//         voice.out_sub = 0;
//         voice.out_amp = 1;

//         // mixer = 0;
//         // voice.out_resonance = 0.6;
//         // voice.out_amp = 1;

//         for (int j = 5; j < 150; j += 5) {
//             // reset
//             voice.pitch_correction.slope = 1;
//             voice.pitch_correction.intersept = 0;
//             voice.cutoff_correction.slope = 1;
//             voice.cutoff_correction.intersept = 0;

//             float freq = measureFrequency(testVoice, (float)j, false);
//             printf("%-10.2f", freq);
//         }
//         printf("\n");
//     }

//     while (1);
// }