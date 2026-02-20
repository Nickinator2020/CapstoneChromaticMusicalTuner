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
// END DEFINES

// BEGIN PRIVATE VARIABLES
const int serialBaudRate = 9600;
const int ADCPin = A5;
const float maxVoltagePkPk = 5;
const int ADCBitResolution = 14;
const long SAMPLE_PERIOD_MICROSECONDS = 119; // Microseconds (8400 Hz)

arduinoFFT FFT = arduinoFFT(); /* Create FFT object */
const double SAMPLE_FREQUENCY = 8400;

double voltageValue = 0;
double stepSize = maxVoltagePkPk / (pow(2,ADCBitResolution) - 1);

int samples[SAMPLES_TAKEN];
int sumOfSamples = 0; // Used for average

char receivedChar = '\0';

// For FFT
double vReal[SAMPLES_TAKEN];
double vImag[SAMPLES_TAKEN];
 // END PRIVATE VARIABLES



void setup() {

  Serial.begin(serialBaudRate);
  analogReadResolution(ADCBitResolution); // Set ADC to appropriate bit resolution
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