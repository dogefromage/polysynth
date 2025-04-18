#include "led.h"

#include "config.h"
#include "utils.h"

void PanelLedController::update(float dt) {
    timeSinceSwitch += dt;
    if (timeSinceSwitch > BLINK_HALF_PERIOD) {
        timeSinceSwitch = 0;
        blinkState = !blinkState;
    }
}

void PanelLedController::write() {
    // SPI.beginTransaction(ledSPISettings);
    // SPI.end();

    // manual setup
    digitalWrite(PIN_P_MOSI, HIGH);
    digitalWrite(PIN_P_SCK, LOW);

    digitalWrite(PIN_P_SR_RCLK, LOW);
    delayMicroseconds(5);  // idk
    
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

    uint64_t led_clock = 100000;
    // uint64_t led_clock = 50000;
    sendSPIBitBangedMode0MSBFirst(buf >> 24, PIN_P_MOSI, PIN_P_SCK, led_clock);
    delayMicroseconds(5);
    sendSPIBitBangedMode0MSBFirst(buf >> 16, PIN_P_MOSI, PIN_P_SCK, led_clock);
    delayMicroseconds(5);
    sendSPIBitBangedMode0MSBFirst(buf >>  8, PIN_P_MOSI, PIN_P_SCK, led_clock);
    delayMicroseconds(5);
    sendSPIBitBangedMode0MSBFirst(buf      , PIN_P_MOSI, PIN_P_SCK, led_clock);
    delayMicroseconds(5);

    // SPI.transfer32(buf);
    
    digitalWrite(PIN_P_MOSI, HIGH);
    digitalWrite(PIN_P_SCK, LOW);

    delayMicroseconds(5);

    digitalWrite(PIN_P_SR_RCLK, HIGH);

    // SPI.begin();

    // SPI.endTransaction();
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
