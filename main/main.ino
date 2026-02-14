/**
 * File: main.ino
 * Description: Capstone Musical Tuner Code
 */

// BEGIN DEFINES
#define SAMPLES_TAKEN 500
#define DIFFERENCE_ERROR 20
// END DEFINES

// BEGIN PRIVATE VARIABLES
const int serialBaudRate = 115200;
const int ADCPin = A5;
const float maxVoltagePkPk = 5;
const int ADCBitResolution = 14;
const long SAMPLE_PERIOD_MS = 4; // Milliseconds (250 Hz)
const pin_size_t BUTTON_INPUT_PIN = 13;

PinStatus pinStatus = LOW;
long prevTime = 0;
long currentTime = 0;
int ADCDigitalValue = 0;
float voltageValue = 0;
float stepSize = maxVoltagePkPk / (pow(2,ADCBitResolution) - 1);


int samples[SAMPLES_TAKEN];
int currentSample = 0;
int sumOfSamples = 0; // Used for average
 // END PRIVATE VARIABLES



void setup() {
  pinMode(BUTTON_INPUT_PIN, INPUT_PULLDOWN);          // sets the digital pin 13 as input
  Serial.begin(serialBaudRate);
  analogReadResolution(ADCBitResolution); // Set ADC to appropriate bit resolution
}

void loop() {
  // Read button
  pinStatus = digitalRead(BUTTON_INPUT_PIN);

  if(pinStatus == HIGH){
    // Let user know button as been pushed
    Serial.println("BUTTON PUSHED!!!");
    Serial.println("Loading in 3 seconds");
    delay(1000);
    Serial.println("Loading in 2 seconds");
    delay(1000);
    Serial.println("Loading in 1 second");
    delay(1000);
    Serial.println("Start sampling!!!");

  // Get 500 samples in specified sampling period
  for(int i = 0; i < SAMPLES_TAKEN; i++){
    samples[i] = analogRead(ADCPin);
    Serial.println("Sample " + String(i) + ": " + String(samples[i]));
    sumOfSamples += samples[currentSample];
    delay(SAMPLE_PERIOD_MS);
  }
  Serial.println("Sum of Samples: " + String(sumOfSamples));
   int average = sumOfSamples / SAMPLES_TAKEN;
    int difference = abs(samples[0] - average);
    // Check for flatlining
    if(difference < DIFFERENCE_ERROR){
      // Not flatlining! Start FFT process
      // TODO
      Serial.println("NOT Flatlined");
      
    }
    Serial.println("Flatlined");

    // Clear
    sumOfSamples = 0;
  } 
  else{
    Serial.println("BUTTON NOT PUSHED!!!");
  }

  

  
}
