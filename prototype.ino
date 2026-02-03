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

// Button configuration
const int buttonPin = A1;
const int buttonMidiNote = 37;  // C#2 (or choose your preferred note)
const int buttonMidiChannel = 0; // Channel 1
const unsigned long buttonDebounceTime = 50; // milliseconds for button debouncing

bool buttonState = HIGH;        // Current button state (HIGH = not pressed, LOW = pressed)
bool lastButtonState = HIGH;    // Previous button state
unsigned long lastButtonDebounceTime = 0;
bool buttonNoteOn = false;      // Track if note is currently on

void setup() {
  pinMode(piezoPin, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); // Enable internal pull-up resistor
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

  // BUTTON HANDLING
  handleButton();
}

/* -------- BUTTON HANDLING -------- */

void handleButton() {
  // Read the button state
  int reading = digitalRead(buttonPin);
  
  // Debounce: check if enough time has passed since last state change
  if (reading != lastButtonState) {
    lastButtonDebounceTime = millis();
  }
  
  // If debounce time has passed, update the button state
  if ((millis() - lastButtonDebounceTime) > buttonDebounceTime) {
    // State has changed
    if (reading != buttonState) {
      buttonState = reading;
      
      // Button pressed (LOW because of pull-up)
      if (buttonState == LOW && !buttonNoteOn) {
        noteOn(buttonMidiChannel, buttonMidiNote, 127); // Full velocity
        MidiUSB.flush();
        buttonNoteOn = true;
      }
      // Button released (HIGH)
      else if (buttonState == HIGH && buttonNoteOn) {
        noteOff(buttonMidiChannel, buttonMidiNote, 0);
        MidiUSB.flush();
        buttonNoteOn = false;
      }
    }
  }
  
  lastButtonState = reading;
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

