// Librararies
#include <Adafruit_NeoPixel.h>
#include <BlynkSimpleEsp32.h>  // Part of Blynk by Volodymyr Shymanskyy
#include <WiFi.h>              // Part of WiFi Built-In by Arduino
#include <WiFiClient.h>        // Part of WiFi Built-In by Arduino
#include <math.h>
#include <stdbool.h>
#include <time.h>

#include "FreeRTOS.h"  // Threading library of FreeRTOS Kernel

// Config
#include "Configuration/Wifi.h"

// GPIO Pins
const unsigned short int nozzleRecoilSensorReadPin = 34;  // Read: 1 = infrared barrier free, 0 = IR interrupted
const unsigned short int triggerPullReadPin = 32;         // Read: 1 = Shoot, 0 = Stop
const unsigned short int triggerTouchReadPin = 4;
const unsigned short int motorControlPin = 33;  // HIGH = shootAction
const unsigned short int neoPixelPin = 23;

// PWM channels and settings
const int freq = 5000;
const int shotChannel = 0;  // Shot Sensor
const int resolution = 8;   // 8 Bits resulting in 0-255 as Range for Duty Cycle

// Configurations
const unsigned short int burstShootCount = 3;     // Defines how many BB's to shootAction in burst mode with one trigger pull
const unsigned int touchDetectionThreshold = 20;  // Adjustment of touch pin sensitivity
const int triggerDebounceDelay = 32;

// Thread Adjustments
const short int triggerTouchSensorThreadIterationDelay = 64;
const short int triggerPullSensorThreadIterationDelay = 32;

const short int shootActionRoutineThreadIterationDelay = 5;

TaskHandle_t triggerTouchSensorThreadHandle;
TaskHandle_t triggerPullSensorThreadHandle;
TaskHandle_t shotCountDetectSensorThreadHandle;

TaskHandle_t shootActionRoutineThreadHandle;  // TODO: Physical device test

TaskHandle_t sightLightHandlingRoutineThreadHandle;
TaskHandle_t setSystemSleepStateRoutineThreadHandle;  // TODO: implement sleep routine with reset function on interaction

// Global vars written by sensors
bool triggerTouching;
unsigned short triggerTouchValue;

bool triggerPulling;
unsigned long long shotsFired;

char* shootMode = "semi";

Adafruit_NeoPixel lights = Adafruit_NeoPixel(4, neoPixelPin, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);

  ledcSetup(shotChannel, freq, resolution);
  ledcAttachPin(nozzleRecoilSensorReadPin, shotChannel);
  pinMode(nozzleRecoilSensorReadPin, INPUT_PULLDOWN);
  pinMode(triggerPullReadPin, INPUT_PULLDOWN);
  pinMode(motorControlPin, OUTPUT);

  disableCore0WDT();  // Disable WatchDogTimeout, so threads can run as long as they want
  disableCore1WDT();  // Same

  xTaskCreatePinnedToCore(triggerTouchSensor, "triggerTouchSensor", 10000, NULL, 20, &triggerTouchSensorThreadHandle, 1);
  xTaskCreatePinnedToCore(triggerPullSensor, "triggerPullSensor", 10000, NULL, 20, &triggerPullSensorThreadHandle, 1);
  xTaskCreatePinnedToCore(nozzleRecoilSensor, "nozzleRecoilSensor", 10000, NULL, 20, &shotCountDetectSensorThreadHandle, 1);

  xTaskCreatePinnedToCore(shootActionRoutine, "shootActionRoutine", 10000, NULL, 20, &shootActionRoutineThreadHandle, 0);

  xTaskCreatePinnedToCore(sightLightHandlingRoutine, "sightLIghtHandler", 10000, NULL, 10, &sightLightHandlingRoutineThreadHandle, 0);
}

void loop() {}

// ----------------------------------------------------------------------------
// Sensor Threads
// ----------------------------------------------------------------------------

void triggerTouchSensor(void* param) {
  // Flags to execute code only on condition transition
  bool setEnabledFlag = false;
  bool setDisabledFlag = false;
  unsigned short int touchValue;

  while (true) {
    touchValue = touchRead(triggerTouchReadPin);
    if (touchValue <= touchDetectionThreshold && setEnabledFlag == false) {
      triggerTouching = true;
      triggerTouchValue = touchValue;
      setEnabledFlag = true;
      setDisabledFlag = false;
      Serial.printf("Touching: %s   ,   value: %d\n", triggerTouching ? "true" : "false", touchValue);
    } else if (touchValue > touchDetectionThreshold && setDisabledFlag == false) {
      triggerTouching = false;
      triggerTouchValue = touchValue;
      setEnabledFlag = false;
      setDisabledFlag = true;
      Serial.printf("Touching: %s   ,   value: %d\n", triggerTouching ? "true" : "false", touchValue);
    }
    delay(triggerTouchSensorThreadIterationDelay);
  }
}

void triggerPullSensor(void* param) {
  unsigned long long lastDebounceTime;  // the last time the output pin was toggled
  int buttonState;                      // the current reading from the input pin
  int lastButtonState = LOW;            // the previous reading from the input pin
  int reading;

  while (true) {
    reading = digitalRead(triggerPullReadPin);
    if (reading != lastButtonState) {
      lastDebounceTime = millis();
    }
    if ((millis() - lastDebounceTime) > triggerDebounceDelay) {
      if (reading != buttonState) {
        buttonState = reading;
        if (buttonState == HIGH) {
          triggerPulling = true;
          Serial.printf("Pulling: %s\n", triggerPulling ? "true" : "false");
        } else {
          triggerPulling = false;
          Serial.printf("Pulling: %s\n", triggerPulling ? "true" : "false");
        }
      }
    }
    lastButtonState = reading;
    delay(triggerPullSensorThreadIterationDelay);
  }
}

void nozzleRecoilSensor(void* param) {
  bool barrierIsFree;
  while (true) {
    barrierIsFree = digitalRead(nozzleRecoilSensorReadPin);
    if (barrierIsFree == false) {
      shotsFired++;
      Serial.printf("Shot fired.\n");
      while (digitalRead(nozzleRecoilSensorReadPin) == false) {
        delay(1);  // wait until piston leaves the barrier
      }
    }
    delay(1);  // to save cpu cycles, nozzle does not move that fast
  }
}

// ----------------------------------------------------------------------------
// Action Threads
// ----------------------------------------------------------------------------

void shootActionRoutine(void* param) {
  while (true) {
    if (triggerTouching && triggerPulling) {
      shoot(shootMode);
    }
    while (triggerTouching && triggerPulling) {
      delay(10);  // sleep until trigger is released
    }
    delay(shootActionRoutineThreadIterationDelay);  // the higher the longer it takes to respond on pull (ping), but saves cpu time
  }
}

void sightLightHandlingRoutine(void* param) {
  lights.begin();
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.setPixelColor(1, pixels.Color(0, 255, 0));
  pixels.setPixelColor(2, pixels.Color(, 0, 255));
  pixels.setPixelColor(3, pixels.Color(255, 255, 255));
  pixels.show();
}

// ----------------------------------------------------------------------------
// Functions:
// ----------------------------------------------------------------------------

void shoot(char* shootMode = "semi") {
  if (shootMode == "semi") {
    shootGivenTimes(1);
  } else if (shootMode == "burst") {
    shootGivenTimes(burstShootCount);
  } else if (shootMode == "full") {  // shoots until either trigger is released or finger stops touching trigger
    shootWhilePulled();
  }
}

void shootGivenTimes(ushort times = 1) {
  ushort shotsFiredBefore = shotsFired;
  digitalWrite(motorControlPin, HIGH);
  while (shotsFired <= shotsFiredBefore + times) {
    delay(10);
  }
  digitalWrite(motorControlPin, LOW);
}

void shootWhilePulled() {
  digitalWrite(motorControlPin, HIGH);
  while (triggerTouching && triggerPulling) {  // exit condition:  !triggerTouching || !triggerPulling
    delay(1)                                   // stops immediately, when releasing the trigger
  };
  digitalWrite(motorControlPin, LOW);
}