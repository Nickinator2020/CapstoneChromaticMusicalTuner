/**
 * File: main.ino
 * Description: Capstone Musical Tuner Code!!!!
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
const long samplePeriod = 119; // microseconds

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
  Serial.begin(serialBaudRate);
  analogReadResolution(ADCBitResolution); // Set ADC to appropriate bit resolution
}

void loop() {
  currentTime = micros();

  // Sample at sample period
  if(currentTime - prevTime >= samplePeriod){ 
    samples[currentSample] = analogRead(ADCPin);
    sumOfSamples += samples[currentSample];
    
    currentSample++;
    prevTime = currentTime;
  }


  // Check if surpassed given samples
  if(currentSample >= SAMPLES_TAKEN){
    
    // BEGIN GET RID OF CODE FOR SQUARE WAVE TEST (JUST KEEP FFT STUFF)
    int average = sumOfSamples / SAMPLES_TAKEN;
    int difference = abs(samples[0] - average);
    // Check for flatlining
    if(difference < DIFFERENCE_ERROR){
      // Not flatlining! Start FFT process
      // TODO
      
    }
   // END  GET RID OF CODE FOR SQUARE WAVE TEST 

    // Reset everything
    currentSample = 0; 
    sumOfSamples = 0;
    memset(samples, 0, sizeof(samples));
    prevTime = micros();
  }

  
  Serial.println("Test!");
  
}
