#include "dacs.h"

#include <Arduino.h>
#include <SPI.h>
#include "utils.h"
#include "instrument.h"

#define DAC_CHANNEL_COUNT 8
#define DAC_COUNT 8
#define DAC_CH_A 0
#define DAC_CH_B 1
#define DAC_CH_C 2
#define DAC_CH_D 3
#define DAC_CH_E 4
#define DAC_CH_F 5
#define DAC_CH_G 6
#define DAC_CH_H 7

uint8_t dac_buffer[DAC_CHANNEL_COUNT * DAC_COUNT];

static uint8_t float_to_char(float t) {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    return (uint8_t)(255 * t);
}

static uint16_t semis_to_short(float semis) {
    float steps_per_semi = 546.125;
    float steps = steps_per_semi * semis;
    if (steps < 0) steps = 0;
    if (steps > 65535) steps = 65535;
    return (uint16_t)steps;
}

static float apply_correction(float x, TuningCorrection* corr) {
    float a = corr->parabolic[0];
    float b = corr->parabolic[1];
    float c = corr->parabolic[2];
    return a + x * (b + x * c);
}

// check
#define DAC_CS_DELAY_MICROS 20

void dacs_write(Instrument* inst) {
    for (int i = 0; i < ACTIVE_VOICES; i += 2) {
        Voice* voice_a = &inst->voices[i];
        Voice* voice_b = &inst->voices[i + 1];

        uint8_t* lower_dac = &dac_buffer[DAC_CHANNEL_COUNT * i];
        uint8_t* upper_dac = &dac_buffer[DAC_CHANNEL_COUNT * (i + 1)];

        lower_dac[DAC_CH_A] = float_to_char(voice_a->out_pulse);
        lower_dac[DAC_CH_B] = float_to_char(voice_b->out_pulse);
        lower_dac[DAC_CH_C] = float_to_char(voice_b->out_resonance);
        lower_dac[DAC_CH_D] = float_to_char(voice_b->out_amp * voice_b->volume_correction);
        lower_dac[DAC_CH_E] = float_to_char(voice_a->out_resonance);
        lower_dac[DAC_CH_F] = float_to_char(voice_a->out_amp * voice_a->volume_correction);
        lower_dac[DAC_CH_G] = float_to_char(voice_b->out_sub);
        lower_dac[DAC_CH_H] = float_to_char(voice_a->out_sub);

        uint16_t pitch_a = semis_to_short(apply_correction(voice_a->out_pitch, &voice_a->pitch_correction));
        uint16_t pitch_b = semis_to_short(apply_correction(voice_b->out_pitch, &voice_b->pitch_correction));
        uint16_t cutoff_a = semis_to_short(apply_correction(voice_a->out_cutoff, &voice_a->cutoff_correction));
        uint16_t cutoff_b = semis_to_short(apply_correction(voice_b->out_cutoff, &voice_b->cutoff_correction));

        // serialPrintf("pulsea %u, pulseb %u, resb %u, vcab %u, resa %u, vcaa %u, subb %u, suba %u\n",
        //     lower_dac[DAC_CH_A], lower_dac[DAC_CH_B], lower_dac[DAC_CH_C], lower_dac[DAC_CH_D], 
        //     lower_dac[DAC_CH_E], lower_dac[DAC_CH_F], lower_dac[DAC_CH_G], lower_dac[DAC_CH_H]);

        // serialPrintf("pitch_a %u, pitch_b %u, cutoff_a %u, cutoff_b %u\n", 
        //     pitch_a, pitch_b, cutoff_a, cutoff_b);

        upper_dac[DAC_CH_A] = (cutoff_b >> 8) & 0xff;
        upper_dac[DAC_CH_B] = cutoff_b & 0xff;
        upper_dac[DAC_CH_C] = (pitch_b >> 8) & 0xff;
        upper_dac[DAC_CH_D] = pitch_b & 0xff;
        upper_dac[DAC_CH_E] = (cutoff_a >> 8) & 0xff;
        upper_dac[DAC_CH_F] = cutoff_a & 0xff;
        upper_dac[DAC_CH_G] = (pitch_a >> 8) & 0xff;
        upper_dac[DAC_CH_H] = pitch_a & 0xff;
    }

    SPI.beginTransaction(dacSPISettings);

    for (int channel = 0; channel < DAC_CHANNEL_COUNT; channel++) {
        digitalWrite(PIN_DAC_CS, LOW);
        delayMicroseconds(DAC_CS_DELAY_MICROS);

        for (int dac = DAC_COUNT - 1; dac >= 0; dac--) {
            uint8_t dac_level = dac_buffer[DAC_CHANNEL_COUNT * dac + channel];
            uint8_t ctrl_msg = channel + 1;
            // send msg
            uint16_t serial_msg = (ctrl_msg << 12) | (dac_level << 4);
            SPI.transfer16(serial_msg);

            delayMicroseconds(DAC_CS_DELAY_MICROS);
        }

        digitalWrite(PIN_DAC_CS, HIGH);
        delayMicroseconds(DAC_CS_DELAY_MICROS);
    }

    SPI.endTransaction();
}
