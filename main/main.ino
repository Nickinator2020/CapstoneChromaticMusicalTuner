/**
 * File: main.ino
 * Description: Capstone Musical Tuner Code
 */

// BEGIN DEFINES
#define SAMPLES_TAKEN 512
#define DIFFERENCE_ERROR 20 // Threshold
// END DEFINES

// BEGIN PRIVATE VARIABLES
const int serialBaudRate = 9600;
const int ADCPin = A5;
const float maxVoltagePkPk = 5;
const int ADCBitResolution = 14;
const long SAMPLE_PERIOD_MICROSECONDS = 119; // Microseconds (8400 Hz)


long prevTime = 0;
long currentTime = 0;
int ADCDigitalValue = 0;
float voltageValue = 0;
float stepSize = maxVoltagePkPk / (pow(2,ADCBitResolution) - 1);


int samples[SAMPLES_TAKEN];
int sumOfSamples = 0; // Used for average

char receivedChar = '\0';
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
      Serial.println("Sample " + String(i) + ": " + String(samples[i]));
      sumOfSamples += samples[i];

      delayMicroseconds(SAMPLE_PERIOD_MICROSECONDS);
    }

    Serial.println("Sum of Samples: " + String(sumOfSamples));
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