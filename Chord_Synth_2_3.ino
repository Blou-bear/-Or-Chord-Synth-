#include <MozziGuts.h>
#include <Oscil.h>
#include <tables/saw_analogue512_int8.h>

#define JOYSTICK_X A6  // Pitch bend
#define JOYSTICK_Y A7  // Modulation

// Define oscillators (4 voices per note)
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> osc1(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> osc2(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> osc3(SAW_ANALOGUE512_DATA);
Oscil<SAW_ANALOGUE512_NUM_CELLS, AUDIO_RATE> osc4(SAW_ANALOGUE512_DATA);

// Define button pins
const int buttonPins[12] = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}; // Note buttons
const int functionButtons[4] = {A0, A1, A2, A3}; // Function buttons A, B, C, D

// Chromatic scale from F3 (~174.61 Hz)
const float noteFrequencies[12] = {
  174.61, 185.00, 196.00, 207.65, 220.00, 233.08, 
  246.94, 261.63, 277.18, 293.66, 311.13, 329.63
};

float pitchBendAmount = 2.0; // Range in semitones
float modulationDepth = 0.02; // LFO depth

// Interval settings for each function button (in half steps)
const int intervalSets[4][3] = {
  {4, 7, 12},  // A: Major (Root, +4, +7, +12)
  {3, 7, 12},  // B: Minor (Root, +3, +7, +12)
  {4, 7, 11},  // C: Major 7 (Root, +4, +7, +11)
  {3, 7, 11}   // D: Minor 7 (Root, +3, +7, +11)
};

// Active function index (default to A)
int activeFunction = 0;
float baseFreq = 0;  // Store frequency globally

void setup() {
  pinMode(JOYSTICK_X, INPUT);
  pinMode(JOYSTICK_Y, INPUT);
  startMozzi();
  
  // Initialize oscillators
  osc1.setFreq(0); 
  osc2.setFreq(0);
  osc3.setFreq(0);
  osc4.setFreq(0);

  // Set button pins as inputs with internal pull-up resistors
  for (int i = 0; i < 12; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  
  // Set function button pins as inputs with internal pull-up resistors
  for (int i = 0; i < 4; i++) {
    pinMode(functionButtons[i], INPUT_PULLUP);
  }
}

void updateControl() {
  bool notePlayed = false;

  // Check function buttons (set activeFunction based on which is pressed)
  for (int i = 0; i < 4; i++) {
    if (digitalRead(functionButtons[i]) == LOW) {
      activeFunction = i;
      break; // only set to the first pressed function button
    }
  }

  // Check note buttons
  for (int i = 0; i < 12; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {  // Active LOW (button pressed)
      baseFreq = noteFrequencies[i];

      // Explicitly cast intervals to float for accurate frequency calculation
      osc1.setFreq(baseFreq);  // Root note
      osc2.setFreq((float)(baseFreq * pow(2.0, (float)intervalSets[activeFunction][0] / 12.0))); // Interval 1
      osc3.setFreq((float)(baseFreq * pow(2.0, (float)intervalSets[activeFunction][1] / 12.0))); // Interval 2
      osc4.setFreq((float)(baseFreq * pow(2.0, (float)intervalSets[activeFunction][2] / 12.0))); // Interval 3

      notePlayed = true;
      break; // Monophonic: play only one note at a time
    }
  }

  // Read joystick inputs
  int pitchValue = analogRead(JOYSTICK_X) - 512;  // Centered at 0
  int modValue = analogRead(JOYSTICK_Y);

  // Pitch Bend: Map joystick X (-512 to 512) to pitch range (-pitchBendAmount to +pitchBendAmount semitones)
  float pitchBendFactor = pow(2.0, (pitchValue / 512.0) * (pitchBendAmount / 12.0));

  // Modulation: Map joystick Y (0-1023) to LFO effect
  float modAmount = (modValue / 1023.0) * modulationDepth;

  // Apply pitch bend and modulation to oscillators
  osc1.setFreq(baseFreq * pitchBendFactor);
  osc2.setFreq((float)(baseFreq * powf(2.0f, 4.0f / 12.0f) * pitchBendFactor));
  osc3.setFreq((float)(baseFreq * powf(2.0f, 7.0f / 12.0f) * pitchBendFactor));
  osc4.setFreq((float)(baseFreq * powf(2.0f, 13.0f / 12.0f) * pitchBendFactor));

// Optional: Apply modulation effect (simple vibrato)
  osc1.setFreq((float)(baseFreq * (1.0f + modAmount)));


  // Stop oscillators if no button is pressed
  if (!notePlayed) {
    osc1.setFreq(0);
    osc2.setFreq(0);
    osc3.setFreq(0);
    osc4.setFreq(0);
  }
}

int updateAudio() {
  return (osc1.next() + osc2.next() + osc3.next() + osc4.next()) >> 2; // Mix voices
}

void loop() {
  audioHook();
}
