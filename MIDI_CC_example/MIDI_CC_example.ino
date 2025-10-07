#include "MIDIUSB.h"

// --- Pin and Variable Definitions ---
const int NUM_POTS = 2; // Total number of potentiometers

// Assign analog pins to an array
const int POT_PINS[NUM_POTS] = {A0, A1};

// Assign a unique MIDI CC number to each potentiometer
const int MIDI_CC_NUMBERS[NUM_POTS] = {1, 2}; // e.g., CC#1, CC#2

// Store the last sent value for each pot to avoid flooding the MIDI bus
int lastSentValues[NUM_POTS];

// --- MIDI Helper Function ---

// Sends a MIDI Control Change message
// event type 0x0B = control change
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// --- Main Program ---

void setup() {
  // Initialize last sent values to an impossible number (-1)
  // to ensure the first reading is always sent.
  for (int i = 0; i < NUM_POTS; i++) {
    lastSentValues[i] = -1;
  }
}

void loop() {
  // Loop through each potentiometer
  for (int i = 0; i < NUM_POTS; i++) {
    // 1. Read the raw sensor value (0-1023) from the current pot
    int sensorValue = analogRead(POT_PINS[i]);

    // 2. Map the sensor value to the MIDI CC range (0-127)
    int midiValue = map(sensorValue, 0, 1023, 0, 127);

    // 3. Only send a message if the value for this pot has changed
    if (midiValue != lastSentValues[i]) {
      
      // Send the MIDI CC message for the corresponding pot
      controlChange(0, MIDI_CC_NUMBERS[i], midiValue); // Channel 0
      MidiUSB.flush();

      // Update the last value sent for this specific pot
      lastSentValues[i] = midiValue;
    }
  }

  // A small delay to keep readings stable after checking all pots
  delay(10);
}