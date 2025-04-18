#pragma once
#include <cstdint>
#include <unordered_map>

#include "instrument.h"
#include "led.h"

#define NOTE_BUFFER_MAX_SIZE 256

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

struct ActiveKey {
    uint32_t travellingMillis;
    bool travelling;
};

enum KeyState {
    KEY_STATE_OPEN,
    KEY_STATE_TRAVELLING,
    KEY_STATE_PRESSED,
};

enum class SongMode {
    Playing,
    Paused
};

class Player {
    static Player* instance;

    Instrument& instr;
    PanelLedController& leds;

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

    SongMode songMode = SongMode::Playing;

    std::unordered_map<int, ActiveKey> activeKeys;

    void setState(PlayerState nextState);
    void pushSequencerNote(int note);
    void bubbleSortBuf();
    void addArpNote(int note);
    void removeArpNote(int note);
    void setTransposition(int note);
    void noteOn(int note, int velocity, bool isMidi);
    void noteOff(int note, int velocity, bool isMidi);
    void step();
    void clockTick(bool isMidi);
    void readKeyBoard();
    void updateKey(int key, KeyState currentState);
    int keyToNote(int key);

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

    void usbMidiStart();
    void usbMidiStop();
    void usbMidiContinue();
    void usbMidiHandleControlChange(uint8_t channel, uint8_t control, uint8_t value);

    Player(const Player&) = delete;

    friend void usbMidiNoteOn(uint8_t channel, uint8_t note, uint8_t velocity);
    friend void usbMidiNoteOff(uint8_t channel, uint8_t note, uint8_t velocity);
    friend void usbMidiHandleControlChange(uint8_t channel, uint8_t control, uint8_t value);
    friend void usbMidiClock();
    friend void usbMidiStart();
    friend void onTimerTick();
};