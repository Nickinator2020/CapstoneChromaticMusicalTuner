/**
 * File: main.ino
 * Description: Capstone Musical Tuner Code
 * 
 * Sources: 
 *  *Button Debounce Logic:
 *      Website: DigitKey
 *      Link: https://www.digikey.com/en/maker/tutorials/2024/how-to-implement-a-software-based-debounce-algorithm-for-button-inputs-on-a-microcontroller
 */

// BEGIN LIBRARIES
#include "arduinoFFT.h"
// END LIBRARIES

// BEGIN DEFINES
#define SAMPLES_TAKEN 512
#define DIFFERENCE_ERROR 20 // Threshold
#define DEBOUNCE_TIME 50    // Debounce time in milliseconds

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

// BEGIN FUNCTION PROTOTYPES
void debounceButtonHandler(pin_size_t pin, bool *pinState, bool *pinLastState, unsigned long *lastDebounceTime, void (*buttonHandler)(void));
void invertModeSelect();
void upHalfStep();
void downHalfStep();
void upOctave();
void downOctave();
char receiveCharFromSerial();
void PrintVector(double *vData, uint16_t elements, uint8_t scaleType);
// END FUNCTION PROTOTYPES

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

// BEGIN PRIVATE VARIABLES
// I/O Parameters
bool modeSelect = SENSOR_MODE;

Button modeSelectButton = {.pin = MODE_SELECT_PIN, .mode = INPUT_PULLUP, .pinState = HIGH, .pinLastState = HIGH, .debounceTimeMilliseconds = 0, .buttonHandler = invertModeSelect};
Button frequencyShiftButtons[] = {
  {.pin = UP_HALFSTEP_PIN, .mode = INPUT_PULLUP, .pinState = HIGH, .pinLastState = HIGH, .debounceTimeMilliseconds = 0, .buttonHandler = upHalfStep},
  {.pin = DOWN_HALFSTEP_PIN, .mode = INPUT_PULLUP, .pinState = HIGH, .pinLastState = HIGH, .debounceTimeMilliseconds = 0, .buttonHandler = downHalfStep},
  {.pin = UP_OCTAVE_PIN, .mode = INPUT_PULLUP, .pinState = HIGH, .pinLastState = HIGH, .debounceTimeMilliseconds = 0, .buttonHandler = upOctave},
  {.pin = DOWN_OCTAVE_PIN, .mode = INPUT_PULLUP, .pinState = HIGH, .pinLastState = HIGH, .debounceTimeMilliseconds = 0, .buttonHandler = downOctave},
};
const int frequencyShiftButtonsLength = sizeof(frequencyShiftButtons) / sizeof(frequencyShiftButtons[0]);


// ADC Parameters
const int serialBaudRate = 9600;
const int ADCPin = A5;
const float maxVoltagePkPk = 5;
const int ADCBitResolution = 12;
const long SAMPLE_PERIOD_MICROSECONDS = 119; // (8400 Hz)
const double SAMPLE_FREQUENCY = 8400;


int samples[SAMPLES_TAKEN];
int sumOfSamples = 0; // Used for average

char TESTING_receivedChar = '\0';

// For FFT
arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
double vReal[SAMPLES_TAKEN];
double vImag[SAMPLES_TAKEN];
double voltageValue = 0;
double stepSize = maxVoltagePkPk / (pow(2,ADCBitResolution) - 1);
 
// For DAC
float fundamentalFrequencies[] = {
  C1, CSharp1, D1, DSharp1, E1, F1, FSharp1, G1, GSharp1, A1, ASharp1, B1,
  C2, CSharp2, D2, DSharp2, E2, F2, FSharp2, G2, GSharp2, A2, ASharp2, B2,
  C3, CSharp3, D3, DSharp3, E3, F3, FSharp3, G3, GSharp3, A3, ASharp3, B3,
  C4, CSharp4, D4, DSharp4, E4, F4, FSharp4, G4, GSharp4, A4, ASharp4, B4,
  C5, CSharp5, D5, DSharp5, E5, F5, FSharp5, G5, GSharp5, A5, ASharp5, B5,
  C6, CSharp6, D6, DSharp6, E6, F6, FSharp6, G6, GSharp6, A6, ASharp6, B6,
  C7, CSharp7, D7, DSharp7, E7, F7, FSharp7, G7, GSharp7, A7, ASharp7, B7,
  C8 
};
const int fundamentalFrequenciesArrayLength = sizeof(fundamentalFrequencies)/sizeof(fundamentalFrequencies[0]);
int DACFreqCurrentIndex = 0;
// END PRIVATE VARIABLES

// BEGIN setup() and loop()

void setup() {
  // Configure Pins
  pinMode(modeSelectButton.pin, modeSelectButton.mode);

  for(int i = 0; i < frequencyShiftButtonsLength; i++){
    pinMode(frequencyShiftButtons[i].pin, frequencyShiftButtons[i].mode);
  }
  
  pinMode(LED_TX, OUTPUT);
  pinMode(LED_RX, OUTPUT);
  

  // Configure serial port for testing
  Serial.begin(serialBaudRate);

  // Set ADC to appropriate bit resolution
  analogReadResolution(ADCBitResolution); 
}

void loop() {
  
  // Read serial - FOR TESTING PURPOSES
  TESTING_receivedChar = receiveCharFromSerial();
  
  debounceButtonHandler(modeSelectButton.pin, &modeSelectButton.pinState, &modeSelectButton.pinLastState, &modeSelectButton.debounceTimeMilliseconds, modeSelectButton.buttonHandler);
  
  if(modeSelect == SENSOR_MODE){
    // Begin Sensor Mode Logic - indicated by LED_TX. Note, 0 indicates ON
    digitalWrite(LED_TX, 0);
    digitalWrite(LED_RX, 1);
   
  //   if(TESTING_receivedChar == 'y' || TESTING_receivedChar == 'Y'){
  //     Serial.println("TEST Sampling run engaged for SENSOR MODE!!");

  //     // Get samples in specified sampling period
  //     for(int i = 0; i < SAMPLES_TAKEN; i++){
  //       samples[i] = analogRead(ADCPin);
  //       //Serial.println("Sample " + String(i) + ": " + String(samples[i]));
  //       sumOfSamples += samples[i];

  //       // Put voltage value in Real part array
  //       voltageValue = samples[i] * stepSize;
  //       vReal[i] = voltageValue;

  //       delayMicroseconds(SAMPLE_PERIOD_MICROSECONDS);
  //     }

  //     //Serial.println("Sum of Samples: " + String(sumOfSamples));
  //     int average = sumOfSamples / SAMPLES_TAKEN;
  //     Serial.println("Average: " + String(average));
  //     int difference = abs(samples[0] - average);
  //     Serial.println("Difference: " + String(difference));

      
  //     // Check for flatlining - TODO - get rid of this implementation
  //     //TODO: Use LED_BUILTIN for clipping detection
  //     if(difference < DIFFERENCE_ERROR){
  //       Serial.println("Flatlined");
      
  //     }
  //     else{
  //       Serial.println("NOT Flatlined");
  //       // Not flatlining! Start FFT process
  //       // TODO TEST THIS MORE!!!
        
  //       // Starting values before FFT
  //       Serial.println("Voltage Values:");
  //       PrintVector(vReal, SAMPLES_TAKEN, SCL_TIME);
        
  //       // Weigh the Data:
  //       FFT.Windowing(vReal, SAMPLES_TAKEN, FFT_WIN_TYP_HAMMING, FFT_FORWARD);	/* Weigh data */
  //       Serial.println("Weighed data:");
  //       PrintVector(vReal, SAMPLES_TAKEN, SCL_TIME);

  //       // Compute FFT:
  //       FFT.Compute(vReal, vImag, SAMPLES_TAKEN, FFT_FORWARD); /* Compute FFT */
  //       Serial.println("Computed Real values:");
  //       PrintVector(vReal, SAMPLES_TAKEN, SCL_INDEX);
  //       Serial.println("Computed Imaginary values:");
  //       PrintVector(vImag, SAMPLES_TAKEN, SCL_INDEX);

  //       // Compute Magnitudes:
  //       Serial.println("Computed magnitudes:");
  //       FFT.ComplexToMagnitude(vReal, vImag, SAMPLES_TAKEN);
  //       // Since it is mirrored!!!
  //       PrintVector(vReal, (SAMPLES_TAKEN >> 1), SCL_FREQUENCY); 

  //       double x = FFT.MajorPeak(vReal, SAMPLES_TAKEN, SAMPLE_FREQUENCY);
  //       Serial.println("Peak Magnitude (Frequency Value I think!!!):");
  //       Serial.println(x, 6);
  //     }
      
  //     // Clear
  //     sumOfSamples = 0;
  //     memset(vReal, 0, sizeof(vReal));
  //     memset(vImag,0, sizeof(vImag));

  // }

  }
  else{
    // Begin Speaker Mode Logic - indicated by LED_RX. Note, 0 indicates ON
    digitalWrite(LED_TX, 1);
    digitalWrite(LED_RX, 0);

    //TODO: Speaker Mode Logic using DAC - NICK S
    float outputFrequency = fundamentalFrequencies[DACFreqCurrentIndex];
    // TODO Test print output frequency - Can get rid of
    Serial.print(outputFrequency);
    Serial.println(" Hz");
    
    // "Listening" frequency buttons logic
    for(int i = 0; i < frequencyShiftButtonsLength; i++){
      debounceButtonHandler(frequencyShiftButtons[i].pin, &frequencyShiftButtons[i].pinState, &frequencyShiftButtons[i].pinLastState, 
        &frequencyShiftButtons[i].debounceTimeMilliseconds, frequencyShiftButtons[i].buttonHandler);
    }
    
  }

}

// END setup() and loop()


/**
 * Name: debounceButtonHandler
 * Description: Debouncing button logic
 * Params:
 * - pin: GPIO Pin
 * - pinState: Variable used to keep track of the pin state
 * - pinLastState: Variable used to keep track of the last pin state
 * - lastDebounceTime: Variable used to keep track of the last debounce time (in milliseconds)
 * - buttonHandler: function for what to do when debouncing is done
 * 
 * Returns: None
 */
void debounceButtonHandler(pin_size_t pin, bool *pinState, bool *pinLastState, unsigned long *lastDebounceTime, void (*buttonHandler)(void)){
  bool pinReading = digitalRead(pin);
  
  if(pinReading != *pinLastState){
    *lastDebounceTime = millis();
  }

  if ((millis() - *lastDebounceTime) > DEBOUNCE_TIME) {
        // Debounce time has passed, check if the state has stabilized
        if (pinReading != *pinState) {
            *pinState = pinReading; // Update the stable state
            if (*pinState == LOW) {
                (*buttonHandler)();
            }
        }
    }
    *pinLastState = pinReading;
}
/**
 * Name: invertModeSelect
 * Description: Inverts mode select
 * Params: None
 * Returns: None
 */
void invertModeSelect(){
  modeSelect = !modeSelect;
}

/**
 * Name: upHalfStep
 * Description: Increases DAC Frequency by a half step in the chromatic scale ranging from C1 - C8. 
 * Note, only use for SPEAKER MODE
 * Params: None
 * Returns: None
 */
void upHalfStep(){
  if(modeSelect == SPEAKER_MODE){
    DACFreqCurrentIndex++;
    
    // Overflow:
    if(DACFreqCurrentIndex >= fundamentalFrequenciesArrayLength){
      DACFreqCurrentIndex = 0;
    }
  }
}

/**
 * Name: downHalfStep
 * Description: Decreases DAC Frequency by a half step in the chromatic scale ranging from C1 - C8. 
 * Note, only use for SPEAKER MODE
 * Params: None
 * Returns: None
 */
void downHalfStep(){
  if(modeSelect == SPEAKER_MODE){
    DACFreqCurrentIndex--;

    // Underflow:
    if(DACFreqCurrentIndex < 0){
      DACFreqCurrentIndex = (fundamentalFrequenciesArrayLength - 1);
    }
  }
}

/**
 * Name: upOctave
 * Description: Increases DAC Frequency by an octave in the chromatic scale ranging from C1 - C8. 
 * Note, only use for SPEAKER MODE
 * Params: None
 * Returns: None
 */
void upOctave(){
  if(modeSelect == SPEAKER_MODE){
    float currFreq = fundamentalFrequencies[DACFreqCurrentIndex];
    DACFreqCurrentIndex += OCTAVE_DISTANCE;

    // Overflow:
    if(DACFreqCurrentIndex >= fundamentalFrequenciesArrayLength){
      
      DACFreqCurrentIndex = DACFreqCurrentIndex - fundamentalFrequenciesArrayLength + 1;
      // Special Case for C8
      if(currFreq == C8) DACFreqCurrentIndex -= OCTAVE_DISTANCE;
    }
  }
}

/**
 * Name: downOctave
 * Description: Decreases DAC Frequency by an octave in the chromatic scale ranging from C1 - C8. 
 * Note, only use for SPEAKER MODE
 * Params: None
 * Returns: None
 */
void downOctave(){
  if(modeSelect == SPEAKER_MODE){
    float currFreq = fundamentalFrequencies[DACFreqCurrentIndex];
    DACFreqCurrentIndex -= OCTAVE_DISTANCE;

    // Underflow:
    if(DACFreqCurrentIndex < 0){
      DACFreqCurrentIndex = DACFreqCurrentIndex + fundamentalFrequenciesArrayLength - 1;
      // Special Case for C1
      if(currFreq == C1) DACFreqCurrentIndex += OCTAVE_DISTANCE;
    }
  }
}

/**
 * Name: receiveCharFromSerial
 * Description: Function to receive a character from Serial line
 * Assumming that serial line is set up to not include carriage return character and new line character.
 * Params: None
 * Returns: Character of received character else 
 */
char receiveCharFromSerial(){
  char c = '\0';
  if(Serial.available() > 0){
    c = Serial.read();
  }
  return c;
}

/**
 * Name: PrintVector
 * Description: Meant to print the data where each index corresponds to
 *  1. Just an index # (SCL_INDEX)
 *  2. A time unit in seconds (SCL_TIME)
 *  3. A frequency unit in Hz
 * 
 * Parameters:
 * - *vData: the data buffer
 * - elements: how many elements to print out
 * - scaleType: either SCL_INDEX, SCL_TIME, or SCL_FREQUENCY
 */
void PrintVector(double *vData, uint16_t elements, uint8_t scaleType)
{
  String unit;
  for (uint16_t i = 0; i < elements; i++)
  {
    double abscissa;
    /* Print abscissa value */
    switch (scaleType)
    {
      case SCL_INDEX:
        abscissa = (i * 1.0);
        unit = " index: ";
	break;
      case SCL_TIME:
        abscissa = ((i * 1.0) / SAMPLE_FREQUENCY);
        unit = " seconds: ";
	break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * SAMPLE_FREQUENCY) / SAMPLES_TAKEN);
        unit = " Hz: ";
	break;
    }
    Serial.print("At ");
    Serial.print(abscissa, 6);
    Serial.print(unit);
    Serial.print(vData[i], 4);
    Serial.println();
  }
  Serial.println();
}