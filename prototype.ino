#include <MIDIUSB.h>

// Piezo configuration: All 12 analog inputs (A0-A11)
const int numPiezos = 12;
const int piezoPins[numPiezos] = {A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11};
const int piezoMidiNotes[numPiezos] = {36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47}; // C2 through B2
const int piezoMidiChannel = 0;  // Channel 1

const int threshold = 240;       // Higher = less sensitive
const int releaseThreshold = 12; // Must fall below this to re-arm
const int scanTime = 10;         // microseconds for peak scan
const unsigned long debounceTime = 50;   // min ms between hits (ignores oscillation retriggers)
const unsigned long rearmDelay = 80;     // min ms before re-arming after a hit (lets piezo settle)

bool piezoArmed[numPiezos] = {true, true, true, true, true, true, true, true, true, true, true, true};
unsigned long lastHitTime[numPiezos] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void setup() {
  // Initialize all 12 piezo pins (A0-A11)
  for (int i = 0; i < numPiezos; i++) {
    pinMode(piezoPins[i], INPUT);
  }
}

void loop() {
  // Handle all 12 piezos (A0-A11)
  for (int i = 0; i < numPiezos; i++) {
    int peak = readPiezoPeak(piezoPins[i]);
    
    // FIRE piezo (only once per hit)
    if (piezoArmed[i] && peak > threshold) {
      // Debounce: ignore if too soon after last hit
      unsigned long currentTime = millis();
      if (currentTime - lastHitTime[i] > debounceTime) {
        int velocity = map(peak, threshold, 600, 15, 127);
        velocity = constrain(velocity, 1, 127);

        noteOn(piezoMidiChannel, piezoMidiNotes[i], velocity);
        MidiUSB.flush();

        delay(6);

        noteOff(piezoMidiChannel, piezoMidiNotes[i], 0);
        MidiUSB.flush();

        lastHitTime[i] = currentTime;
        piezoArmed[i] = false; // disarm until vibration stops
      }
    }

    // RE-ARM piezo only after it settles AND minimum time has passed
    if (!piezoArmed[i] && peak < releaseThreshold) {
      if (millis() - lastHitTime[i] > rearmDelay) {
        piezoArmed[i] = true;
      }
    }
  }
}

/* -------- PIEZO PEAK DETECTION -------- */

int readPiezoPeak(int pin) {
  int peak = 0;
  unsigned long start = micros();

  while (micros() - start < scanTime) {
    int val = analogRead(pin);
    if (val > peak) peak = val;
  }

  return peak;
}

/* -------- MIDI FUNCTIONS -------- */

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t event = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(event);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t event = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(event);
x}
