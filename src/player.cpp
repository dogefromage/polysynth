#include "player.h"

#include <usb_midi.h>

#include "utils.h"
#include <Arduino.h>
#include <SPI.h>

/**
 * [Bank]: first or second keyboard matrix
 * [Row]: Shift register output address from 0 to 7 in specified bank
 * [Column]: Mux input ABC address from 0 to 7
 * Value: index of key from 0 to NUM_KEYS-1. if above 100 then 
 *          second pushbutton at index (key-100) is meant
 */
int16_t keyMatrices[2][8][8] = {
    {
        {  32,  36,  34,  38,  33,  37,  35,  39 },
        { 132, 136, 134, 138, 133, 137, 135, 139 },
        { 140, 144, 142, 146, 141, 145, 143, 147 },
        { 148, 152, 150, 154, 149, 153, 151, 155 },
        { 156, 160, 158,  -1, 157,  -1, 159,  -1 },
        {  40,  44,  42,  46,  41,  45,  43,  47 },
        {  48,  52,  50,  54,  49,  53,  51,  55 },
        {  56,  60,  58,  -1,  57,  -1,  59,  -1 }
    },
    {
        {   8,  12,  10,  14,   9,  13,  11,  15 },
        {  16,  20,  18,  22,  17,  21,  19,  23 },
        {   0,   4,   2,   6,   1,   5,   3,   7 },
        { 100, 104, 102, 106, 101, 105, 103, 107 },
        { 108, 112, 110, 114, 109, 113, 111, 115 },
        { 116, 120, 118, 122, 117, 121, 119, 123 },
        { 124, 128, 126, 130, 125, 129, 127, 131 },
        {  24,  28,  26,  30,  25,  29,  27,  31 }
    }
};

// per bank input from mux
int keyMatrixInputs[2] = { PIN_KYBD_MUX_1, PIN_KYBD_MUX_2 };

void usbMidiSendClock() {
    usbMIDI.sendClock();
}

void usbMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity) {
    Player::getInstance().noteOn(note, velocity, true);
}

void usbMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity) {
    Player::getInstance().noteOff(note, velocity, true);
}

void usbMidiHandleControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    switch (control) {
        case 123:
            Player::getInstance().instr.allNotesOff();
            break;
    }
}

void usbMidiClock() {
    Player::getInstance().clockTick(true);
}

Player& Player::getInstance() {
    return *Player::instance;
}

void Player::setState(PlayerState nextState) {
    state = nextState;
    instr.allNotesOff();
    settings[PLS_TRANSPOSING] = false;
    
    switch (state) {
        case PLSTATE_NORMAL:
            leds.setSingle(LED_ARP_EN, LED_MODE_OFF);
            leds.setSingle(LED_RECORD, LED_MODE_OFF);
            break;
        case PLSTATE_ARP:
            leds.setSingle(LED_ARP_EN, LED_MODE_ON);
            leds.setSingle(LED_RECORD, LED_MODE_OFF);
            break;
        case PLSTATE_SEQ_RECORDING:
            leds.setSingle(LED_ARP_EN, LED_MODE_OFF);
            leds.setSingle(LED_RECORD, LED_MODE_BLINK);
            break;
        case PLSTATE_SEQ_PLAYING:
            leds.setSingle(LED_ARP_EN, LED_MODE_OFF);
            leds.setSingle(LED_RECORD, LED_MODE_ON);
            break;
    }
}

void Player::pushSequencerNote(int note) {
    if (state != PLSTATE_SEQ_RECORDING) {
        serialPrintf("sequencer not recording!\n");
        return;
    }
    if (noteBufferSize >= sequenceLength) {
        serialPrintf("sequence already full!\n");
        return;
    }

    leds.setSingle((PanelLeds)(PanelLeds::LED_PATCH_01 + noteBufferSize), 
        LedModes::LED_MODE_ON);
    noteBuffer[noteBufferSize] = note;
    noteBufferSize++;

    if (noteBufferSize >= sequenceLength) {
        setState(PLSTATE_SEQ_PLAYING);
    }
}

void Player::bubbleSortBuf() {
    for (int n = 0; n < noteBufferSize - 1; n++) {
        for (int i = 0; i < noteBufferSize - 1; i++) {
            if (noteBuffer[i] > noteBuffer[i + 1]) {
                // temp
                int temp = noteBuffer[i];
                noteBuffer[i] = noteBuffer[i + 1];
                noteBuffer[i + 1] = temp;
            }
        }
    }
}

void Player::addArpNote(int note) {
    for (int i = 0; i < noteBufferSize; i++) {
        if (noteBuffer[i] == note) {
            return;  // note already present
        }
    }

    if (noteBufferSize >= NOTE_BUFFER_MAX_SIZE) {
        serialPrintf("exceeded arp note count\n");
        return;
    }

    noteBuffer[noteBufferSize++] = note;
    bubbleSortBuf();
}

void Player::removeArpNote(int note) {
    for (int i = 0; i < noteBufferSize; i++) {
        if (note == noteBuffer[i]) {
            noteBuffer[i] = 9000;  // larger than any note, gets sorted to end
            bubbleSortBuf();
            noteBufferSize--;  // cut off this last note
            return;
        }
    }
    // here, note not found
}

void Player::setTransposition(int note) {
    // set transposition such that lowest note in note buffer is heard as given note
    if (noteBufferSize <= 0) {
        printf("note buffer empty, returning\n");
        return;
    }

    int lowest = noteBuffer[0];
    for (int i = 1; i < noteBufferSize; i++) {
        int curr = noteBuffer[i];
        if (curr < lowest) {
            lowest = curr;
        }
    }

    keyboardTransposition = note - lowest;

    printf("transposing to %d\n", keyboardTransposition);
}

void Player::noteOn(int note, int velocity, bool isMidi) {

    // instr.scheduleNoteOn(note, velocity);
    // return;

    if (settings[PLS_TRANSPOSING]) {
        setTransposition(note);
        return;
    } else {
        // reset transposition
        keyboardTransposition = 0;
    }

    switch (state) {
        case PLSTATE_ARP:
            addArpNote(note);
            break;
        case PLSTATE_SEQ_RECORDING:
            pushSequencerNote(note);
            // do not break here
        case PLSTATE_NORMAL:
        case PLSTATE_SEQ_PLAYING:
            instr.scheduleNoteOn(note, velocity);
            break;
    }
}

void Player::noteOff(int note, int velocity, bool isMidi) {

    // instr.scheduleNoteOff(note);
    // return;

    if (settings[PLS_TRANSPOSING]) {
        return;
    }

    switch (state) {
        case PLSTATE_ARP:
            removeArpNote(note);
            break;
        case PLSTATE_NORMAL:
        case PLSTATE_SEQ_RECORDING:
        case PLSTATE_SEQ_PLAYING:
            instr.scheduleNoteOff(note);
            break;
    }
}

void Player::step() {

    if (noteUpStep) {
        instr.allNotesOff();

    } else {
        if (state == PLSTATE_ARP && noteBufferSize > 0) {
            // int arpMode = settings[PLS_ARP_MODE];
            // int arpRange = settings[PLS_ARP_RANGE];
            // ignore range and mode for now TODO

            if (noteBufPosition < 0 || noteBufPosition >= noteBufferSize) {
                noteBufPosition = 0;
            } else {
                noteBufPosition = (noteBufPosition + 1) % noteBufferSize;
            }

            int note = noteBuffer[noteBufPosition] + keyboardTransposition;
            instr.scheduleNoteOn(note, 127);
        }

        if (state == PLSTATE_SEQ_PLAYING) {
            if (noteBufPosition < 0 || noteBufPosition >= noteBufferSize) {
                noteBufPosition = 0;
            } else {
                noteBufPosition = (noteBufPosition + 1) % noteBufferSize;
            }

            if (noteBuffer[noteBufPosition] >= 0) {
                int note = noteBuffer[noteBufPosition] + keyboardTransposition;
                instr.scheduleNoteOn(note, 127);
            }
        }
    }

    noteUpStep = !noteUpStep;
}

static int midiClockDivFromRate(int rate) {
    return 6; // TODO
}

static float getTickDuration(int rate) {
    return 0.1 * faderLog(1023 - rate);
}

void Player::clockTick(bool isMidi) {
    bool useMidiClock = settings[PLS_MIDICLOCK];
    if (isMidi != useMidiClock) {
        return;
    }

    int divider = 6;
    if (useMidiClock) {
        divider = midiClockDivFromRate(settings[PLS_RATE]);
    }

    ticksSinceStep++;
    if (ticksSinceStep >= divider) {
        ticksSinceStep = 0;
        step();
    }
}

static int calculateVelocity(uint32_t millis) {
    // TODO: fix this shitty curve
    return std::min(127, 1000 / (int)millis);
}

int Player::keyToNote(int key) {
    // input key: 0 - number of keys - 1
    // final pitch = C0 + note
    return key + 12 * (2 + settings[PLS_OCTAVE_OFFSET]);
}

void Player::updateKey(int key, KeyState currentState) {

    auto activeKey = activeKeys.find(key);

    if (currentState == KEY_STATE_OPEN && activeKey != activeKeys.end()) {
        if (!activeKey->second.travelling) {
            int note = keyToNote(key);
            noteOff(note, 0, false);
        }
        activeKeys.erase(activeKey);
    }

    if (currentState == KEY_STATE_TRAVELLING && activeKey == activeKeys.end()) {
        // not already present
        activeKeys[key] = {
            travellingMillis: millis(),
            travelling: true
        };
    }

    if (currentState == KEY_STATE_PRESSED) {
        if (activeKey != activeKeys.end() && activeKey->second.travelling) {
            // transition from travelling to fully pressed
            activeKey->second.travelling = false;

            uint32_t deltaMillis = millis() - activeKey->second.travellingMillis;
            int velocity = calculateVelocity(deltaMillis);
            int note = keyToNote(key);
            noteOn(note, velocity, false);
        }
        else if (activeKey == activeKeys.end()) {
            // key did not enter travelling phase - directly start with full velocity
            int note = keyToNote(key);
            noteOn(note, 127, false);

            activeKeys[key] = {
                travellingMillis: 0,
                travelling: false
            };
        }
    }
}

void Player::testKeyBed() {
    bool somethingPressed = false;

    for (int row = 0; row < 8; row++) {

        SPI.beginTransaction(keyboardSPISettings);
        digitalWrite(PIN_KYBD_CS, LOW);
        delayMicroseconds(1);
        // send two hot bits for both matrices of the keyboard through the SRs
        uint16_t twoHotRow = (uint16_t)(((1 << 8) | 1) << row);
        SPI.transfer16(twoHotRow);
        digitalWrite(PIN_KYBD_CS, HIGH);
        SPI.endTransaction();

        // read inputs
        for (int column = 0; column < 8; column++) {

            digitalWrite(PIN_KYBD_MUX_A, column & 4);
            digitalWrite(PIN_KYBD_MUX_B, column & 2);
            digitalWrite(PIN_KYBD_MUX_C, column & 1);

            delayMicroseconds(200);

            if (digitalRead(PIN_KYBD_MUX_1)) {
                printf("(1,%d,%d) ", row, column);
                somethingPressed = true;
            }

            if (digitalRead(PIN_KYBD_MUX_2)) {
                printf("(2,%d,%d) ", row, column);
                somethingPressed = true;
            }
        }
    }

    if (somethingPressed) {
        printf("\n");
    }
}

int Player::getMidiChannel() {
    return midiChannel;
}

void Player::readKeyBoard() {

    KeyState states[NUM_KEYS];

    for (int i = 0; i < NUM_KEYS; i++) {
        states[i] = KEY_STATE_OPEN;
    }

    for (int row = 0; row < 8; row++) {

        SPI.beginTransaction(keyboardSPISettings);
        digitalWrite(PIN_KYBD_CS, LOW);
        delayMicroseconds(1);
        // send two hot bits for both matrices of the keyboard through the SRs
        uint16_t twoHotRow = (uint16_t)(((1 << 8) | 1) << row);
        SPI.transfer16(twoHotRow);
        digitalWrite(PIN_KYBD_CS, HIGH);
        SPI.endTransaction();

        // read inputs
        for (int column = 0; column < 8; column++) {

            digitalWrite(PIN_KYBD_MUX_A, column & 4);
            digitalWrite(PIN_KYBD_MUX_B, column & 2);
            digitalWrite(PIN_KYBD_MUX_C, column & 1);

            delayMicroseconds(20);
            // delayMicroseconds(5);

            for (int bank = 0; bank < 2; bank++) {
                int button = keyMatrices[bank][row][column];
                if (button < 0) {
                    // unused address in button matrix
                    continue;
                }

                if (digitalRead(keyMatrixInputs[bank])) {

                    if (button >= 100) {
                        // is second trigger
                        states[button - 100] = KEY_STATE_PRESSED;
                    } else {
                        // states[button] = KEY_STATE_PRESSED;

                        // is first trigger
                        if (states[button] == KEY_STATE_OPEN) {
                            states[button] = KEY_STATE_TRAVELLING;
                        }
                    }
                }
            }
        }
    }

    for (int i = 0; i < NUM_KEYS; i++) {
        updateKey(i, states[i]);
    }
}

Player* Player::instance = NULL;

Player::Player(Instrument& instr, PanelLedController& leds) : instr(instr), leds(leds) {
    Player::instance = this;
}

void Player::init() {
    usbMIDI.setHandleNoteOn(usbMidiNoteOn);
    usbMIDI.setHandleNoteOff(usbMidiNoteOff);
    usbMIDI.setHandleControlChange(usbMidiHandleControlChange);
    usbMIDI.setHandleClock(usbMidiClock);
}

void Player::update(float dt) {

    if (midiChannel < 0) {
        usbMIDI.read();
    } else {
        usbMIDI.read(midiChannel + 1);
    }

    // read keys
    readKeyBoard();

    // play arp or seq
    if (state == PLSTATE_ARP || state == PLSTATE_SEQ_PLAYING) {
        
        timeSinceTick += dt;
        float tickDuration = getTickDuration(settings[PLS_RATE]);
        if (timeSinceTick >= tickDuration) {
            timeSinceTick = 0;

            if (!settings[PLS_MIDICLOCK]) {
                clockTick(false);
                usbMidiSendClock();
            }
        }
    }
}

PlayerState Player::getState() {
    return state;
}

int16_t* Player::getSettingsList() {
    return settings;
}

void Player::setStateNormal() {
    setState(PLSTATE_NORMAL);
}

void Player::setStateArp() {
    setState(PLSTATE_ARP);
    noteBufferSize = 0;
}

void Player::setStateSeqRecording(int size) {
    if (size <= 0) {
        serialPrintf("invalid seq size %d\n", size);
        return;
    }
    if (size >= NOTE_BUFFER_MAX_SIZE) {
        serialPrintf("seq size too large for note buffer\n");
        return;
    }

    leds.setAllNumbers(LedModes::LED_MODE_OFF);

    setState(PLSTATE_SEQ_RECORDING);
    sequenceLength = size;
    noteBufferSize = 0;
}

void Player::pushBlankNote() {
    if (state == PLSTATE_SEQ_RECORDING) {
        pushSequencerNote(-1);
    }
}

void Player::toggleMidiChannel(int channel) {
    if (midiChannel == channel) {
        midiChannel = 0;
    } else {
        midiChannel = channel;
    }
}