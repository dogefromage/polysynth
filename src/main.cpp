#include <Arduino.h>
#include <SPI.h>

#include <climits>

#include "config.h"
#include "instrument.h"
#include "led.h"
#include "memory.h"
#include "midi.h"
#include "panel.h"
#include "player.h"
#include "utils.h"

PanelLedController leds;
Instrument instr(leds);
Player player(instr, leds);
Panel panel(instr, player, leds);

// uint32_t clockIn, uint8_t bitOrderIn, uint8_t dataModeIn
SPISettings dacSPISettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
SPISettings mcp4802Settings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
SPISettings pga2311Settings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
SPISettings keyboardSPISettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);

// SPISettings dacSPISettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
// SPISettings mcp4802Settings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
// SPISettings pga2311Settings = SPISettings(1000000, MSBFIRST, SPI_MODE0);
// SPISettings keyboardSPISettings = SPISettings(1000000, MSBFIRST, SPI_MODE0);

void pin_setup() {
    // seperate bitbanged pseudo-SPI line for whacky panel
    pinMode(PIN_P_MOSI, OUTPUT);
    pinMode(PIN_P_SCK, OUTPUT);
    digitalWrite(PIN_P_MOSI, HIGH);
    digitalWrite(PIN_P_SCK, LOW);

    pinMode(PIN_SPI_MOSI, OUTPUT);
    pinMode(PIN_SPI_SCK, OUTPUT);
    SPI.setMOSI(PIN_SPI_MOSI);
    SPI.setSCK(PIN_SPI_SCK);
    SPI.begin();

    pinMode(PIN_EN_SQR, OUTPUT);
    pinMode(PIN_EN_SAW, OUTPUT);

    pinMode(PIN_AUDIO_LOOPBACK, INPUT_PULLDOWN);

    pinMode(PIN_KYBD_MUX_A, OUTPUT);
    pinMode(PIN_KYBD_MUX_B, OUTPUT);
    pinMode(PIN_KYBD_MUX_C, OUTPUT);

    pinMode(PIN_KYBD_MUX_1, INPUT_PULLDOWN);
    pinMode(PIN_KYBD_MUX_2, INPUT_PULLDOWN);

    pinMode(PIN_P_MUX_A, OUTPUT);
    pinMode(PIN_P_MUX_B, OUTPUT);
    pinMode(PIN_P_MUX_C, OUTPUT);

    pinMode(PIN_P_MUX_1, INPUT);
    pinMode(PIN_P_MUX_2, INPUT);
    pinMode(PIN_P_MUX_3, OUTPUT);  // controls 4053 ABC, design change
    pinMode(PIN_P_MUX_4, INPUT);
    pinMode(PIN_P_MUX_5, INPUT);
    pinMode(PIN_P_MUX_6, INPUT);

    pinMode(PIN_CHORUS_1, OUTPUT);
    pinMode(PIN_CHORUS_2, OUTPUT);
    digitalWrite(PIN_CHORUS_1, HIGH);
    digitalWrite(PIN_CHORUS_2, HIGH);

    pinMode(PIN_P_SR_RCLK, OUTPUT);
    pinMode(PIN_CHORUS_DAC_CS, OUTPUT);
    pinMode(PIN_DAC_CS, OUTPUT);
    pinMode(PIN_AMP_CS, OUTPUT);
    pinMode(PIN_KYBD_CS, OUTPUT);
    digitalWrite(PIN_P_SR_RCLK, HIGH);
    digitalWrite(PIN_CHORUS_DAC_CS, HIGH);
    digitalWrite(PIN_DAC_CS, HIGH);
    digitalWrite(PIN_AMP_CS, HIGH);
    digitalWrite(PIN_KYBD_CS, HIGH);
}

void init_test_all() {
    printf("\nTesting:\n");
    delay(1000);

    // // TEST PANEL READ AND LABELED
    printf("\nPanel elements:\n");
    panel.read();
    panel.test_print_panel_elements();
    delay(1000);

    // TEST LEDS
    printf("\nLEDs:\n");
    for (int i = LED__COUNT__ - 1; i >= 0; i--) {
        serialPrintf("%d\n", i);
        leds.setAll(LED_MODE_OFF);
        leds.setSingle((PanelLeds)i, LED_MODE_ON);
        leds.write();
        delay(300);
    }
    delay(1000);

    printf("\nTesting done!\n");
}

void setup() {
    Serial.begin(115200);

    // while (!Serial); // wait for serial to open

    pin_setup();
    analogReadAveraging(4);  // values from 1-4 https://forum.pjrc.com/index.php?threads/analog-read-on-teensy-4-0-slower-compared-to-teensy-3-6.57683/

    // instr.testTuning();
    // instr.test();

    // while (1) {
    //     digitalWrite(PIN_EN_SAW, HIGH);
    //     printf("HIGH\n");
    //     delay(2000);

    //     digitalWrite(PIN_EN_SAW, LOW);
    //     printf("LOW\n");
    //     delay(2000);

    //     // for (int i = 0; i < 4; i++) {
    //     //     digitalWrite(PIN_EN_SAW, !(i & MIXER_SAW));
    //     //     digitalWrite(PIN_EN_SQR, !(i & MIXER_SQR));
    //     //     printf("%d\n", i);

    //     //     delay(2000);
    //     // }
    // }
    // instr.testChorus();

    player.init();

    delay(2000);  // warm-up
    instr.load_tuning();
    // instr.tune();

    instr.getVoice(0).volume_correction = 0.6;
    instr.getVoice(1).volume_correction = 1.0;
    instr.getVoice(2).volume_correction = 1.0;
    instr.getVoice(3).volume_correction = 1.0;
    instr.getVoice(4).volume_correction = 0.7;
    instr.getVoice(5).volume_correction = 1.0;

    // set active patch equal to first
    Patch firstPatch;
    memory_load_buffer((uint8_t*)&firstPatch, MEMORY_PRESETS_START_ADDRESS, sizeof(Patch));
    instr.getPatch() = firstPatch;

    init_test_all();

    printf("\nStarting loop...\n");
}

unsigned long previousMicros = 0;

float pastTime = 0;
float secondCounter;
int lastSecondPrint = 0;
int loopCounter = 0;

int lastSecondsInt = 0;

void loop() {
    unsigned long currentMicros, elapsedTimeMicros;
    currentMicros = micros();
    if (currentMicros >= previousMicros) {
        elapsedTimeMicros = currentMicros - previousMicros;
    } else {
        elapsedTimeMicros = (ULONG_MAX - previousMicros) + currentMicros + 1;
    }
    previousMicros = currentMicros;
    float dt = elapsedTimeMicros / 1000000.0;
    pastTime += dt;
    secondCounter += dt;

    int secondsInt = (int)secondCounter;
    if (secondsInt - lastSecondPrint >= 10) {
        printf("%u seconds, %d loops\n", secondsInt, loopCounter);
        lastSecondPrint = secondsInt;
        loopCounter = 0;
    }
    loopCounter++;

    panel.read();
    panel.update();

    player.update(dt);

    instr.update(dt);
    instr.write();
    delayMicroseconds(5000);

    leds.update(dt);
    leds.write();
}