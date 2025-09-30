#include "MIDIUSB.h"

// --- Pin and Variable Definitions ---
const int POTENTIOMETER_PIN = A0; // The analog pin for the potentiometer
const int MIDI_CC_NUMBER = 1;     // The MIDI CC number for this control (1 = Mod Wheel)
int lastSentValue = -1;           // Stores the last value sent to avoid flooding

// --- MIDI Helper Function ---

// Sends a MIDI Control Change message
// event type 0x0B = control change
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// --- Main Program ---

void setup() {
  // No setup required for this sketch
}

void loop() {
  // 1. Read the raw sensor value (0-1023)
  int sensorValue = analogRead(POTENTIOMETER_PIN);

  // 2. Map the sensor value to the MIDI CC range (0-127)
  int midiValue = map(sensorValue, 0, 1023, 0, 127);

  // 3. Only send a message if the value has changed
  if (midiValue != lastSentValue) {
    
    // Send the MIDI CC message
    controlChange(0, MIDI_CC_NUMBER, midiValue); // Channel 0
    MidiUSB.flush();

    // Update the last value sent
    lastSentValue = midiValue;
  }

  // A small delay to keep readings stable
  delay(10);
}