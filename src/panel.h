#pragma once
#include <stdint.h>

#include "instrument.h"
#include "patch.h"
#include "player.h"
#include "led.h"

// amount a fader must move such
// that it goes into panel mode
#define FADER_NUDGE_THRESHOLD 100

// extend remaining panel faders
enum PanelFaders {
    FD_PB_BEND = PATCH_FD__COUNT__,
    FD_PB_MOD,
    FD_PB_MOD_VCO,
    FD_PB_MOD_VCF,
    FD_CTRL_RATE,
    FD_OUTPUT_VOLUME,
    PANEL_FD__COUNT__,
};

enum PanelSwitches {
    SW_BEND_OCTAVE = PATCH_SW__COUNT__,
    SW_RANGE_UP,
    SW_RANGE_DOWN,
    SW_HOLD,
    SW_KEY_TRANSPOSE,
    SW_MIDI_SYNC,
    SW_ARP_EN,
    SW_ARP_MODE,
    SW_ARP_RANGE,
    SW_SEQ_RECORD,
    SW_SEQ_BLANK,
    SW_PROG_LOAD,
    SW_PROG_STORE,
    SW_PROG_MIDI_CH,
    SW_PROG_RETUNE,
    SW_PROG_LOAD_PANEL,
    SW_PROG_NUM_01,
    SW_PROG_NUM_02,
    SW_PROG_NUM_03,
    SW_PROG_NUM_04,
    SW_PROG_NUM_05,
    SW_PROG_NUM_06,
    SW_PROG_NUM_07,
    SW_PROG_NUM_08,
    SW_PROG_NUM_09,
    SW_PROG_NUM_10,
    SW_PROG_NUM_11,
    SW_PROG_NUM_12,
    SW_PROG_NUM_13,
    SW_PROG_NUM_14,
    SW_PROG_NUM_15,
    SW_PROG_NUM_16,
    PANEL_SW__COUNT__,
};

struct PanelElement {
    int16_t current, last, lastActive;
    bool active = true;
};

class Panel {
    Instrument& instr;
    Player& player;
    PanelLedController& leds;

    PanelElement faders[PANEL_FD__COUNT__];
    PanelElement switches[PANEL_SW__COUNT__];

    void updateStatefulFader(int faderIndex, int16_t* target);
    void updateStatefulSwitch(int switchIndex, int8_t* target);
    bool isClicked(int switchIndex);
    bool isClickedEarly(int switchIndex);
    bool isHeld(int switchIndex);
    int getClickedNumber();
    void setPanelInputsActivity(bool active);
    
   public:
    Panel(Instrument&, Player&, PanelLedController&);

    void read();
    void update();

    void test_print_raw_matrix();
    void test_print_panel_elements();

    Panel(const Panel&) = delete;
};
