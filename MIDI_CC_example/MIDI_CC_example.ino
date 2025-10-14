#include "MIDIUSB.h"

// --- MIDI Helper Function (global) ---
void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}

// --- Potentiometer Class (sem alterações) ---
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

// --- Classe PiezoDrumTrigger com a nova função ---
class PiezoDrumTrigger {
  private:
    byte pin;
    byte ccNumber;
    int threshold;
    unsigned long maskTime;
    unsigned long lastHitTime;

  public:
    // Constructor
    PiezoDrumTrigger(byte analogPin, byte midiCcNumber, int triggerThreshold, unsigned long cooldown) {
      pin = analogPin;
      ccNumber = midiCcNumber;
      threshold = triggerThreshold;
      maskTime = cooldown;
      lastHitTime = 0;
    }

    // Função de operação normal
    void update() {
      if (millis() - lastHitTime < maskTime) {
        return;
      }
      
      int sensorValue = analogRead(pin);

      if (sensorValue > threshold) {
        int midiValue = map(sensorValue, threshold, 1023, 0, 127);
        midiValue = constrain(midiValue, 0, 127);

        controlChange(0, ccNumber, midiValue);
        controlChange(0, ccNumber, 0);

        MidiUSB.flush();
        
        lastHitTime = millis();
      }
    }

    // --- NOVA FUNÇÃO DE MONITORAMENTO ---
    // Esta função lê o pino e imprime o valor no monitor serial.
    void monitorSerial() {
      int sensorValue = analogRead(pin);
      Serial.println(sensorValue);
    }
};


// --- Programa Principal ---

// 1. Instancia os Potenciômetros
Potentiometer pots[] = {
  Potentiometer(A0, 1), 
  Potentiometer(A1, 2)
};
const int NUM_POTS = sizeof(pots[0]) / sizeof(pots[0]);


// 2. Instancia o Gatilho de Bateria Piezo
PiezoDrumTrigger myDrum(A8, 3, , 50);

void setup() {
  // Inicia a comunicação serial para podermos usar a função de monitoramento
  Serial.begin(9600);
}

void loop() {
  // --- MODO DE CALIBRAÇÃO ---
  // Para calibrar o piezo, comente a operação normal e descomente a linha abaixo.
  // Depois, abra o Serial Plotter (Ferramentas > Serial Plotter).
  
  myDrum.monitorSerial();

  // --- MODO DE OPERAÇÃO NORMAL ---
  // Depois de calibrar, comente a linha myDrum.monitorSerial()
  // e descomente as linhas abaixo.
  
  for (int i = 0; i < NUM_POTS; i++) {
    pots[i].update();
  }
  myDrum.update();
  
  delay(5); // Um pequeno delay para estabilizar a leitura serial
}