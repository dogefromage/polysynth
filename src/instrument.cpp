#include "instrument.h"

#include <Arduino.h>
#include <SPI.h>

#include "config.h"
#include "core_pins.h"
#include "dacs.h"
#include "panel.h"
#include "sine.h"
#include "utils.h"

static float clamp01(float x) {
    if (x < 0.0) x = 0.0;
    if (x > 1.0) x = 1.0;
    return x;
}

#define ADSR_EPS 0.05
#define ADSR_MINUS_LN_EPS 2.9957

void Envelope::update(float dt, bool gate) {
    if (this->lastGate != gate) {
        this->lastGate = gate;
        this->state = gate ? ENVELOPE_ATTACK : ENVELOPE_RELEASE;
    }

    float target = 0, period = 1;

    switch (this->state) {
        case ENVELOPE_ATTACK:
            target = 1.0;
            period = this->attack;
            break;
        case ENVELOPE_DECAY:
            target = this->sustain;
            period = this->decay;
            break;
        case ENVELOPE_RELEASE:
            target = 0;
            period = this->release;
            break;
    }

    float lambda = ADSR_MINUS_LN_EPS / period;
    float delta = target - this->level;

    this->level += delta * clamp01(lambda * dt);

    float err = fabsf(delta);

    if (this->state == ENVELOPE_ATTACK && err < ADSR_EPS) {
        this->state = ENVELOPE_DECAY;
    }
}

void Lfo::update(float dt, bool gate) {
    if (lastGate != gate) {
        lastGate = gate;
        // time = 0;
    }

    x += 2 * M_PI * frequency * dt;

    level = sineApprox(x);
    if (time < delay) {
        level = 0;
    }
    time += dt;
}

void Voice::update(float dt, Patch* patch) {
    env.attack = 10 * faderLog(patch->faders[FD_ATTACK]);
    env.decay = 10 * faderLog(patch->faders[FD_DECAY]);
    env.sustain = faderLin(patch->faders[FD_SUSTAIN]);
    env.release = 10 * faderLog(patch->faders[FD_RELEASE]);

    lfo.frequency = 10 * faderLog(patch->faders[FD_LFO_RATE]);
    lfo.delay = 10 * faderLog(patch->faders[FD_LFO_DELAY]);

    env.update(dt, gate);
    lfo.update(dt, gate);

    float vcoLfo = 30 * faderLog(patch->faders[FD_VIBRATO]);
    float vcfFreq = lerp(faderLin(patch->faders[FD_CUTOFF]), -20, 60);
    float vcfKybd = faderLinSnap(patch->faders[FD_FILTER_KEYTRACK], 0.05);
    float vcfLfo = 30 * faderLog(patch->faders[FD_FILTER_LFO]);
    float vcfEnv = 50 * faderLin(patch->faders[FD_FILTER_ENVELOPE]);

    float pwm = faderLin(patch->faders[FD_PULSE_WIDTH]);
    out_pitch = note + vcoLfo * lfo.level;
    out_cutoff = vcfFreq + vcfKybd * note + vcfLfo * lfo.level + vcfEnv * env.level;
    out_pulse = chooseValue(
        0.5 + 0.5 * env.level * pwm,
        0.5 + 0.5 * lfo.level * pwm,
        pwm,
        patch->switches[SW_VCO_PWM_SOURCE]);
    out_sub = faderLin(patch->faders[FD_SUB_OSCILLATOR]);
    out_resonance = 0.6 * faderLin(patch->faders[FD_RESONANCE]);
    out_amp = 0.4 * chooseValue(
                        env.level,
                        gate ? 1.0 : 0,
                        0.0,
                        patch->switches[SW_AMP_SHAPE]);
}

Instrument::Instrument(PanelLedController& leds) : leds(leds) {
    memset(settings, 0, sizeof(settings));

    patch.switches[PatchSwitches::SW_VCO_SAW] = 1;

    settings[INS_VOLUME] = 900;
}

void Instrument::update(float dt) {
    for (int i = 0; i < ACTIVE_VOICES; i++) {
        // serialPrintf("%u, %u, %u\n", i, voices[i].note, voices[i].gate);
        voices[i].update(dt, &patch);
    }

    int square = patch.switches[SW_VCO_SQUARE] & 1;
    int saw = patch.switches[SW_VCO_SAW] & 1;
    mixer = (saw << 1) | square;

    int chorus1 = patch.switches[SW_CHORUS_I] & 1;
    int chorus2 = patch.switches[SW_CHORUS_II] & 1;
    chorusType = (chorus2 << 1) | chorus1;

    leds.setSingle(LED_SQUARE, square ? LED_MODE_ON : LED_MODE_OFF);
    leds.setSingle(LED_SAW, saw ? LED_MODE_ON : LED_MODE_OFF);
    leds.setSingle(LED_CHORUS_I, chorus1 ? LED_MODE_ON : LED_MODE_OFF);
    leds.setSingle(LED_CHORUS_II, chorus2 ? LED_MODE_ON : LED_MODE_OFF);

    switch (chorusType) {
        case 0:
        case 1:  // TODO customize chorus per mode
        case 2:
        case 3:
            chorusLfoLeft.frequency = 0.3;
            chorusLfoRight.frequency = 0.3;
            // 90° offset
            chorusLfoRight.x = chorusLfoLeft.x + 0.5 * M_PI;
            break;
    }
    chorusLfoLeft.update(dt, HIGH);
    chorusLfoRight.update(dt, HIGH);

    mainVolume = settings[INS_VOLUME] / 1024.0f;
    // serialPrintf("%.2f\n", mainVolume);
    // delay(100);
}

void Instrument::scheduleNoteOn(int note, int velocity) {
    if (velocity == 0) {
        scheduleNoteOff(note);
        return;
    }

    for (int i = 0; i < ACTIVE_VOICES; i++) {
        if (voices[i].gate && voices[i].note == note) {
            return;  // some voice is already playing that note :(
        }
    }

    int oldest = 0;
    int oldestTag = voices[0].schedulingTag;
    bool oldestGate = voices[0].gate;

    // if (monoVoice >= 0) {
    //     // schedule mono voice
    //     oldest = monoVoice;
    // } else

    // find oldest voice or steal active one
    for (int i = 0; i < ACTIVE_VOICES; i++) {
        int currentTag = voices[i].schedulingTag;
        int currentGate = voices[i].gate;
        // higher priority for scheduling voice if gate off but respect tag on both ends
        // => lexicographic ordering of (gate, tag)
        if ((!currentGate && oldestGate) || (currentGate == oldestGate && currentTag < oldestTag)) {
            oldest = i;
            oldestTag = currentTag;
            oldestGate = currentGate;
        }
    }

    // // test only 1 voice:
    oldest = 0;

    // schedule oldest voice and steal if necessary
    Voice& next = voices[oldest];

    if (next.gate) {
        // steal voice, shut down envelope such that it starts correctly
        next.env.level = 0;
        next.env.state = EnvelopeState::ENVELOPE_ATTACK;
    }

    next.note = note;
    next.velocity = velocity;
    next.gate = true;
    next.schedulingTag = schedulingTagCounter++;

    serialPrintf("scheduled %d -> [%d], vel=%d\n", note, oldest, velocity);
}

void Instrument::scheduleNoteOff(int note) {
    for (int i = 0; i < ACTIVE_VOICES; i++) {
        if (voices[i].gate && voices[i].note == note) {
            voices[i].gate = false;
            voices[i].schedulingTag = schedulingTagCounter++;
            serialPrintf("scheduled note %d off [%d]\n", note, i);
            return;
        }
    }
}

void Instrument::allNotesOff() {
    for (int i = 0; i < ACTIVE_VOICES; i++) {
        // only change tag if actual state changes
        if (voices[i].gate) {
            voices[i].gate = false;
            voices[i].schedulingTag = schedulingTagCounter++;
            return;
        }
    }
}

void reset_correction(TuningCorrection& corr) {
    corr.parabolic[0] = 0;
    corr.parabolic[1] = 1;
    corr.parabolic[2] = 0;
}

void Instrument::test() {
    int i = 1;

    for (Voice& v : voices) {
        v.out_amp = 0;
    }

    mixer = MIXER_SAW /*  | MIXER_SQR */;
    chorusType = 0;
    mainVolume = 0.8;

    Voice& v = getVoice(i);
    v.out_pitch = 50;
    v.out_cutoff = 50;
    v.out_pulse = 0.5;
    v.out_resonance = 0;
    v.out_sub = 0.6;
    v.out_amp = 0.8;

    reset_correction(v.pitch_correction);
    reset_correction(v.cutoff_correction);

    while (true) {
        // write();
        // delay(500);

        // printf("test outer loop\n");

        // for (int i = 0; i <= 100; i++) {
        //     v.out_amp = 0.01 * i;
        //     write();
        //     delay(100);
        // }

        // v.out_pitch = 100;
        // v.out_amp = 1.0;
        // v.out_pitch = 20;
        // mainVolume = 0.5;

        // chorusType = 3;
        // chorusLfoLeft.frequency = chorusLfoRight.frequency = 2.0;
        // chorusLfoLeft.update(0.01, HIGH);
        // chorusLfoRight.update(0.01, HIGH);

        // write();
        // delay(100);

        // v.out_pitch = 30.0;
        // write();
        // delay(1000);

        for (int i = 10; i < 100; i += 10) {
            v.out_pitch = (float)i;
            printf("%.2f\n", v.out_pitch);
            write();
            delay(500);
        }

        // delayMicroseconds(500);
        // serialPrintf("Test\n");
        // delay(500);
    };
}

void Instrument::testChorus() {
    Lfo lfo;
    lfo.frequency = 0.1;

    while (1) {
        printf("%f\n", lfo.level);

        lfo.update(0.1f, true);
        delay(100);

        float normalized = 0.25 + 0.75 * (0.5 + 0.5 * lfo.level);
        int level = (int)(255 * normalized);

        SPI.beginTransaction(mcp4802Settings);
        digitalWrite(PIN_CHORUS_DAC_CS, LOW);
        delayMicroseconds(1);
        SPI.transfer16((1 << 12) | (level << 4));  // 12bit means dac active
        digitalWrite(PIN_CHORUS_DAC_CS, HIGH);
        delayMicroseconds(1);
        digitalWrite(PIN_CHORUS_DAC_CS, LOW);
        delayMicroseconds(1);
        SPI.transfer16((1 << 15) | (1 << 12) | (level << 4));  // 15 bit means channel B
        digitalWrite(PIN_CHORUS_DAC_CS, HIGH);
        SPI.endTransaction();
    }
}

int toClampedChar(float x) {
    int intX = (int)x;
    if (intX > 255) {
        intX = 255;
    }
    if (intX < 0) {
        intX = 0;
    }
    return (int)x;
}

char chorus_level(float lfo) {
    float normalized = 0.25 + 0.75 * (0.5 + 0.5 * lfo);
    return toClampedChar(255 * normalized);
}

void Instrument::write() {
    dacs_write(this);

    // printf("Mixer: %d\n", mixer);

    digitalWrite(PIN_EN_SAW, !(mixer & MIXER_SAW));
    digitalWrite(PIN_EN_SQR, !(mixer & MIXER_SQR));
    digitalWrite(PIN_CHORUS_1, !(chorusType & 1));
    digitalWrite(PIN_CHORUS_2, !(chorusType & 2));

    // MCP4802 for chorus
    // int levelA = toClampedChar(255 * chorusLfoLeft.level);
    // int levelB = toClampedChar(255 * chorusLfoRight.level);
    int levelA = chorus_level(chorusLfoLeft.level);
    int levelB = chorus_level(chorusLfoRight.level);

    spiBeginTransaction(mcp4802Settings);
    SPI.beginTransaction(mcp4802Settings);
    digitalWrite(PIN_CHORUS_DAC_CS, LOW);
    delayMicroseconds(1);

    SPI.transfer16((1 << 12) | (levelA << 4));  // 12bit means dac active

    digitalWrite(PIN_CHORUS_DAC_CS, HIGH);
    delayMicroseconds(1);

    digitalWrite(PIN_CHORUS_DAC_CS, LOW);
    delayMicroseconds(1);

    SPI.transfer16((1 << 15) | (1 << 12) | (levelB << 4));  // 15 bit means channel B

    digitalWrite(PIN_CHORUS_DAC_CS, HIGH);
    SPI.endTransaction();

    // PGA2311
    SPI.beginTransaction(pga2311Settings);
    digitalWrite(PIN_AMP_CS, LOW);
    delayMicroseconds(3);

    char mainGain = toClampedChar(255 * mainVolume);
    SPI.transfer16((mainGain << 8) | mainGain);

    digitalWrite(PIN_AMP_CS, HIGH);
    delayMicroseconds(3);
    SPI.endTransaction();
}