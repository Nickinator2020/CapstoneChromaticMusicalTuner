#ifndef MAIN_H
#define MAIN_H


// BEGIN DEFINES
#define SAMPLES_TAKEN 512
#define DEBOUNCE_TIME 50    // Debounce time in milliseconds
#define CLIPPED_LED_BLINK_TIMES 3
#define CLIPPED_LED_BLINK_DURATION_MS 1000

// For FFT
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02

// FUNDAMENTAL FREQUENCIES
// Octave 1
#define C1 32.70f
#define CSharp1 34.65f
#define D1 36.71f
#define DSharp1 38.89f
#define E1 41.20f
#define F1 43.65f
#define FSharp1 46.25f
#define G1 49.00f
#define GSharp1 51.91f
#define A1 55.00f
#define ASharp1 58.27f
#define B1 61.74f
// Octave 2 
#define C2 65.41f
#define CSharp2 69.30f
#define D2 73.42f
#define DSharp2 77.78f
#define E2 82.41f
#define F2 87.31f
#define FSharp2 92.50f
#define G2 98.00f
#define GSharp2 103.83f
#define A2 110.00f
#define ASharp2 116.54f
#define B2 123.47f
// Octave 3 
#define C3 130.81f
#define CSharp3 138.59f
#define D3 146.83f
#define DSharp3 155.56f
#define E3 164.81f
#define F3 174.61f
#define FSharp3 185.00f
#define G3 196.00f
#define GSharp3 207.65f
#define A3 220.00f
#define ASharp3 233.08f
#define B3 246.94f
// Octave 4
#define C4 261.63f
#define CSharp4 277.18f
#define D4 293.66f
#define DSharp4 311.13f
#define E4 329.63f
#define F4 349.23f
#define FSharp4 369.99f
#define G4 392.00f
#define GSharp4 415.30f
#define A4 440.00f
#define ASharp4 466.16f
#define B4 493.88f
// Octave 5 
#define C5 523.25f
#define CSharp5 554.37f
#define D5 587.33f
#define DSharp5 622.25f
#define E5 659.26f
#define F5 698.46f
#define FSharp5 739.99f
#define G5 783.99f
#define GSharp5 830.61f
#define A5 880.00f
#define ASharp5 932.33f
#define B5 987.77f
// Octave 6
#define C6 1046.50f
#define CSharp6 1108.73f
#define D6 1174.66f
#define DSharp6 1244.51f
#define E6 1381.51f
#define F6 1396.91f
#define FSharp6 1479.98f
#define G6 1567.98f
#define GSharp6 1661.22f
#define A6 1760.00f
#define ASharp6 1864.66f
#define B6 1975.53f
// Octave 7
#define C7 2093.00f
#define CSharp7 2217.46f
#define D7 2349.32f
#define DSharp7 2489.02f
#define E7 2637.02f
#define F7 2793.83f
#define FSharp7 2959.96f
#define G7 3135.96f
#define GSharp7 3322.44f
#define A7 3520.00f
#define ASharp7 3729.31f
#define B7 3951.07f
// Octave 8 
#define C8 4186.01f

#define OCTAVE_DISTANCE 12
// END DEFINES

// BEGIN ENUMS

typedef enum {
  SPEAKER_MODE = 0,
  SENSOR_MODE = 1,
} ModeSelect;

typedef enum {
  NONE = -1,
  MODE_SELECT_PIN = 0,
  UP_HALFSTEP_PIN = 1,
  DOWN_HALFSTEP_PIN = 2,
  UP_OCTAVE_PIN = 3,
  DOWN_OCTAVE_PIN = 4
} GPIOPinNumber;
// END ENUMS

// BEGIN STRUCT DEFINITIONS
typedef struct Button {
   pin_size_t pin;
   PinMode mode;
   bool pinState;
   bool pinLastState;
   unsigned long debounceTimeMilliseconds;
   void (*buttonHandler)(void);
};
// END STRUCT DEFINITIONS

// BEGIN FUNCTION PROTOTYPES
void debounceButtonHandler(pin_size_t pin, bool *pinState, bool *pinLastState, unsigned long *lastDebounceTime, void (*buttonHandler)(void));
void invertModeSelect();
void upHalfStep();
void downHalfStep();
void upOctave();
void downOctave();
char receiveCharFromSerial();
int PrintVector(double *vData, uint16_t elements, uint8_t scaleType, int samples, int sampleFrequency);
void blinkSystemLED(int blinkTimes, int blinkDuration_ms);
// END FUNCTION PROTOTYPES


#endif // MAIN_H