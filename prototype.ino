#include <MIDIUSB.h>

const int piezoPin = A0;
const int midiNote = 36;        // C2
const int midiChannel = 0;      // Channel 1

const int threshold = 8;        // Sensitive
const int releaseThreshold = 4; // Must fall below this to re-arm
const int scanTime = 5000;      // microseconds for peak scan (increased for better detection)
const unsigned long debounceTime = 10; // milliseconds to ignore hits after trigger

bool armed = true;
unsigned long lastHitTime = 0;

void setup() {
  pinMode(piezoPin, INPUT);
}

void loop() {
  int peak = readPiezoPeak();

  // FIRE (only once per hit)
  if (armed && peak > threshold) {
    // Debounce: ignore if too soon after last hit
    unsigned long currentTime = millis();
    if (currentTime - lastHitTime > debounceTime) {
      int velocity = map(peak, threshold, 600, 15, 127);
      velocity = constrain(velocity, 1, 127);

      noteOn(midiChannel, midiNote, velocity);
      MidiUSB.flush();

      delay(6);

      noteOff(midiChannel, midiNote, 0);
      MidiUSB.flush();

      lastHitTime = currentTime;
      armed = false; // disarm until vibration stops
    }
  }

  // RE-ARM once the piezo settles
  if (!armed && peak < releaseThreshold) {
    armed = true;
  }
}

/* -------- PIEZO PEAK DETECTION -------- */

int readPiezoPeak() {
  int peak = 0;
  unsigned long start = micros();

  while (micros() - start < scanTime) {
    int val = analogRead(piezoPin);
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
}

