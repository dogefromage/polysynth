#include "midis.h"

#include <Arduino.h>

MIDI_CREATE_INSTANCE(HardwareSerial, Serial8, MIDI);

void midiInit() {
    MIDI.begin(MIDI_CHANNEL_OMNI);
}

void midiRead(int channel) {
    MIDI.read(channel);
    usbMIDI.read(channel);
}

void midiSendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel) {
    MIDI.sendNoteOn(note, velocity, channel);
    usbMIDI.sendNoteOn(note, velocity, channel);
}

void midiSendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel) {
    MIDI.sendNoteOff(note, velocity, channel);
    usbMIDI.sendNoteOff(note, velocity, channel);
}

void midiSendClock() {
    MIDI.sendClock();
    usbMIDI.sendClock();
}

void midiSetHandleNoteOn(void (*callback)(uint8_t channel, uint8_t note, uint8_t velocity)) {
    MIDI.setHandleNoteOn(callback);
    usbMIDI.setHandleNoteOn(callback);
}

void midiSetHandleNoteOff(void (*callback)(uint8_t channel, uint8_t note, uint8_t velocity)) {
    MIDI.setHandleNoteOff(callback);
    usbMIDI.setHandleNoteOff(callback);
}

void midiSetHandleControlChange(void (*callback)(uint8_t channel, uint8_t control, uint8_t value)) {
    MIDI.setHandleControlChange(callback);
    usbMIDI.setHandleControlChange(callback);
}

void midiSetHandleClock(void (*callback)(void)) {
    MIDI.setHandleClock(callback);
    usbMIDI.setHandleClock(callback);
}

void midiSetHandleStart(void (*callback)(void)) {
    MIDI.setHandleStart(callback);
    usbMIDI.setHandleStart(callback);
}

void midiSetHandleStop(void (*callback)(void)) {
    MIDI.setHandleStop(callback);
    usbMIDI.setHandleStop(callback);
}

void midiSetHandleContinue(void (*callback)(void)) {
    MIDI.setHandleContinue(callback);
    usbMIDI.setHandleContinue(callback);
}
