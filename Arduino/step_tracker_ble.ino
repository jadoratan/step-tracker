/*
  I tested this on the BLE Rev Sense2, the connection works and all you need to do is update the detectStep method!
  Since you guys are using RP2040 Connect, I think that all that needed to change was the accelerometer so i hope that works
*/

#include <ArduinoBLE.h>
#include <Arduino_LSM6DSOX.h>   // Nano RP2040 Connect accelerometer
#include <nano-ble-wearable_inferencing.h>
#include <cmath>


// =====================================================================
//  BLE (what we already did, though your implementation might have been slightly different)
// =====================================================================

// Service and Characteristic
BLEService stepService("39B10010-E8F2-537E-4F6C-D104768A1214"); // PLEASE POST WHAT NAME YOU END UP USING SO WE DON'T OVERLAP!! we can all end up just changing the first digit or something
BLEUnsignedIntCharacteristic stepChar("39B10011-E8F2-537E-4F6C-D104768A1214", BLERead | BLENotify);

unsigned int steps = 0; // bc of the way i structured this i'm leaving this at the top but it's not necessarily essential to all the ble setup beyond publishing
bool wasConnected = false; 

bool notRest = true;

void setupBLE() {
  if (!BLE.begin()) {
    Serial.println("starting BluetoothÂ® Low Energy module failed!");
    while (1);
  }

  BLE.setLocalName("Step Tracker");
  BLE.setAdvertisedService(stepService);

  stepService.addCharacteristic(stepChar);
  BLE.addService(stepService);

  stepChar.writeValue(0);

  BLE.advertise();
  Serial.println("BLE Step Tracker Peripheral");
}

// called while waiting between samples, handles connect/disconnect w/o blocking
void updateBLE() {
  BLEDevice central = BLE.central();
  bool connected = central && central.connected();

  if (connected && !wasConnected) {
    Serial.print("Connected to central: ");
    Serial.println(central.address());
    stepChar.writeValue(steps);   // send the current total right away
  } else if (!connected && wasConnected) {
    Serial.println("Disconnected from central");
  }

  wasConnected = connected;
}

void publishSteps() { // not rlly necessary i just thought it'd make easier to read
  stepChar.writeValue(steps);
}


unsigned int pendingSteps = 0;

// =====================================================================
//  Model logic, see https://docs.edgeimpulse.com/hardware/deployments/run-cpp-overview and https://docs.edgeimpulse.com/tools/clis/edge-impulse-cli/data-forwarder for context
// =====================================================================

static const char* WALKING_LABEL = "walk";   // must match edge impulse label exactly, right now this is designed for two labels because the docs designed their example for two but in the future we can have more!
static const float CONFIDENCE_THRESHOLD = 0.70f;

static unsigned long time_between_samples_us = (unsigned long)(EI_CLASSIFIER_INTERVAL_MS * 1000.0f);

// to classify 1 frame of data you need EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE values
static float features[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE];

// returns true if the window was classified as walking, just put this in a function while the OG didn't because we're using it as a bool
bool classifyWindow() {
  ei_impulse_result_t result = { 0 };

  // create signal from features frame
  signal_t signal;
  numpy::signal_from_buffer(features, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);

  // run classifier
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
  if (res != EI_IMPULSE_OK) {
    Serial.print("run_classifier failed: ");
    Serial.println(res);
    return false;
  }

  // print the predictions
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    Serial.print(result.classification[ix].label);
    Serial.print(": ");
    Serial.print(result.classification[ix].value, 2);
    Serial.print("  ");
  }

  // find the top label
  size_t best = 0;
  for (size_t ix = 1; ix < EI_CLASSIFIER_LABEL_COUNT; ix++) {
    if (result.classification[ix].value > result.classification[best].value) best = ix;
  }

  return strcmp(result.classification[best].label, WALKING_LABEL) == 0 &&
         result.classification[best].value >= CONFIDENCE_THRESHOLD; // is the top label walking? (if not it's idle)
}

// =====================================================================
//  Step detection (fill in the function with how you want your "steps" to be processed)
// =====================================================================

static const float STEP_HIGH = 1.00f;   // g, peak must exceed this to count
static const float STEP_LOW  = 0.98f;   // g, must drop below this to re-arm
static const unsigned long MIN_STEP_INTERVAL_MS = 250;  // ~4 steps/s max

bool armed = true;
unsigned long lastStepMs = 0;

void detectStep(float x, float y, float z) {
  float mag = sqrtf(x*x + y*y + z*z);
  unsigned long now = millis();
  if (armed && mag > STEP_HIGH && (now - lastStepMs) >= MIN_STEP_INTERVAL_MS) {
    pendingSteps++;
    lastStepMs = now;
    armed = false;               // wait for the peak to end
  } else if (!armed && mag < STEP_LOW) {
    armed = true;                // signal came back down; ready for next step
  }
}

// =====================================================================
//  Main
// =====================================================================

void setup() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && millis() - t0 < 3000);   // wait for Serial, but still start on battery

  if (!IMU.begin()) {
    Serial.println("Failed to initialize IMU!");
    while (1);
  }

  setupBLE();
}

void loop() { // I had to change some of these lines from the original example, but think of all the logic as the same
  float x, y, z;

  // fill the features array
  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME) {
    unsigned long next_tick = micros() + time_between_samples_us;
    IMU.readAcceleration(x, y, z);   // g

    // copy accelerometer data into the features array
    features[ix + 0] = x;
    features[ix + 1] = y;
    features[ix + 2] = z;

    detectStep(x, y, z);

    while ((long)(micros() - next_tick) < 0) {
      updateBLE(); 
    }
  }

  // frame full? then classify
  bool walk = classifyWindow();
  if (walk && pendingSteps > 0) {
    steps += pendingSteps;
    publishSteps();
  }
  pendingSteps = 0;   // "steps found" while idle are discarded

  Serial.print(" steps: ");
  Serial.println(steps);
}