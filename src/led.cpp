#include "led.h"

#include "SPIWrapper.h"
#include "config.h"

void PanelLedController::update(float dt) {
    timeSinceSwitch += dt;
    if (timeSinceSwitch > BLINK_HALF_PERIOD) {
        timeSinceSwitch = 0;
        blinkState = !blinkState;
    }
}

void PanelLedController::write() {
    int ledMapping[] = {
        0,  // 0
        LED_BEND_OCT,
        LED_RANGE_DOWN,
        LED_RANGE_UP,
        0,
        LED_HOLD,
        LED_MIDI_CLOCK,
        LED_TRANSPOSE,
        0,  // 8
        0,
        LED_ARP_EN,
        LED_RECORD,
        LED_SAW,
        LED_SQUARE,
        LED_CHORUS_I,
        LED_CHORUS_II,
        LED_PATCH_05,  // 16
        LED_PATCH_04,
        LED_PATCH_03,
        LED_PATCH_02,
        LED_PATCH_01,
        LED_PATCH_06,
        LED_PATCH_07,
        LED_PATCH_08,
        LED_PATCH_16,  // 24
        LED_PATCH_15,
        LED_PATCH_14,
        LED_PATCH_13,
        LED_PATCH_12,
        LED_PATCH_11,
        LED_PATCH_10,
        LED_PATCH_09,
    };

    uint32_t buf = 0;
    for (uint32_t i = 0; i < 32; i++) {
        uint32_t ledPos = ledMapping[i];
        if (ledPos >= 0) {
            LedModes mode = ledState[ledPos];
            if (mode == LED_MODE_ON ||
                (mode == LED_MODE_BLINK && blinkState)) {
                buf |= (1u << i);
            }
        }
    }

    enterCritical();
    spiWrapper.beginTransaction(ledSPISettings);
    digitalWrite(PIN_P_SR_RCLK, LOW);
    delayMicroseconds(5);  // idk
    spiWrapper.transfer16(buf >> 16);
    spiWrapper.transfer16(buf);
    delayMicroseconds(5);
    digitalWrite(PIN_P_SR_RCLK, HIGH);
    spiWrapper.endTransaction();
    exitCritical();
}

void PanelLedController::setAll(LedModes mode) {
    for (int i = 0; i < LED__COUNT__; i++) {
        setSingle((PanelLeds)i, mode);
    }
}

void PanelLedController::setAllNumbers(LedModes mode) {
    for (int i = LED_PATCH_01; i <= LED_PATCH_16; i++) {
        setSingle((PanelLeds)i, mode);
    }
}

void PanelLedController::setSingle(PanelLeds led, LedModes mode) {
    ledState[led] = mode;
}
