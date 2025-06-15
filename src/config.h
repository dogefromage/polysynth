#pragma once

// teensy pins
#include "SPIWrapper.h"
#include "StableTimer.h"

// #define PIN_SPI_MOSI 11 // already in teensy headers
// #define PIN_SPI_SCK 13
#define PIN_EN_SQR 41
#define PIN_EN_SAW 26

#define PIN_AUDIO_LOOPBACK 38

#define PIN_KYBD_MUX_A 1
#define PIN_KYBD_MUX_B 2
#define PIN_KYBD_MUX_C 3

#define PIN_KYBD_MUX_1 4
#define PIN_KYBD_MUX_2 24

#define PIN_P_MOSI 7
#define PIN_P_SCK 8

#define PIN_P_MUX_A 21
#define PIN_P_MUX_B 22
#define PIN_P_MUX_C 23

#define PIN_P_MUX_1 14
#define PIN_P_MUX_2 15
#define PIN_P_MUX_3 16
#define PIN_P_MUX_4 17
#define PIN_P_MUX_5 18
#define PIN_P_MUX_6 19

#define PIN_CHORUS_1 36
#define PIN_CHORUS_2 37

#define PIN_P_SR_RCLK 20
#define PIN_CHORUS_DAC_CS 31
#define PIN_DAC_CS 32
#define PIN_AMP_CS 33
#define PIN_KYBD_CS 5

// #define PIN_MIDI_TX 35
// #define PIN_MIDI_RX 34

#define VERBOSE_TUNING

#define ACTIVE_VOICES 8

#define NUM_KEYS 61

extern SPIWrapperSettings ledSPISettings;
extern SPIWrapperSettings dacSPISettings;
extern SPIWrapperSettings mcp4802Settings;
extern SPIWrapperSettings pga2311Settings;
extern SPIWrapperSettings keyboardSPISettings;

extern StableTimer clockTimer;

inline void enterCritical() {
    clockTimer.enterCriticalSection();
}

inline void exitCritical() {
    clockTimer.exitCriticalSection();
}

#if 0
#define debugprintf printf
#else
#define debugprintf(...) void
#endif
