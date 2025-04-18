#pragma once
#include <cstdint>

enum PanelLeds {
    LED_BEND_OCT,
    LED_RANGE_UP,
    LED_RANGE_DOWN,
    LED_HOLD,
    LED_TRANSPOSE,
    LED_MIDI_CLOCK,
    LED_ARP_EN,
    LED_RECORD,
    LED_SQUARE,
    LED_SAW,
    LED_CHORUS_I,
    LED_CHORUS_II,
    LED_PATCH_01,
    LED_PATCH_02,
    LED_PATCH_03,
    LED_PATCH_04,
    LED_PATCH_05,
    LED_PATCH_06,
    LED_PATCH_07,
    LED_PATCH_08,
    LED_PATCH_09,
    LED_PATCH_10,
    LED_PATCH_11,
    LED_PATCH_12,
    LED_PATCH_13,
    LED_PATCH_14,
    LED_PATCH_15,
    LED_PATCH_16,
    LED__COUNT__,
};

enum LedModes {
    LED_MODE_OFF,
    LED_MODE_ON,
    LED_MODE_BLINK,
};

#define BLINK_HALF_PERIOD 0.5

struct PanelLedController {
    LedModes ledState[LED__COUNT__] = {};
    bool blinkState = false;
    float timeSinceSwitch = 0;

   public:
    void update(float dt);
    void write();
    void setAll(LedModes mode);
    void setAllNumbers(LedModes mode);
    void setSingle(PanelLeds led, LedModes mode);
};