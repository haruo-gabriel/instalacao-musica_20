#include "MIDIUSB.h"

// --- MIDI Helper Function (remains global) ---
// Sends a MIDI Control Change message
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// --- Potentiometer Class Definition ---
class Potentiometer {
  private:
    byte pin;         // The analog pin the potentiometer is connected to
    byte ccNumber;    // The MIDI CC number for this potentiometer
    int lastValue;    // The last MIDI value sent

  public:
    // Constructor: runs when a new Potentiometer object is created
    Potentiometer(byte analogPin, byte midiCcNumber) {
      pin = analogPin;
      ccNumber = midiCcNumber;
      lastValue = -1; // Initialize to an invalid value to force the first send
    }

    // Update function: reads the pot and sends MIDI if the value has changed
    void update() {
      // 1. Read the raw sensor value (0-1023)
      int sensorValue = analogRead(pin);

      // 2. Map the value to the MIDI range (0-127)
      int midiValue = map(sensorValue, 0, 1023, 0, 127);

      // 3. Only send a message if the value has changed
      if (midiValue != lastValue) {
        controlChange(0, ccNumber, midiValue); // Send on MIDI channel 0
        MidiUSB.flush();
        lastValue = midiValue; // Update the last sent value
      }
    }
};

// --- Main Program ---

// Create instances of the Potentiometer class in an array
Potentiometer pots[] = {
  Potentiometer(A0, 1),  // Potentiometer 1 on pin A0, sending on CC #1
  Potentiometer(A1, 2)   // Potentiometer 2 on pin A1, sending on CC #2
};

// Calculate the number of potentiometers based on the array size
const int NUM_POTS = sizeof(pots) / sizeof(pots[0]);


void setup() {
  // Setup is now handled by the class constructors, so this is empty.
}

void loop() {
  // Loop through each potentiometer object and call its update function
  for (int i = 0; i < NUM_POTS; i++) {
    pots[i].update();
  }

  // A small delay to keep readings stable
  delay(10);
}