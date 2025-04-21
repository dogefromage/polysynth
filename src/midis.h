#pragma once

#include <MIDI.h>
#include <usb_midi.h>

void midiInit();
void midiRead(int channel);  // 0 OMNI otherwise 1-16
void midiSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel);
void midiSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel);
void midiSendClock();
void midiSetHandleNoteOn(void (*callback)(uint8_t channel, uint8_t note, uint8_t velocity));
void midiSetHandleNoteOff(void (*callback)(uint8_t channel, uint8_t note, uint8_t velocity));
void midiSetHandleControlChange(void (*callback)(uint8_t channel, uint8_t control, uint8_t value));
void midiSetHandleClock(void (*callback)(void));
void midiSetHandleStart(void (*callback)(void));
void midiSetHandleStop(void (*callback)(void));
void midiSetHandleContinue(void (*callback)(void));
