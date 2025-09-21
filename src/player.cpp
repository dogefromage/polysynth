#include "player.h"

#include <Arduino.h>

#include <unordered_set>

#include "SPIWrapper.h"
#include "StableTimer.h"
#include "config.h"
#include "midis.h"
#include "utils.h"

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
        debugprintf("sequencer not recording!\n");
        return;
    }
    if (noteBufferSize >= sequenceLength) {
        debugprintf("sequence already full!\n");
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
    for (int i = 0; i < NUM_KEYS; i++) {
        bool isPressed = keybed.getKeyStates()[i] != KeyStates::OPEN;
        if (isPressed) {
            int note = keyToNote(i);
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
        debugprintf("note buffer empty, returning\n");
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

    debugprintf("transposing to %d\n", keyboardTransposition);
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
            midiSendNoteOn(note, velocity, MIDI_SEND_CHANNEL);
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
            midiSendNoteOff(note, velocity, MIDI_SEND_CHANNEL);
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

void Player::testKeyBed() {
    keybed.test();
}

int Player::getMidiChannel() {
    return midiChannel;
}

Player* Player::instance = NULL;

Player::Player(Instrument& instr, PanelLedController& leds) : instr(instr), leds(leds) {
    Player::instance = this;
}

void Player::init() {
    midiInit();

    midiSetHandleNoteOn([](uint8_t channel, uint8_t note, uint8_t velocity) {
        debugprintf("MIDI note on received %x %x %x\n", note, velocity, channel);
        Player::getInstance().handleNoteOn(note, velocity, true);
    });
    midiSetHandleNoteOff([](uint8_t channel, uint8_t note, uint8_t velocity) {
        debugprintf("MIDI note off received %x %x %x\n", note, velocity, channel);
        Player::getInstance().handleNoteOff(note, velocity, true);
    });
    midiSetHandleControlChange([](uint8_t channel, uint8_t control, uint8_t value) {
        debugprintf("MIDI control change received %x %x %x\n", channel, control, value);
        Player::getInstance().handleMidiControlChange(channel, control, value);
    });
    midiSetHandleClock([]() { Player::getInstance().clockTick(true); });
    midiSetHandleStart([]() { Player::getInstance().handleMidiStart(); });
    midiSetHandleStop([]() { Player::getInstance().handleMidiStop(); });
    midiSetHandleContinue([]() { Player::getInstance().handleMidiContinue(); });

    clockTimer.begin([]() { Player::getInstance().clockTick(false); });

    keybed.setHandleKeyDown([](int key, int velocity) {
        int note = Player::getInstance().keyToNote(key);
        Player::getInstance().handleNoteOn(note, velocity, false);
    });

    keybed.setHandleKeyUp([](int key) {
        int note = Player::getInstance().keyToNote(key);
        Player::getInstance().handleNoteOff(note, 0, false);
    });

    keybed.init();
}

void Player::update(float dt) {
    midiRead(midiChannel);

    keybed.update();

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
    debugprintf("Recording %d notes...\n", size);
    if (size <= 0) {
        debugprintf("invalid seq size %d\n", size);
        return;
    }
    if (size >= NOTE_BUFFER_MAX_SIZE) {
        debugprintf("seq size too large for note buffer\n");
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
