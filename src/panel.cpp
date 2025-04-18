#include "panel.h"

#include <Arduino.h>
#include <string.h>

#include "config.h"
#include "memory.h"
#include "utils.h"

#define USE_TOGGLE(S, T) \
    if (isClicked(S)) {  \
        (T) = !(T);      \
    }

Panel::Panel(Instrument& instr, Player& player, PanelLedController& leds)
    : instr(instr), player(player), leds(leds) {
    // initialize panel
    setPanelInputsActivity(true);
}

// returns a number from 0 to positions-1 indicating
// in which voltage division the analog pin voltage falls
static int read_switch(int pin, int positions) {
    int step = 1023 / (positions - 1);
    int val = analogRead(pin);
    return (val + step / 2) / step;
}

// adjust delay to as low as possible
#define PANEL_MUX_IDLE_MICROS 100

void Panel::read() {
    digitalWrite(PIN_P_MUX_A, LOW);
    digitalWrite(PIN_P_MUX_B, LOW);
    digitalWrite(PIN_P_MUX_C, LOW);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_ARP_EN].current = !digitalRead(PIN_P_MUX_1);
    switches[SW_LFO_SYNC].current = !digitalRead(PIN_P_MUX_2);
    faders[FD_RESONANCE].current = analogRead(PIN_P_MUX_4);
    faders[FD_SUSTAIN].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_01].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_PROG_LOAD_PANEL].current = !digitalRead(PIN_P_MUX_4);
    switches[SW_RANGE_DOWN].current = !digitalRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_11].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, HIGH);
    digitalWrite(PIN_P_MUX_B, LOW);
    digitalWrite(PIN_P_MUX_C, LOW);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_SEQ_BLANK].current = !digitalRead(PIN_P_MUX_2);
    switches[SW_VCO_SAW].current = !digitalRead(PIN_P_MUX_4);
    faders[FD_OUTPUT_VOLUME].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_08].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_CHORUS_II].current = !digitalRead(PIN_P_MUX_4);
    faders[FD_PB_MOD_VCO].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_16].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, LOW);
    digitalWrite(PIN_P_MUX_B, HIGH);
    digitalWrite(PIN_P_MUX_C, LOW);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_MIDI_SYNC].current = !digitalRead(PIN_P_MUX_1);
    faders[FD_LFO_RATE].current = analogRead(PIN_P_MUX_2);
    faders[FD_SUB_OSCILLATOR].current = analogRead(PIN_P_MUX_4);
    faders[FD_ATTACK].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_03].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_PROG_STORE].current = !digitalRead(PIN_P_MUX_4);
    switches[SW_RANGE_UP].current = !digitalRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_09].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, HIGH);
    digitalWrite(PIN_P_MUX_B, HIGH);
    digitalWrite(PIN_P_MUX_C, LOW);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_SEQ_RECORD].current = !digitalRead(PIN_P_MUX_2);
    switches[SW_VCO_SQUARE].current = !digitalRead(PIN_P_MUX_4);
    switches[SW_AMP_SHAPE].current = read_switch(PIN_P_MUX_5, 2);
    switches[SW_PROG_NUM_07].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_CHORUS_I].current = !digitalRead(PIN_P_MUX_4);
    faders[FD_PB_MOD_VCF].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_15].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, LOW);
    digitalWrite(PIN_P_MUX_B, LOW);
    digitalWrite(PIN_P_MUX_C, HIGH);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    faders[FD_CTRL_RATE].current = analogRead(PIN_P_MUX_1);
    faders[FD_LFO_DELAY].current = analogRead(PIN_P_MUX_2);
    faders[FD_CUTOFF].current = analogRead(PIN_P_MUX_4);
    faders[FD_DECAY].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_02].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_PROG_MIDI_CH].current = !digitalRead(PIN_P_MUX_4);
    switches[SW_BEND_OCTAVE].current = !digitalRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_10].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, HIGH);
    digitalWrite(PIN_P_MUX_B, LOW);
    digitalWrite(PIN_P_MUX_C, HIGH);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_HOLD].current = !digitalRead(PIN_P_MUX_1);
    faders[FD_PULSE_WIDTH].current = analogRead(PIN_P_MUX_4);
    faders[FD_FILTER_LFO].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_06].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    faders[FD_PB_BEND].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_14].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, LOW);
    digitalWrite(PIN_P_MUX_B, HIGH);
    digitalWrite(PIN_P_MUX_C, HIGH);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_ARP_MODE].current = read_switch(PIN_P_MUX_1, 3);
    faders[FD_VIBRATO].current = analogRead(PIN_P_MUX_2);
    faders[FD_FILTER_ENVELOPE].current = analogRead(PIN_P_MUX_4);
    faders[FD_RELEASE].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_04].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_PROG_RETUNE].current = !digitalRead(PIN_P_MUX_4);
    switches[SW_PROG_NUM_12].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_A, HIGH);
    digitalWrite(PIN_P_MUX_B, HIGH);
    digitalWrite(PIN_P_MUX_C, HIGH);

    digitalWrite(PIN_P_MUX_3, LOW);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_KEY_TRANSPOSE].current = !digitalRead(PIN_P_MUX_1);
    switches[SW_ARP_RANGE].current = read_switch(PIN_P_MUX_2, 3);
    switches[SW_VCO_PWM_SOURCE].current = read_switch(PIN_P_MUX_4, 3);
    faders[FD_FILTER_KEYTRACK].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_05].current = !digitalRead(PIN_P_MUX_6);

    digitalWrite(PIN_P_MUX_3, HIGH);
    delayMicroseconds(PANEL_MUX_IDLE_MICROS);

    switches[SW_PROG_LOAD].current = !digitalRead(PIN_P_MUX_4);
    faders[FD_PB_MOD].current = analogRead(PIN_P_MUX_5);
    switches[SW_PROG_NUM_13].current = !digitalRead(PIN_P_MUX_6);
}

void Panel::test_print_raw_matrix() {
    serialPrintf("\n\n%-12s%-10s%-10s%-10s%-10s%-10s\n", "mode", "1", "2", "4", "5", "6");

    for (int i = 0; i < 16; i++) {
        int chA = (i >> 0) & 1;
        int chB = (i >> 1) & 1;
        int chC = (i >> 2) & 1;
        int ch3 = (i >> 3) & 1;
        digitalWrite(PIN_P_MUX_A, chA);
        digitalWrite(PIN_P_MUX_B, chB);
        digitalWrite(PIN_P_MUX_C, chC);
        digitalWrite(PIN_P_MUX_3, ch3);

        char mode[5];
        memcpy(mode, "____\0", 5);
        if (chA) mode[0] = 'A';
        if (chB) mode[1] = 'B';
        if (chC) mode[2] = 'C';
        if (ch3) mode[3] = '3';

        // delayMicroseconds(100);
        delayMicroseconds(500);

        int r1 = analogRead(PIN_P_MUX_1);
        int r2 = analogRead(PIN_P_MUX_2);
        int r4 = analogRead(PIN_P_MUX_4);
        int r5 = analogRead(PIN_P_MUX_5);
        int r6 = analogRead(PIN_P_MUX_6);

        serialPrintf("%-12s%-10d%-10d%-10d%-10d%-10d\n", mode, r1, r2, r4, r5, r6);
    }
}

const char* fader_names[] = {
    "LFO_RATE",
    "LFO_DELAY",
    "VIBRATO",
    "PULSE_WIDTH",
    "SUB_OSCILLATOR",
    "CUTOFF",
    "RESONANCE",
    "FILTER_ENVELOPE",
    "FILTER_LFO",
    "FILTER_KEYTRACK",
    "ATTACK",
    "DECAY",
    "SUSTAIN",
    "RELEASE",
    "BP_BEND",
    "BP_MOD",
    "PB_MOD_VCO",
    "PB_MOD_VCF",
    "CTRL_RATE",
    "OUTPUT_VOLUME"};

const char* switch_names[] = {
    "AMP_SHAPE",
    "VCO_PWM_SOURCE",
    "LFO_SYNC",
    "VCO_SQUARE",
    "VCO_SAW",
    "CHORUS_I",
    "CHORUS_II",
    "BEND_OCTAVE",
    "RANGE_UP",
    "RANGE_DOWN",
    "HOLD",
    "KEY_TRANSPOSE",
    "MIDI_SYNC",
    "ARP_EN",
    "ARP_MODE",
    "ARP_RANGE",
    "SEQ_RECORD",
    "SEQ_BLANK",
    "PROG_LOAD",
    "PROG_STORE",
    "PROG_MIDI_CH",
    "PROG_RETUNE",
    "PROG_LOAD_PANEL",
    "PROG_NUM_01",
    "PROG_NUM_02",
    "PROG_NUM_03",
    "PROG_NUM_04",
    "PROG_NUM_05",
    "PROG_NUM_06",
    "PROG_NUM_07",
    "PROG_NUM_08",
    "PROG_NUM_09",
    "PROG_NUM_10",
    "PROG_NUM_11",
    "PROG_NUM_12",
    "PROG_NUM_13",
    "PROG_NUM_14",
    "PROG_NUM_15",
    "PROG_NUM_16",
};

const char* fader_value_lines[] = {
    "                ",
    "_               ",
    "__              ",
    "___             ",
    "____            ",
    "_____           ",
    "______          ",
    "_______         ",
    "________        ",
    "_________       ",
    "__________      ",
    "___________     ",
    "____________    ",
    "_____________   ",
    "______________  ",
    "_______________ ",
    "________________",
};

const char* fader_line(int value) {
    return fader_value_lines[value / 61];
}

void Panel::test_print_panel_elements() {
    serialPrintf("\nPanel faders:\n");
    for (int i = FD_LFO_RATE; i <= FD_OUTPUT_VOLUME; i++) {
        serialPrintf("%-20s | %s %s\n",
                     fader_names[i], fader_line(faders[i].current), faders[i].active ? "ACTIVE" : "INACT.");
    }
    delay(500);

    serialPrintf("\nPanel switches:\n");
    for (int i = SW_AMP_SHAPE; i <= SW_PROG_NUM_16; i++) {
        serialPrintf("%-20s | %-4d [was %-4d]\n",
                     switch_names[i], switches[i].current, switches[i].last);
    }
}

void Panel::updateStatefulFader(int faderIndex, int16_t* target) {
    PanelElement& pe = faders[faderIndex];
    // activate fader by moving
    if (!pe.active) {
        int16_t diff = abs(pe.current - pe.lastActive);
        if (diff > FADER_NUDGE_THRESHOLD) {
            pe.active = true;
        }
    }
    // update target of active fader
    if (pe.active) {
        *target = pe.lastActive = pe.current;
    }
}

void Panel::updateStatefulSwitch(int switchIndex, int8_t* target) {
    PanelElement& pe = switches[switchIndex];
    if (!pe.active && (pe.lastActive != pe.current)) {
        pe.active = true;
    }
    if (pe.active) {
        *target = pe.lastActive = pe.current;
    }
}

bool Panel::isClicked(int switchIndex) {
    return !switches[switchIndex].current && switches[switchIndex].last;
}

bool Panel::isClickedEarly(int switchIndex) {
    return switches[switchIndex].current && !switches[switchIndex].last;
}

bool Panel::isHeld(int switchIndex) {
    return switches[switchIndex].current;
}

int Panel::getClickedNumber() {
    for (int i = 0; i < 16; i++) {
        if (isClicked(SW_PROG_NUM_01 + i)) {
            return i;
        }
    }
    return -1;
}

void Panel::setPanelInputsActivity(bool active) {
    for (int i = 0; i < PATCH_FD__COUNT__; i++) {
        faders[i].active = active;
    }
    for (int i = 0; i < PATCH_SW__COUNT__; i++) {
        switches[i].active = active;
    }
}

int max(int a, int b) {
    return (a < b) ? b : a;
}
int min(int a, int b) {
    return (a > b) ? b : a;
}

void Panel::update() {
    // patch stuff:
    // ____Active array must get all set to false if patch gets loaded from somewhere

    for (int i = 0; i < PATCH_FD__COUNT__; i++) {
        updateStatefulFader(i, &instr.getPatch().faders[i]);
    }
    for (int i = 0; i < PATCH_SW__COUNT__; i++) {
        switch (i) {
            case SW_VCO_SQUARE:
            case SW_VCO_SAW:
            case SW_CHORUS_I:
            case SW_CHORUS_II: {
                USE_TOGGLE(i, instr.getPatch().switches[i]);
                break;
            }
            default:
                // others, e.g. pwm source
                updateStatefulSwitch(i, &instr.getPatch().switches[i]);
                break;
        }
    }

    // instrument settings:
    // ____Active element must get set to false if externally value is overwritten (e.g. MIDI CC)

    int16_t* instrSettings = instr.getSettings();

    updateStatefulFader(FD_PB_BEND, &instrSettings[INS_PITCHBEND]);
    updateStatefulFader(FD_PB_MOD, &instrSettings[INS_MODWHEEL]);
    updateStatefulFader(FD_OUTPUT_VOLUME, &instrSettings[INS_VOLUME]);
    // the following can be set directly
    instrSettings[INS_MOD_VCO] = faders[FD_PB_MOD_VCO].current;
    instrSettings[INS_MOD_VCF] = faders[FD_PB_MOD_VCF].current;

    USE_TOGGLE(SW_BEND_OCTAVE, instrSettings[INS_BEND_OCTAVE]);
    leds.setSingle(LED_BEND_OCT, instrSettings[INS_BEND_OCTAVE] ? LED_MODE_ON : LED_MODE_OFF);

    // PLAYER

    PlayerState playerState = player.getState();
    int16_t* playerSettings = player.getSettingsList();

    USE_TOGGLE(SW_HOLD, playerSettings[PLS_HOLDING]);
    leds.setSingle(LED_HOLD, playerSettings[PLS_HOLDING] ? LED_MODE_ON : LED_MODE_OFF);

    USE_TOGGLE(SW_KEY_TRANSPOSE, playerSettings[PLS_TRANSPOSING]);
    leds.setSingle(LED_TRANSPOSE, playerSettings[PLS_TRANSPOSING] ? LED_MODE_ON : LED_MODE_OFF);

    USE_TOGGLE(SW_MIDI_SYNC, playerSettings[PLS_MIDICLOCK]);
    leds.setSingle(LED_MIDI_CLOCK, playerSettings[PLS_MIDICLOCK] ? LED_MODE_ON : LED_MODE_OFF);

    playerSettings[PLS_RATE] = faders[FD_CTRL_RATE].current;
    playerSettings[PLS_ARP_MODE] = switches[SW_ARP_MODE].current;
    playerSettings[PLS_ARP_RANGE] = switches[SW_ARP_RANGE].current;

    int16_t newRange = playerSettings[PLS_OCTAVE_OFFSET];
    if (isClicked(SW_RANGE_UP)) {
        newRange = min(2, newRange + 1);
    }
    if (isClicked(SW_RANGE_DOWN)) {
        newRange = max(-2, newRange - 1);
    }
    if (newRange != playerSettings[PLS_OCTAVE_OFFSET]) {
        instr.allNotesOff();
        playerSettings[PLS_OCTAVE_OFFSET] = newRange;
        switch (newRange) {
            case -2:
                leds.setSingle(LED_RANGE_DOWN, LED_MODE_BLINK);
                leds.setSingle(LED_RANGE_UP, LED_MODE_OFF);
                break;
            case -1:
                leds.setSingle(LED_RANGE_DOWN, LED_MODE_ON);
                leds.setSingle(LED_RANGE_UP, LED_MODE_OFF);
                break;
            case 0:
                leds.setSingle(LED_RANGE_DOWN, LED_MODE_OFF);
                leds.setSingle(LED_RANGE_UP, LED_MODE_OFF);
                break;
            case 1:
                leds.setSingle(LED_RANGE_DOWN, LED_MODE_OFF);
                leds.setSingle(LED_RANGE_UP, LED_MODE_ON);
                break;
            case 2:
                leds.setSingle(LED_RANGE_DOWN, LED_MODE_OFF);
                leds.setSingle(LED_RANGE_UP, LED_MODE_BLINK);
                break;
        }
    }

    if (isClicked(SW_ARP_EN)) {
        switch (playerState) {
            case PLSTATE_ARP:
                player.setStateNormal();
                break;
            default:
                player.setStateArp();
                break;
        }
    }

    if (isHeld(SW_SEQ_RECORD)) {
        int number = getClickedNumber();
        if (number >= 0) {
            player.setStateSeqRecording(1 + number);
        }
    }
    if (isClickedEarly(SW_SEQ_RECORD) &&
        ((player.getState() == PlayerState::PLSTATE_SEQ_PLAYING) ||
         (player.getState() == PlayerState::PLSTATE_SEQ_RECORDING))) {
        player.setStateNormal();
    }
    if (isClicked(SW_SEQ_BLANK)) {
        player.pushBlankNote();
    }

    // PROGRAM SECTION

    if (isClicked(SW_PROG_RETUNE)) {
        instr.tune();
    }

    Patch loadedPatch;
    bool hasLoadedPatch = false;

    if (isHeld(SW_PROG_LOAD)) {
        int patchNumber = getClickedNumber();
        if (patchNumber >= 0) {
            // load patch
            leds.setAllNumbers(LED_MODE_OFF);
            leds.write();

            // aesthetic delay
            delay(500);

            // load into seperate buffer which is only
            // applied at end of function. this makes the user
            // able to swap active patch with patchNumber if both
            // load and store is pressed simultaneously
            hasLoadedPatch = true;
            int patchAddr = MEMORY_PRESETS_START_ADDRESS + sizeof(Patch) * patchNumber;
            memory_load_buffer((uint8_t*)&loadedPatch, patchAddr, sizeof(Patch));

            // set panel to inactive
            setPanelInputsActivity(false);

            leds.setSingle((PanelLeds)(LED_PATCH_01 + patchNumber), LED_MODE_ON);
            leds.write();
        }
    }
    if (isHeld(SW_PROG_STORE)) {
        int patchNumber = getClickedNumber();
        if (patchNumber >= 0) {
            // leds off
            leds.setAllNumbers(LED_MODE_OFF);
            leds.write();
            // aesthetic delay
            delay(500);
            // store patch
            int patchAddr = MEMORY_PRESETS_START_ADDRESS + sizeof(Patch) * patchNumber;
            memory_save_buffer((uint8_t*)&instr.getPatch(), patchAddr, sizeof(Patch));
            // patch led on
            leds.setSingle((PanelLeds)(LED_PATCH_01 + patchNumber), LED_MODE_ON);
            leds.write();
        }
    }
    if (isHeld(SW_PROG_MIDI_CH)) {
        leds.setAllNumbers(LedModes::LED_MODE_OFF);
        int currentChannel = player.getMidiChannel();
        if (currentChannel == 0) {
            leds.setAllNumbers(LedModes::LED_MODE_ON);
        } else {
            int led = PanelLeds::LED_PATCH_01 + currentChannel - 1;
            leds.setSingle((PanelLeds)led, LedModes::LED_MODE_ON);
        }
        leds.write();

        int channel = getClickedNumber();
        if (channel >= 0) {
            player.toggleMidiChannel(1 + channel);
        }
    }
    if (isClicked(SW_PROG_LOAD_PANEL)) {
        setPanelInputsActivity(true);
        leds.setAllNumbers(LED_MODE_OFF);
    }

    if (hasLoadedPatch) {
        instr.getPatch() = loadedPatch;
    }

    for (int i = 0; i < PANEL_FD__COUNT__; i++) {
        faders[i].last = faders[i].current;
    }
    for (int i = 0; i < PANEL_SW__COUNT__; i++) {
        switches[i].last = switches[i].current;
    }
}
