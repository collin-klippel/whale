#include <MIDIUSB.h>

// Piezo configuration: Analog inputs 0-5 (A0-A5)
const int numPiezos = 6;
const int piezoPins[numPiezos] = {A0, A1, A2, A3, A4, A5};
const int piezoMidiNotes[numPiezos] = {36, 37, 38, 39, 40, 41}; // C2, C#2, D2, D#2, E2, F2
const int piezoMidiChannel = 0;  // Channel 1

const int threshold = 8;        // Sensitive
const int releaseThreshold = 4; // Must fall below this to re-arm
const int scanTime = 10;      // microseconds for peak scan (increased for better detection)
const unsigned long debounceTime = 10; // milliseconds to ignore hits after trigger

bool piezoArmed[numPiezos] = {true, true, true, true, true, true};
unsigned long lastHitTime[numPiezos] = {0, 0, 0, 0, 0, 0};

// Button configuration: Analog inputs 6-11 (A6-A11)
const int numButtons = 6;
const int buttonPins[numButtons] = {A6, A7, A8, A9, A10, A11};
const int buttonMidiNotes[numButtons] = {42, 43, 44, 45, 46, 47}; // F#2, G2, G#2, A2, A#2, B2
const int buttonMidiChannel = 0; // Channel 1
const unsigned long buttonDebounceTime = 50; // milliseconds for button debouncing

bool buttonState[numButtons] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};        // Current button state (HIGH = not pressed, LOW = pressed)
bool lastButtonState[numButtons] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};    // Previous button state
unsigned long lastButtonDebounceTime[numButtons] = {0, 0, 0, 0, 0, 0};
bool buttonNoteOn[numButtons] = {false, false, false, false, false, false};      // Track if note is currently on

void setup() {
  // Initialize piezo pins (A0-A5)
  for (int i = 0; i < numPiezos; i++) {
    pinMode(piezoPins[i], INPUT);
  }
  
  // Initialize button pins (A6-A11) with pull-up resistors
  for (int i = 0; i < numButtons; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  // Handle all piezos (A0-A5)
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

    // RE-ARM piezo once it settles
    if (!piezoArmed[i] && peak < releaseThreshold) {
      piezoArmed[i] = true;
    }
  }

  // Handle all buttons (A6-A11)
  for (int i = 0; i < numButtons; i++) {
    handleButton(i);
  }
}

/* -------- BUTTON HANDLING -------- */

void handleButton(int buttonIndex) {
  // Read the button state
  int reading = digitalRead(buttonPins[buttonIndex]);
  
  // Debounce: check if enough time has passed since last state change
  if (reading != lastButtonState[buttonIndex]) {
    lastButtonDebounceTime[buttonIndex] = millis();
  }
  
  // If debounce time has passed, update the button state
  if ((millis() - lastButtonDebounceTime[buttonIndex]) > buttonDebounceTime) {
    // State has changed
    if (reading != buttonState[buttonIndex]) {
      buttonState[buttonIndex] = reading;
      
      // Button pressed (LOW because of pull-up)
      if (buttonState[buttonIndex] == LOW && !buttonNoteOn[buttonIndex]) {
        noteOn(buttonMidiChannel, buttonMidiNotes[buttonIndex], 127); // Full velocity
        MidiUSB.flush();
        buttonNoteOn[buttonIndex] = true;
      }
      // Button released (HIGH)
      else if (buttonState[buttonIndex] == HIGH && buttonNoteOn[buttonIndex]) {
        noteOff(buttonMidiChannel, buttonMidiNotes[buttonIndex], 0);
        MidiUSB.flush();
        buttonNoteOn[buttonIndex] = false;
      }
    }
  }
  
  lastButtonState[buttonIndex] = reading;
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
}
