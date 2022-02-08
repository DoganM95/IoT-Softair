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

const short int threadShootOnTouchAndTriggerRoutineSleepDuration = 100;

TaskHandle_t triggerTouchSensorThreadHandle;
TaskHandle_t triggerPullSensorThreadHandle;
TaskHandle_t shootOnTouchAndTriggerRoutineThreadHandle;
TaskHandle_t shotCountDetectSensorThreadHandle;
TaskHandle_t setSystemSleepStateRoutineThreadHandle;

// Global vars written by sensors
bool triggerTouching;
unsigned short triggerTouchValue;

bool triggerPulling;
unsigned long long shotsFired;

char* fireMode = "semi-automatic";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(4, neoPixelPin, NEO_GRB + NEO_KHZ800);

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

  // xTaskCreatePinnedToCore(shootActionRoutine, "shootActionRoutine", 10000, NULL, 20, &shootActionRoutineThreadHandle, 1);
}

void loop() {}

// void shootAction(bool state, char* shootMode = "") {
//   unsigned int duration = 0;
//   unsigned int shots = 0;

//   if (state) {
//     if (shootMode == "semi-automatic") {
//       digitalWrite(motorControlPin, HIGH);
//       while (digitalRead(nozzleRecoilSensorReadPin) == 1) {  // while infrared barrier is not blocked, wait
//         delay(1);                                      // FIXME: needs to be replaced by a time check
//         duration++;
//         if (duration >= 3000) {
//           goto skipCounterCausedByBarrierError;
//           break;  // security mechanism to stop shooting after x ms (in case of sensor failure)}
//         }
//       }

//       shotsFired++;
//     skipCounterCausedByBarrierError:
//       digitalWrite(motorControlPin, LOW);  // on infrared barrier blocked, cut motor electricity
//     }

//     else if (shootMode == "burst") {
//       digitalWrite(motorControlPin, HIGH);
//       while (digitalRead(nozzleRecoilSensorReadPin) == 1) {
//         delay(1);
//       }
//     }

//     else if (shootMode == "full-automatic") {  // shoots until either trigger is released or finger stopped touching trigger
//       while (digitalRead(triggerPullReadPin) == HIGH && touchRead(4) <= touchDetectionThreshold) {
//       continueShooting:
//         digitalWrite(motorControlPin, HIGH);
//       }
//       delay(triggerDebounceDelay / 4);
//       if (digitalRead(triggerPullReadPin) == LOW || touchRead(4) >= touchDetectionThreshold) {
//         digitalWrite(motorControlPin, LOW);
//       } else {
//         goto continueShooting;
//       }
//     }

//   } else {
//     digitalWrite(motorControlPin, LOW);
//   }
// }

// ----------------------------------------------------------------------------
// Sensor Threads
// ----------------------------------------------------------------------------

void triggerTouchSensor(void* param) {
  // Flags to only execute code on condition transition
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
          Serial.printf("Pulling: %s", triggerPulling ? "true" : "false");
        } else {
          triggerPulling = false;
          Serial.printf("Pulling: %s", triggerPulling ? "true" : "false");
        }
      }
    }
    lastButtonState = reading;
    delay(triggerPullSensorThreadIterationDelay);
  }
}

void nozzleRecoilSensor(void* param) {
  bool interruption;
  while (true) {
    interruption = digitalRead(nozzleRecoilSensorReadPin);
    if (digitalRead(nozzleRecoilSensorReadPin) == 1) {
      shotsFired++;
      Serial.printf("Shot fired.\n");
      while (digitalRead(nozzleRecoilSensorReadPin) == 1) {
        delay(1);  // wait until piston leaves the barrier to prevent multiple increasements in one shot
      }
    }
  }
}

// ----------------------------------------------------------------------------
// Action Threads
// ----------------------------------------------------------------------------

// void shootOnTouchAndTriggerRoutine(void* param) {
//   while (true) {
//     if (triggerTouching && triggerPulling) {
//       shootAction(true, fireMode);
//     }
//     delay(threadShootOnTouchAndTriggerRoutineSleepDuration);  // time to sleep between each iteration
//   }
// }
