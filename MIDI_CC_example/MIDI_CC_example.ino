#include "MIDIUSB.h"

// --- MIDI Helper Function (global) ---
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// --- Potentiometer Class ---
class Potentiometer {
  private:
    byte pin;
    byte ccNumber;
    int lastValue;

  public:
    Potentiometer(byte analogPin, byte midiCcNumber) {
      pin = analogPin;
      ccNumber = midiCcNumber;
      lastValue = -1;
    }
    void update() {
      int sensorValue = analogRead(pin);
      int midiValue = map(sensorValue, 0, 1023, 0, 127);
      if (midiValue != lastValue) {
        controlChange(0, ccNumber, midiValue);
        MidiUSB.flush();
        lastValue = midiValue;
      }
    }
};

// --- PiezoDrumTrigger Class with Linear Decay ---
class PiezoDrumTrigger {
  private:
    byte pin;
    byte ccNumber;
    int threshold;
    unsigned long maskTime;
    unsigned long lastHitTime;
    
    // Variables for decay control
    unsigned long decayTime;
    bool isDecaying;
    unsigned long decayStartTime;
    int decayStartValue;
    int lastSentValue;

  public:
    // Constructor
    PiezoDrumTrigger(byte analogPin, byte midiCcNumber, int triggerThreshold, unsigned long cooldown, unsigned long decayDuration) {
      pin = analogPin;
      ccNumber = midiCcNumber;
      threshold = triggerThreshold;
      maskTime = cooldown;
      decayTime = decayDuration;
      lastHitTime = 0;
      isDecaying = false;
      lastSentValue = 0;
    }

    // Update function with explicit linear decay logic
    void update() {
      // PART 1: Handle an active decay ramp.
      // This section runs on every loop AFTER a hit has been detected.
      if (isDecaying) {
        // Calculate how much time has passed since the initial hit.
        unsigned long elapsedTime = millis() - decayStartTime;

        // If the decay duration is over, stop the process.
        if (elapsedTime >= decayTime) {
          isDecaying = false; // Stop the decay.
          if (lastSentValue != 0) { // Ensure the final value sent is exactly zero.
            controlChange(0, ccNumber, 0);
            MidiUSB.flush();
            lastSentValue = 0;
          }
          return; // Exit, ready for a new hit.
        }

        // If still decaying, calculate the current value for the linear ramp down.
        // The map() function creates a straight line from the peak value down to zero over the decayTime.
        int currentValue = map(elapsedTime, 0, decayTime, decayStartValue, 0);
        
        // Send the new MIDI value only if it has changed from the last one.
        if (currentValue != lastSentValue) {
          controlChange(0, ccNumber, currentValue);
          MidiUSB.flush();
          lastSentValue = currentValue;
        }
        return; // Exit after processing the decay step.
      }

      // PART 2: Look for a new hit (only runs if not currently decaying).
      if (millis() - lastHitTime < maskTime) {
        return; // Wait for the cooldown period to pass.
      }
      
      int sensorValue = analogRead(pin);

      // A hit is detected if the sensor value is above the threshold.
      if (sensorValue > threshold) {
        // Calculate the peak value of the hit.
        int midiValue = map(sensorValue, threshold, 1023, 0, 127);
        midiValue = constrain(midiValue, 0, 127);

        // Send the initial peak value immediately. THIS HAPPENS ONLY ONCE.
        controlChange(0, ccNumber, midiValue);
        MidiUSB.flush();
        
        // Now, set up the state to start the linear decay from the next loop cycle.
        isDecaying = true;             // This activates PART 1 on the next loop.
        decayStartTime = millis();     // Mark the exact time of the hit.
        decayStartValue = midiValue;   // Set the starting point for the ramp.
        lastSentValue = midiValue;     // Record the value we just sent.
        
        // Start the cooldown timer to prevent double triggers.
        lastHitTime = millis();
      }
    }

    void monitorSerial() {
      int sensorValue = analogRead(pin);
      Serial.println(sensorValue);
    }
};

// --- PressureSensor Class ---
class PressureSensor {
  private:
    byte pin;
    byte ccNumber;
    int threshold;
    int lastValue;
    static const int numReadings = 10;
    int readings[numReadings];
    int readIndex;
    long total;

  public:
    PressureSensor(byte analogPin, byte midiCcNumber, int activeThreshold) {
      pin = analogPin;
      ccNumber = midiCcNumber;
      threshold = activeThreshold;
      lastValue = -1;
      for (int i = 0; i < numReadings; i++) { readings[i] = 0; }
      total = 0;
      readIndex = 0;
    }

    void update() {
      total -= readings[readIndex];
      readings[readIndex] = analogRead(pin);
      total += readings[readIndex];
      readIndex = (readIndex + 1) % numReadings;
      int smoothedValue = total / numReadings;

      int midiValue = 0;
      if (smoothedValue > threshold) {
        midiValue = map(smoothedValue, threshold, 1023, 0, 127);
      }
      midiValue = constrain(midiValue, 0, 127);

      if (midiValue != lastValue) {
        controlChange(0, ccNumber, midiValue);
        MidiUSB.flush();
        lastValue = midiValue;
      }
    }

    void monitorSerial() {
      int sensorValue = analogRead(pin);
      Serial.println(sensorValue);
    }
};


// --- Main Program ---

// 1. Instantiate Potentiometers
Potentiometer pots[] = {
  Potentiometer(A0, 1),
  Potentiometer(A1, 2)
};
const int NUM_POTS = sizeof(pots) / sizeof(pots[0]);

// 2. Instantiate Piezo Drum Trigger
// PiezoDrumTrigger(pin, cc, threshold, maskTime_ms, decayTime_ms)
PiezoDrumTrigger myDrum(A11, 3, 300, 50, 1000);

// 3. Instantiate Pressure Sensor
// PressureSensor(pin, cc, threshold)
PressureSensor myPressure(A8, 4, 30);

void setup() {
  Serial.begin(9600);
}

void loop() {
  // --- CALIBRATION MODE ---
  // myDrum.monitorSerial();
  // myPressure.monitorSerial();

  // --- NORMAL OPERATION MODE ---
  for (int i = 0; i < NUM_POTS; i++) {
    pots[i].update();
  }
  myDrum.update();
  myPressure.update();
  
  delay(5);
}