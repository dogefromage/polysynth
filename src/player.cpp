#include "player.h"

#include <Arduino.h>

#include <unordered_set>

#include "SPIWrapper.h"
#include "StableTimer.h"
#include "config.h"
#include "midis.h"
#include "utils.h"

/**
 * [Bank]: first or second keyboard matrix
 * [Row]: Shift register output address from 0 to 7 in specified bank
 * [Column]: Mux input ABC address from 0 to 7
 * Value: index of key from 0 to NUM_KEYS-1. if above 100 then
 *          second pushbutton at index (key-100) is meant
 */
int16_t keyMatrices[2][8][8] = {
    {{32, 36, 34, 38, 33, 37, 35, 39},
     {132, 136, 134, 138, 133, 137, 135, 139},
     {140, 144, 142, 146, 141, 145, 143, 147},
     {148, 152, 150, 154, 149, 153, 151, 155},
     {156, 160, 158, -1, 157, -1, 159, -1},
     {40, 44, 42, 46, 41, 45, 43, 47},
     {48, 52, 50, 54, 49, 53, 51, 55},
     {56, 60, 58, -1, 57, -1, 59, -1}},
    {{8, 12, 10, 14, 9, 13, 11, 15},
     {16, 20, 18, 22, 17, 21, 19, 23},
     {0, 4, 2, 6, 1, 5, 3, 7},
     {100, 104, 102, 106, 101, 105, 103, 107},
     {108, 112, 110, 114, 109, 113, 111, 115},
     {116, 120, 118, 122, 117, 121, 119, 123},
     {124, 128, 126, 130, 125, 129, 127, 131},
     {24, 28, 26, 30, 25, 29, 27, 31}}};

// per bank input from mux
int keyMatrixInputs[2] = {PIN_KYBD_MUX_1, PIN_KYBD_MUX_2};

enum class ArpModes {
    UP,
    UPDOWN,
    DOWN
};

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

    int led = noteBufferSize % 16;
    if (led == 0) {
        // clear leds at start or wrap
        leds.setAllNumbers(LedModes::LED_MODE_OFF);
    }
    leds.setSingle((PanelLeds)(PanelLeds::LED_PATCH_01 + led), LedModes::LED_MODE_ON);
    noteBuffer[noteBufferSize] = note;
    noteBufferSize++;

    if (noteBufferSize >= sequenceLength) {
        setState(PLSTATE_SEQ_PLAYING);
    }
}

void swap(int* a, int* b) {
    int c = *a;
    *a = *b;
    *b = c;
}

void Player::updateArpSequence() {
    // reconstruct arp sequence from currently pressed keys
    noteBufferSize = 0;
    for (const auto& activeKey : activeKeys) {
        if (activeKey.second.state == KeyStates::PRESSED) {
            int note = keyToNote(activeKey.first);
            noteBuffer[noteBufferSize++] = note;
            if (noteBufferSize >= NOTE_BUFFER_MAX_SIZE) {
                break;  // too many notes
            }
        }
    }

    // sort notes
    for (int n = 0; n < noteBufferSize - 1; n++) {
        for (int i = 0; i < noteBufferSize - 1; i++) {
            if (noteBuffer[i] > noteBuffer[i + 1]) {
                swap(&noteBuffer[i], &noteBuffer[i + 1]);
            }
        }
    }
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
        if (curr >= 0 && curr < lowest) {
            lowest = curr;
        }
    }

    keyboardTransposition = note - lowest;

    printf("transposing to %d\n", keyboardTransposition);
}

void Player::handleNoteOn(int note, int velocity, bool isMidi) {
    // instr.scheduleNoteOn(note, velocity);
    // return;

    if (settings[PLS_TRANSPOSING]) {
        setTransposition(note);
        return;
    }
    // reset transposition
    keyboardTransposition = 0;

    if (state == PLSTATE_ARP) {
        updateArpSequence();
    } else {
        instr.scheduleNoteOn(note, velocity);

        if (state == PLSTATE_SEQ_RECORDING) {
            pushSequencerNote(note);
        } else if (state == PLSTATE_NORMAL && !isMidi) {
            midiSendNoteOn(note, velocity, midiChannel);
        }
    }
}

void Player::handleNoteOff(int note, int velocity, bool isMidi) {
    // instr.scheduleNoteOff(note);
    // return;

    if (settings[PLS_TRANSPOSING]) {
        return;
    }

    if (state == PLSTATE_ARP) {
        if (!settings[PLS_HOLDING]) {
            updateArpSequence();
        }
    } else {
        instr.scheduleNoteOff(note);

        if (state == PLSTATE_NORMAL && !isMidi) {
            midiSendNoteOff(note, velocity, midiChannel);
        }
    }
}

void Player::step() {
    if (noteUpStep) {
        instr.scheduleNoteOff(lastStepNote);

    } else {
        if (state == PLSTATE_ARP && noteBufferSize > 0) {
            ArpModes arpMode = (ArpModes)settings[PLS_ARP_MODE];
            int arpRange = settings[PLS_ARP_RANGE];
            int multiplier = 1;
            switch (arpRange) {
                case 1:
                    multiplier = 2;
                    break;
                case 2:
                    multiplier = 3;
                    break;
            }

            bool isBelow = noteBufPosition < 0;
            int arpEnd = noteBufferSize * multiplier;
            bool isAbove = noteBufPosition >= arpEnd;

            if (arpMode == ArpModes::UP) {
                arpDownwards = false;  // for seamless transition if switched
                if (isBelow || isAbove) {
                    noteBufPosition = 0;
                }

            } else if (arpMode == ArpModes::UPDOWN) {
                if (isBelow) {
                    arpDownwards = false;
                    noteBufPosition = min(arpEnd, 1);
                }
                if (isAbove) {
                    arpDownwards = true;
                    noteBufPosition = max(0, arpEnd - 2);
                }
            } else if (arpMode == ArpModes::DOWN) {
                arpDownwards = true;  // for seamless transition if switched
                if (isBelow || isAbove) {
                    noteBufPosition = arpEnd - 1;
                }
            }

            int noteIndex = noteBufPosition % noteBufferSize;
            int currOctaveOffset = noteBufPosition / noteBufferSize;

            int note = noteBuffer[noteIndex] + keyboardTransposition + 12 * currOctaveOffset;
            instr.scheduleNoteOn(note, 127);
            lastStepNote = note;

            noteBufPosition += arpDownwards ? -1 : 1;
        }

        if (state == PLSTATE_SEQ_PLAYING) {
            if (noteBufPosition < 0 || noteBufPosition >= noteBufferSize) {
                // initial note or out of bounds state sets index to 0
                noteBufPosition = 0;
            } else {
                noteBufPosition = (noteBufPosition + 1) % noteBufferSize;
            }

            if (noteBuffer[noteBufPosition] >= 0) {
                int note = noteBuffer[noteBufPosition] + keyboardTransposition;
                instr.scheduleNoteOn(note, 127);
                lastStepNote = note;
            }

            int ledIndex = noteBufPosition % 16;
            leds.setAllNumbers(LedModes::LED_MODE_OFF);
            leds.setSingle((PanelLeds)(PanelLeds::LED_PATCH_01 + ledIndex), LedModes::LED_MODE_ON);
        }
    }

    noteUpStep = !noteUpStep;
}

int midiClockDivFromRate(int rate) {
    int numDividers = 14;
    int dividers[] = {96, 72, 48, 36, 32, 24, 18, 16, 12, 9, 8, 6, 4, 3};

    int selectedDivider = discretizeValue(rate, numDividers);
    return dividers[selectedDivider];
}

float getClockStepSeconds(int rate) {
    return 0.001 + 0.05 * faderLog(1023 - rate);
}

void Player::clockTick(bool isMidi) {
    if (state != PLSTATE_ARP && state != PLSTATE_SEQ_PLAYING) {
        return;
    }
    if (songMode == SongMode::Paused) {
        return;
    }
    bool useMidiClock = settings[PLS_MIDICLOCK];
    if (isMidi != useMidiClock) {
        // check if is selected clock source
        return;
    }
    if (!isMidi) {
        midiSendClock();
    }

    int divider = 12;  // always 8th notes with builtin clock
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

void Player::setSongMode(SongMode mode) {
    songMode = mode;
}

void Player::handleMidiStart() {
    resetClockProgress();
    setSongMode(SongMode::Playing);
}

void Player::handleMidiStop() {
    setSongMode(SongMode::Paused);
    instr.allNotesOff();
}

void Player::handleMidiContinue() {
    setSongMode(SongMode::Playing);
}

void Player::handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value) {
    switch (control) {
        case 123:
            instr.allNotesOff();
            break;
        case 250:
            resetClockProgress();
            break;
    }
}

void Player::resetClockProgress() {
    instr.allNotesOff();
    ticksSinceStep = 0;
    arpDownwards = false;
    noteUpStep = false;
    noteBufPosition = -1;
}

void Player::updateKey(int key, KeyStates currentState) {
    auto activeKey = activeKeys.find(key);

    if (currentState == KeyStates::OPEN && activeKey != activeKeys.end()) {
        bool wasPressed = activeKey->second.state == KeyStates::PRESSED;
        activeKeys.erase(activeKey);
        if (wasPressed) {
            handleNoteOff(keyToNote(key), 0, false);
        }
    }

    if (currentState == KeyStates::TRAVELLING && activeKey == activeKeys.end()) {
        // not already present
        activeKeys[key] = {
            state : KeyStates::TRAVELLING,
            travellingMillis : millis(),
        };
    }

    if (currentState == KeyStates::PRESSED) {
        if (activeKey != activeKeys.end() && activeKey->second.state == KeyStates::TRAVELLING) {
            // transition from travelling to fully pressed
            activeKey->second.state = KeyStates::PRESSED;

            uint32_t deltaMillis = millis() - activeKey->second.travellingMillis;
            int velocity = calculateVelocity(deltaMillis);
            int note = keyToNote(key);
            handleNoteOn(note, velocity, false);
        } else if (activeKey == activeKeys.end()) {
            // key did not enter travelling phase - directly start with full velocity
            activeKeys[key] = {
                state : KeyStates::PRESSED,
                travellingMillis : 0,
            };

            int note = keyToNote(key);
            handleNoteOn(note, 127, false);
        }
    }
}

void Player::testKeyBed() {
    bool somethingPressed = false;

    for (int row = 0; row < 8; row++) {
        enterCritical();
        spiWrapper.beginTransaction(keyboardSPISettings);
        digitalWrite(PIN_KYBD_CS, LOW);
        delayMicroseconds(1);
        // send two hot bits for both matrices of the keyboard through the SRs
        uint16_t twoHotRow = (uint16_t)(((1 << 8) | 1) << row);
        spiWrapper.transfer16(twoHotRow);
        digitalWrite(PIN_KYBD_CS, HIGH);
        spiWrapper.endTransaction();
        exitCritical();

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
    KeyStates states[NUM_KEYS];

    for (int i = 0; i < NUM_KEYS; i++) {
        states[i] = KeyStates::OPEN;
    }

    for (int row = 0; row < 8; row++) {
        enterCritical();
        spiWrapper.beginTransaction(keyboardSPISettings);
        digitalWrite(PIN_KYBD_CS, LOW);
        delayMicroseconds(1);
        // send two hot bits for both matrices of the keyboard through the SRs
        uint16_t twoHotRow = (uint16_t)(((1 << 8) | 1) << row);
        spiWrapper.transfer16(twoHotRow);
        digitalWrite(PIN_KYBD_CS, HIGH);
        spiWrapper.endTransaction();
        exitCritical();

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
                        states[button - 100] = KeyStates::PRESSED;
                    } else if (states[button] != KeyStates::PRESSED) {
                        // is first trigger
                        states[button] = KeyStates::TRAVELLING;
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
    midiInit();

    midiSetHandleNoteOn([](uint8_t channel, uint8_t note, uint8_t velocity) {
        Player::getInstance().handleNoteOn(note, velocity, true);
    });
    midiSetHandleNoteOff([](uint8_t channel, uint8_t note, uint8_t velocity) {
        Player::getInstance().handleNoteOff(note, velocity, true);
    });
    midiSetHandleControlChange([](uint8_t channel, uint8_t control, uint8_t value) {
        Player::getInstance().handleMidiControlChange(channel, control, value);
    });
    midiSetHandleClock([]() { Player::getInstance().clockTick(true); });
    midiSetHandleStart([]() { Player::getInstance().handleMidiStart(); });
    midiSetHandleStop([]() { Player::getInstance().handleMidiStop(); });
    midiSetHandleContinue([]() { Player::getInstance().handleMidiContinue(); });

    clockTimer.begin([]() { Player::getInstance().clockTick(false); });
}

void Player::update(float dt) {
    midiRead(midiChannel);

    readKeyBoard();

    float tickDuration = getClockStepSeconds(settings[PLS_RATE]);
    clockTimer.setIntervalMicroseconds((uint32_t)(1000000 * tickDuration));
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
    printf("Recording %d notes...\n", size);
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