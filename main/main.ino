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
#include <math.h>
#include "main.h"
// END LIBRARIES

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
const float maxVoltagePkPk = 5;
const int ADCBitResolution = 12;
const int MaxADCValue = (pow(2,ADCBitResolution) - 1); // Used for clipping detection
const int MinADCValue = 0;                             // Used for clipping detection
const long SAMPLE_PERIOD_MICROSECONDS = 119; // (8400 Hz)
const double SAMPLE_FREQUENCY = 8400;


int samples[SAMPLES_TAKEN];

// FFT and DAC Parameters
arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
double vReal[SAMPLES_TAKEN];
double vImag[SAMPLES_TAKEN];
double voltageValue = 0;
double stepSize = maxVoltagePkPk / (pow(2,ADCBitResolution) - 1);
bool signalClipped = false;
double userPlayedFreq = 0;
double fundamentalFreq = 0;
const double FFT_MAIN_MULTIPLIER = 0.764375;

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
  // Private Variables
  const int serialBaudRate = 9600;

  // Configure Pins
  pinMode(modeSelectButton.pin, modeSelectButton.mode);

  for(int i = 0; i < frequencyShiftButtonsLength; i++){
    pinMode(frequencyShiftButtons[i].pin, frequencyShiftButtons[i].mode);
  }
  
  pinMode(LED_TX, OUTPUT);
  pinMode(LED_RX, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  

  // Configure serial port for testing
  Serial.begin(serialBaudRate);

  // Set ADC to appropriate bit resolution
  analogReadResolution(ADCBitResolution); 
}

void loop() {
  
  // Read serial - FOR TESTING PURPOSES
  char TESTING_receivedChar = receiveCharFromSerial();
  
  debounceButtonHandler(modeSelectButton.pin, &modeSelectButton.pinState, &modeSelectButton.pinLastState, &modeSelectButton.debounceTimeMilliseconds, modeSelectButton.buttonHandler);
  
  if(modeSelect == SENSOR_MODE){
    // Begin Sensor Mode Logic - indicated by LED_TX. Note, 0 indicates ON
    digitalWrite(LED_TX, 0);
    digitalWrite(LED_RX, 1);

    if(TESTING_receivedChar == 'y' || TESTING_receivedChar == 'Y'){
      Serial.println("TEST Sampling run engaged for SENSOR MODE!!");

      // Get samples in specified sampling period
      for(int i = 0; i < SAMPLES_TAKEN; i++){
        samples[i] = analogRead(PIN_A5);
        if(samples[i] == MaxADCValue || samples[i] == MinADCValue){
          signalClipped = true;
          // Blink System LED to indicate signal has been clipped
          blinkSystemLED(CLIPPED_LED_BLINK_TIMES, CLIPPED_LED_BLINK_DURATION_MS);
          break;
        }

        // Put voltage value in Real part array
        voltageValue = samples[i] * stepSize;


        vReal[i] = voltageValue;

        delayMicroseconds(SAMPLE_PERIOD_MICROSECONDS);
      }

      if(!signalClipped){
        // Not flatlining! Start FFT process
        
      
        // double peakFreq = computeFFT(SAMPLES_TAKEN, SAMPLE_FREQUENCY, vReal, vImag);
        // Serial.println("Peak Frequency:");
        // Serial.println(peakFreq, 6);

        // NOTE: Currently, this only works between 32 Hz - 3100 Hz on sine and square waves - NL
        userPlayedFreq = computeFFT(SAMPLES_TAKEN, SAMPLE_FREQUENCY, vReal, vImag) * FFT_MAIN_MULTIPLIER;
        fundamentalFreq = determineFundamentalFreq(userPlayedFreq);
        
        // Printing out data: - TODO: Get rid of below print statements!!!
        Serial.print("User played frequency: ");
        Serial.println(userPlayedFreq,4);

        
        Serial.print("Fundamental frequency: ");
        Serial.println(fundamentalFreq,4);

        Serial.print("Corresponding Note: ");
        Serial.println(getStringFromFundamentalFreq(fundamentalFreq));
  
      }

      // Reset Variables
      memset(vReal, 0, sizeof(vReal));
      memset(vImag,0, sizeof(vImag));
      signalClipped = false;

  }
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
 * Name: getStringFromFundamentalFreq
 * Description: Gets the string version of the fundamental frequency
 * Params:
 * - fundamentalFreq: The fundamental frequency as a float.
 * Returns: string version of the fundamental frequency or a blank string if the parameter doesn't match with a fundamental frequency
 */
char * getStringFromFundamentalFreq( float fundamentalFreq )
{
  // Octave 1
  if(fundamentalFreq == C1) return "C1";
  else if(fundamentalFreq == CSharp1) return "C#1";
  else if(fundamentalFreq == D1) return "D1";
  else if(fundamentalFreq == DSharp1) return "D#1";
  else if(fundamentalFreq == E1) return "E1";
  else if(fundamentalFreq == F1) return "F1";
  else if(fundamentalFreq == FSharp1) return "F#1";
  else if(fundamentalFreq == G1) return "G1";
  else if(fundamentalFreq == GSharp1) return "G#1";
  else if(fundamentalFreq == A1) return "A1";
  else if(fundamentalFreq == ASharp1) return "A#1";
  else if(fundamentalFreq == B1) return "B1";
  // Octave 2
  else if(fundamentalFreq == C2) return "C2";
  else if(fundamentalFreq == CSharp2) return "C#2";
  else if(fundamentalFreq == D2) return "D2";
  else if(fundamentalFreq == DSharp2) return "D#2";
  else if(fundamentalFreq == E2) return "E2";
  else if(fundamentalFreq == F2) return "F2";
  else if(fundamentalFreq == FSharp2) return "F#2";
  else if(fundamentalFreq == G2) return "G2";
  else if(fundamentalFreq == GSharp2) return "G#2";
  else if(fundamentalFreq == A2) return "A2";
  else if(fundamentalFreq == ASharp2) return "A#2";
  else if(fundamentalFreq == B2) return "B2";
  // Octave 3
  else if(fundamentalFreq == C3) return "C3";
  else if(fundamentalFreq == CSharp3) return "C#3";
  else if(fundamentalFreq == D3) return "D3";
  else if(fundamentalFreq == DSharp3) return "D#3";
  else if(fundamentalFreq == E3) return "E3";
  else if(fundamentalFreq == F3) return "F3";
  else if(fundamentalFreq == FSharp3) return "F#3";
  else if(fundamentalFreq == G3) return "G3";
  else if(fundamentalFreq == GSharp3) return "G#3";
  else if(fundamentalFreq == A3) return "A3";
  else if(fundamentalFreq == ASharp3) return "A#3";
  else if(fundamentalFreq == B3) return "B3";
  // Octave 4
  else if(fundamentalFreq == C4) return "C4";
  else if(fundamentalFreq == CSharp4) return "C#4";
  else if(fundamentalFreq == D4) return "D4";
  else if(fundamentalFreq == DSharp4) return "D#4";
  else if(fundamentalFreq == E4) return "E4";
  else if(fundamentalFreq == F4) return "F4";
  else if(fundamentalFreq == FSharp4) return "F#4";
  else if(fundamentalFreq == G4) return "G4";
  else if(fundamentalFreq == GSharp4) return "G#4";
  else if(fundamentalFreq == A4) return "A4";
  else if(fundamentalFreq == ASharp4) return "A#4";
  else if(fundamentalFreq == B4) return "B4";
  // Octave 5
  else if(fundamentalFreq == C5) return "C5";
  else if(fundamentalFreq == CSharp5) return "C#5";
  else if(fundamentalFreq == D5) return "D5";
  else if(fundamentalFreq == DSharp5) return "D#5";
  else if(fundamentalFreq == E5) return "E5";
  else if(fundamentalFreq == F5) return "F5";
  else if(fundamentalFreq == FSharp5) return "F#5";
  else if(fundamentalFreq == G5) return "G5";
  else if(fundamentalFreq == GSharp5) return "G#5";
  else if(fundamentalFreq == A5) return "A5";
  else if(fundamentalFreq == ASharp5) return "A#5";
  else if(fundamentalFreq == B5) return "B5";
  // Octave 6
  else if(fundamentalFreq == C6) return "C6";
  else if(fundamentalFreq == CSharp6) return "C#6";
  else if(fundamentalFreq == D6) return "D6";
  else if(fundamentalFreq == DSharp6) return "D#6";
  else if(fundamentalFreq == E6) return "E6";
  else if(fundamentalFreq == F6) return "F6";
  else if(fundamentalFreq == FSharp6) return "F#6";
  else if(fundamentalFreq == G6) return "G6";
  else if(fundamentalFreq == GSharp6) return "G#6";
  else if(fundamentalFreq == A6) return "A6";
  else if(fundamentalFreq == ASharp6) return "A#6";
  else if(fundamentalFreq == B6) return "B6";
  // Octave 7
  else if(fundamentalFreq == C7) return "C7";
  else if(fundamentalFreq == CSharp7) return "C#7";
  else if(fundamentalFreq == D7) return "D7";
  else if(fundamentalFreq == DSharp7) return "D#7";
  else if(fundamentalFreq == E7) return "E7";
  else if(fundamentalFreq == F7) return "F7";
  else if(fundamentalFreq == FSharp7) return "F#7";
  else if(fundamentalFreq == G7) return "G7";
  else if(fundamentalFreq == GSharp7) return "G#7";
  else if(fundamentalFreq == A7) return "A7";
  else if(fundamentalFreq == ASharp7) return "A#7";
  else if(fundamentalFreq == B7) return "B7";
  // Octave 8
  else if(fundamentalFreq == C7) return "C8";
  else return "";
}


/**
 * Name: determineFundamentalFreq
 * Description: Determines the closest fundamental frequency (located in fundamentalFrequencies[]) based on the user frequency
 * Params:
 * - userFreq: The user frequency as a float.
 * Returns: one of the fundamental frequencies located in fundamentalFrequencies[] that most closely corresponds to the user frequency
 */
float determineFundamentalFreq(float userFreq){
  float currentDifference = 0.0f;
  float fundamentalFreq = fundamentalFrequencies[0];
  float minDifference = fabs(userFreq - fundamentalFrequencies[0]);

  for(int i = 1; i < fundamentalFrequenciesArrayLength; i++){
    currentDifference = fabs(userFreq - fundamentalFrequencies[i]);
    if(currentDifference > minDifference) break;
    minDifference = currentDifference;
    fundamentalFreq = fundamentalFrequencies[i];
  }

  return fundamentalFreq;
}

/**
 * Name: computeFFT
 * Description: Computes the FFT on the vReal data and returns the peak frequency
 * Params:
 * - samples: Samples. Must be a power of 2
 * - sampleFrequency: Sample frequency. Must be greater than 0
 * - vReal: Pointer to double array that functions as input and output. It is assumed that the length of the array is equal to
 *  the samples 
 *    - input: Contains samples that are the voltage values
 *    - output: Stores the complex magnitudes from the FFT computation. Each index corresponds to the bin width.
 * - vImag: Imag array that functions as an input and output. It is assumed that the length of the array is equal to
 *  the samples 
 *    - input: Contains NULL data for each element
 *    - output: Contains the imaginary computed FFT values
 * 
 * Returns: double: the peak frequency from the *vReal array or -1 if the samples are not a power of 2 or if the sample frequency is <= 0
 */
double computeFFT(int samples, int sampleFrequency, double *vReal, double *vImag){
        // Error checking:
        double difference = log2(samples) - ((int)log2(samples));
        if(sampleFrequency <= 0 || difference != 0.0) return -1;

        //Serial.println("Voltage Values:");
        //PrintVector(vReal, SAMPLES_TAKEN, SCL_TIME, SAMPLES_TAKEN, SAMPLE_FREQUENCY);
        unsigned long prevTime = millis();
     
        // Weigh the Data:
        FFT.Windowing(vReal, SAMPLES_TAKEN, FFT_WIN_TYP_HAMMING, FFT_FORWARD);	/* Weigh data */
    
        // Compute FFT:
        FFT.Compute(vReal, vImag, SAMPLES_TAKEN, FFT_FORWARD); /* Compute FFT */
        // Serial.println("Computed Real values:");
        // PrintVector(vReal, SAMPLES_TAKEN, SCL_INDEX, SAMPLES_TAKEN, SAMPLE_FREQUENCY);
        // Serial.println("Computed Imaginary values:");
        // PrintVector(vImag, SAMPLES_TAKEN, SCL_INDEX, SAMPLES_TAKEN, SAMPLE_FREQUENCY);

        // Compute Magnitudes:
        Serial.println("Computed magnitudes:");
        FFT.ComplexToMagnitude(vReal, vImag, SAMPLES_TAKEN);
        // Since it is mirrored!!!
        //PrintVector(vReal, (SAMPLES_TAKEN >> 1), SCL_FREQUENCY, SAMPLES_TAKEN, SAMPLE_FREQUENCY); 
        
        // unsigned long timeDiff = (millis() - prevTime);
        // Serial.print("FFT Computation Time: ");

        // Serial.print(timeDiff);
        // Serial.println(" ms");

        return FFT.MajorPeak(vReal, SAMPLES_TAKEN, SAMPLE_FREQUENCY);
}

/**
 * Name: blinkSystemLED
 * Description: Function to blink PIN_LED a certain number of times with each blink being a 
 * specified duration
 * Params:
 * - blinkTimes: The number of times to blink the LED. Must be greater than 0, otherwise this function just returns.
 * - blinkDuration_ms: The duration in milliseconds for each blink. Must be greater than 0, otherwise this function just returns.
 * Returns: None
 */
void blinkSystemLED(int blinkTimes, int blinkDuration_ms){
  
  if(blinkTimes <= 0 || blinkDuration_ms <= 0) return;

  int halfBlinkDuration_ms = blinkDuration_ms / 2;

  for(int j = 0; j < blinkTimes; j++){
    digitalWrite(PIN_LED, 1); // ON
    delay(halfBlinkDuration_ms);
    digitalWrite(PIN_LED, 0);// OFF
    delay(halfBlinkDuration_ms);
  }
}


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
 * - samples: Samples. Must be a power of 2
 * - sampleFrequency: Sample frequency. Must be greater than 0
 * 
 * Return: int
 * -1 if error
 *  0 if success 
 */
int PrintVector(double *vData, uint16_t elements, uint8_t scaleType, int samples, int sampleFrequency)
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
        abscissa = ((i * 1.0) / sampleFrequency);
        unit = " seconds: ";
	break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * sampleFrequency) / samples);
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