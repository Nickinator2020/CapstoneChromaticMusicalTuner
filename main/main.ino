/**
 * File: main.ino
 * Description: Capstone Musical Tuner Code
 */

// BEGIN LIBRARIES
#include "arduinoFFT.h"
// END LIBRARIES

// BEGIN DEFINES
#define SAMPLES_TAKEN 512
#define DIFFERENCE_ERROR 20 // Threshold

// For FFT
#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02

// FUNDAMENTAL FREQUENCIES
// Octave 1
#define C1 32.70
#define CSharp1 34.65
#define D1 36.71
#define DSharp1 38.89
#define E1 41.20
#define F1 43.65
#define FSharp1 46.25
#define G1 49.00
#define GSharp1 51.91
#define A1 55.00
#define ASharp1 58.27
#define B1 61.74
// Octave 2 TODO

// Octave 3 TODO

// Octave 4 TODO
#define A4 440.00
// Octave 5 TODO

// Octave 6 TODO

// Octave 7 TODO

// Octave 8 TODO

// END DEFINES

// BEGIN ENUMS

typedef enum {
  SPEAKER_MODE = 0,
  SENSOR_MODE = 1,
} ModeSelect;

typedef enum {
  NONE = 0,
  UP_HALFSTEP_PIN = 1,
  DOWN_HALFSTEP_PIN = 2,
  UP_OCTAVE_PIN = 3,
  DOWN_OCTAVE_PIN = 4
} NoteButtonPinNumber;
// END ENUMS

// BEGIN PRIVATE VARIABLES
// I/O Parameters
const int modeSelectPin = 0;
const int upHalfStepPin = UP_HALFSTEP_PIN;
const int downHalfStepPin = DOWN_HALFSTEP_PIN;
const int upOctavePin = UP_OCTAVE_PIN;
const int downOctavePin = DOWN_OCTAVE_PIN;

bool modeSelect = SENSOR_MODE;

// ADC Parameters
const int serialBaudRate = 9600;
const int ADCPin = A5;
const float maxVoltagePkPk = 5;
const int ADCBitResolution = 12;
const long SAMPLE_PERIOD_MICROSECONDS = 119; // Microseconds (8400 Hz)
const double SAMPLE_FREQUENCY = 8400;


int samples[SAMPLES_TAKEN];
int sumOfSamples = 0; // Used for average

char receivedChar = '\0';

// For FFT
arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
double vReal[SAMPLES_TAKEN];
double vImag[SAMPLES_TAKEN];
double voltageValue = 0;
double stepSize = maxVoltagePkPk / (pow(2,ADCBitResolution) - 1);
 
// For DAC
float fundamentalFrequencies[] = {C1, CSharp1, }; //TODO
float DACFreqCurrentIndex = 0;
// END PRIVATE VARIABLES

// BEGIN setup() and loop()

void setup() {
  // Configure Pins
  pinMode(modeSelectPin, INPUT_PULLUP);
  pinMode(upHalfStepPin, INPUT_PULLUP);
  pinMode(downHalfStepPin, INPUT_PULLUP);
  pinMode(upOctavePin, INPUT_PULLUP);
  pinMode(downOctavePin, INPUT_PULLUP);

  // Attach interrupts to pins
  attachInterrupt(digitalPinToInterrupt(modeSelectPin), modeConfigurationISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(upHalfStepPin), upHalfStepISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(downHalfStepPin), downHalfStepISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(upOctavePin), upOctaveISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(downOctavePin), downOctaveISR, FALLING);


  // Configure serial port for testing
  Serial.begin(serialBaudRate);

  // Set ADC to appropriate bit resolution
  analogReadResolution(ADCBitResolution); 
  Serial.println("Starting loop!");
}

void loop() {
  // Read serial - FOR TESTING PURPOSES
  receivedChar = receiveCharFromSerial();

  if(receivedChar == 'y' || receivedChar == 'Y'){
    Serial.println("Sampling run engaged!!");

    // Get samples in specified sampling period
   for(int i = 0; i < SAMPLES_TAKEN; i++){
      samples[i] = analogRead(ADCPin);
      //Serial.println("Sample " + String(i) + ": " + String(samples[i]));
      sumOfSamples += samples[i];

      // Put voltage value in Real part array
      voltageValue = samples[i] * stepSize;
      vReal[i] = voltageValue;

      delayMicroseconds(SAMPLE_PERIOD_MICROSECONDS);
    }

    //Serial.println("Sum of Samples: " + String(sumOfSamples));
    int average = sumOfSamples / SAMPLES_TAKEN;
    Serial.println("Average: " + String(average));
    int difference = abs(samples[0] - average);
    Serial.println("Difference: " + String(difference));

    // Check for flatlining
    if(difference < DIFFERENCE_ERROR){
      Serial.println("Flatlined");
      
      
    }
    else{
      Serial.println("NOT Flatlined");
      // Not flatlining! Start FFT process
      // TODO
      
      // Starting values before FFT
      Serial.println("Voltage Values:");
      PrintVector(vReal, SAMPLES_TAKEN, SCL_TIME);
      
      // Weigh the Data:
      FFT.Windowing(vReal, SAMPLES_TAKEN, FFT_WIN_TYP_HAMMING, FFT_FORWARD);	/* Weigh data */
      Serial.println("Weighed data:");
      PrintVector(vReal, SAMPLES_TAKEN, SCL_TIME);

      // Compute FFT:
      FFT.Compute(vReal, vImag, SAMPLES_TAKEN, FFT_FORWARD); /* Compute FFT */
      Serial.println("Computed Real values:");
      PrintVector(vReal, SAMPLES_TAKEN, SCL_INDEX);
      Serial.println("Computed Imaginary values:");
      PrintVector(vImag, SAMPLES_TAKEN, SCL_INDEX);

      // Compute Magnitudes:
      Serial.println("Computed magnitudes:");
      FFT.ComplexToMagnitude(vReal, vImag, SAMPLES_TAKEN);
      // Since it is mirrored!!!
      PrintVector(vReal, (SAMPLES_TAKEN >> 1), SCL_FREQUENCY); 

      double x = FFT.MajorPeak(vReal, SAMPLES_TAKEN, SAMPLE_FREQUENCY);
      Serial.println("Peak Magnitude (Frequency Value I think!!!):");
      Serial.println(x, 6);
    }
    
    


    
    // Clear
    sumOfSamples = 0;
    memset(vReal, 0, sizeof(vReal));
    memset(vImag,0, sizeof(vImag));

  }
  
  Serial.println(digitalRead(modeSelectPin));
  delay(1000);
}

// END setup() and loop()

// BEGIN ISRs
void modeConfigurationISR(){
  modeSelect = !modeSelect;
 

}

void upHalfStepISR(){
  if(modeSelect == SPEAKER_MODE){
    //TODO Change DACFreqCurrentIndex
  }
}

void downHalfStepISR(){
  if(modeSelect == SPEAKER_MODE){
    //TODO Change DACFreqCurrentIndex
  }
}

void upOctaveISR(){
  if(modeSelect == SPEAKER_MODE){
    //TODO Change DACFreqCurrentIndex
  }
}

void downOctaveISR(){
  if(modeSelect == SPEAKER_MODE){
    //TODO Change DACFreqCurrentIndex
  }
}
// END ISRs

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