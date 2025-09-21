#pragma once
#include <cstdint>

#include "instrument.h"
#include "keybed.h"
#include "led.h"

#define NOTE_BUFFER_MAX_SIZE 256

#define MIDI_SEND_CHANNEL 5

enum PlayerState {
    PLSTATE_NORMAL,
    PLSTATE_ARP,
    PLSTATE_SEQ_RECORDING,
    PLSTATE_SEQ_PLAYING,
};

enum PlayerSettings {
    PLS_HOLDING,
    PLS_TRANSPOSING,
    PLS_MIDICLOCK,
    PLS_RATE,
    PLS_ARP_MODE,
    PLS_ARP_RANGE,
    PLS_OCTAVE_OFFSET,
    PLS__COUNT__,
};

enum class SongMode {
    Playing,
    Paused
};

class Player {
    static Player* instance;

    Instrument& instr;
    PanelLedController& leds;

    Keybed keybed;

    PlayerState state = PLSTATE_NORMAL;
    int16_t settings[PLS__COUNT__] = {};
    int keyboardTransposition = 0;
    int midiChannel = 0;  // zero means all channels, 1-16 specific

    // used for both arpeggios and sequence
    int noteBuffer[NOTE_BUFFER_MAX_SIZE] = {};
    int noteBufferSize = 0;
    int sequenceLength;

    int noteBufPosition = -1;
    bool noteUpStep = false;
    bool arpDownwards = false;
    int arpLayer = 0;
    // float timeSinceTick = 0;
    int ticksSinceStep = 0;
    int lastStepNote = 0;

    SongMode songMode = SongMode::Playing;

    void setState(PlayerState nextState);
    void pushSequencerNote(int note);
    void setTransposition(int note);
    void step();

    void clockTick(bool isMidi);
    void handleNoteOn(int note, int velocity, bool isMidi);
    void handleNoteOff(int note, int velocity, bool isMidi);
    void handleMidiControlChange(uint8_t channel, uint8_t control, uint8_t value);
    void handleMidiStart();
    void handleMidiStop();
    void handleMidiContinue();

   public:
    Player(Instrument&, PanelLedController& leds);

    static Player& getInstance();

    void init();
    void update(float dt);
    PlayerState getState();
    int16_t* getSettingsList();
    void setStateNormal();
    void setStateArp();
    void setStateSeqRecording(int size);
    void pushBlankNote();
    void toggleMidiChannel(int channel);
    void testKeyBed();
    int getMidiChannel();
    void resetClockProgress();
    void setSongMode(SongMode mode);
    void updateArpSequence();
    int keyToNote(int key);

    Player(const Player&) = delete;
};
