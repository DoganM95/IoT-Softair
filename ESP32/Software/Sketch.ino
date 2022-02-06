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
const unsigned short int pistonSensorReadPin = 34;  // Read: 1 = infrared barrier free, 0 = IR interrupted
const unsigned short int triggerPullReadPin = 32;   // Read: 1 = Shoot, 0 = Stop
const unsigned short int triggerTouchReadPin = 4;
const unsigned short int motorControlPin = 33;  // HIGH = shootAction
const unsigned short int neoPixelPin = 23;

// PWM channels and settings
const int freq = 5000;
const int shotChannel = 0;  // Shot Sensor
const int resolution = 8;   // 8 Bits resulting in 0-255 as Range for Duty Cycle

// Configurations
const unsigned short int burstShootCount = 3;     // Defines how many BB's to shootAction in burst mode with one trigger pull
const unsigned int touchDetectionThreshold = 18;  // Adjustment of touch pin sensitivity
const int triggerDebounceTime = 100;
char* fireMode;

// Thread Adjustments
const short int threadSetTriggerTouchedStateRoutineSleepDuration = 100;
const short int threadSetTriggerPulledStateRoutineSleepDuration = 100;
const short int threadShootOnTouchAndTriggerRoutineSleepDuration = 100;

TaskHandle_t triggerTouchSensorThreadHandle;
TaskHandle_t triggerPullSensorThreadHandle;
TaskHandle_t shootOnTouchAndTriggerRoutineThreadHandle;
TaskHandle_t shotCountDetectSensorThreadHandle;
TaskHandle_t setSystemSleepStateRoutineThreadHandle;

bool triggerEnabledByTouch;
bool triggerPulledByFinger;
unsigned long long shotsFired = 0;
unsigned short triggerTouchValue = 0;

char* fireMode = "semi-automatic";

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(4, neoPixelPin, NEO_GRB + NEO_KHZ800);

void shootAction(bool state, char* shootMode = "") {
  unsigned int duration = 0;
  unsigned int shots = 0;

  if (state) {
    if (shootMode == "semi-automatic") {
      digitalWrite(motorControlPin, HIGH);
      while (digitalRead(pistonSensorReadPin) == 1) {  // while infrared barrier is not blocked, wait
        delay(1);                                      // FIXME: needs to be replaced by a time check
        duration++;
        if (duration >= 3000) {
          goto skipCounterCausedByBarrierError;
          break;  // security mechanism to stop shooting after x ms (in case of sensor failure)}
        }
      }

      shotsFired++;
    skipCounterCausedByBarrierError:
      digitalWrite(motorControlPin, LOW);  // on infrared barrier blocked, cut motor electricity
    }

    else if (shootMode == "burst") {
      digitalWrite(motorControlPin, HIGH);
      while (digitalRead(pistonSensorReadPin) == 1) {
        delay(1);
      }
    }

    else if (shootMode == "full-automatic") {  // shoots until either trigger is released or finger stopped touching trigger
      while (digitalRead(triggerPullReadPin) == HIGH && touchRead(4) <= touchDetectionThreshold) {
      continueShooting:
        digitalWrite(motorControlPin, HIGH);
      }
      delay(triggerDebounceDelay / 4);
      if (digitalRead(triggerPullReadPin) == LOW || touchRead(4) >= touchDetectionThreshold) {
        digitalWrite(motorControlPin, LOW);
      } else {
        goto continueShooting;
      }
    }

  } else {
    digitalWrite(motorControlPin, LOW);
  }
}

// ----------------------------------------------------------------------------
// Sensor Threads
// ----------------------------------------------------------------------------

void triggerTouchSensor(void* param) {
  bool setEnabledFlag = false;
  bool setDisabledFlag = false;
  short int tempTouchValue;

  while (true) {
    tempTouchValue = touchRead(triggerTouchReadPin);
    triggerTouchValue = tempTouchValue;
    if (tempTouchValue <= touchDetectionThreshold && setEnabledFlag == false) {
      triggerEnabledByTouch = true;
      setEnabledFlag = true;
      setDisabledFlag = false;
    } else if (tempTouchValue > touchDetectionThreshold && setDisabledFlag == false) {
      triggerEnabledByTouch = false;
      setEnabledFlag = false;
      setDisabledFlag = true;
    }
    delay(threadSetTriggerTouchedStateRoutineSleepDuration);
  }
}

void triggerPullSensor(void* param) {
  bool setEnabledFlag = false;
  bool setDisabledFlag = false;
  bool touchFlag = false;

  unsigned int timerLow = 0;
  unsigned int timerHigh = 0;
  unsigned int timeoutTimer = 0;

  while (true) {
    timerHigh = 0;
    timerLow = 0;

    if (triggerEnabledByTouch) {
      touchFlag = true;
      Serial.printf("-----------------------------------------------------\n");
      Serial.printf("pullThread - Trigger Touching: %d\n", triggerTouchValue);

      while (digitalRead(triggerPullReadPin) == HIGH && setEnabledFlag == false) {
        timerHigh++;
        Serial.printf("pullThread - Time High: %d\n", timerHigh);
        if (timerHigh >= 10) {
          Serial.printf("pullThread - pulling trigger and touching, timerHigh = %d\n", timerHigh);
          triggerPulledByFinger = true;
          setEnabledFlag = true;
          setDisabledFlag = false;
        }
        delay(1);
      }

      while (digitalRead(triggerPullReadPin) == LOW && setDisabledFlag == false) {
        timerLow++;
        Serial.printf("pullThread - Time Low: %d\n", timerLow);
        if (timerLow >= 10) {
          Serial.printf("pullThread - released trigger or not touching, timerLow = %d\n", timerLow);
          triggerPulledByFinger = false;
          setEnabledFlag = false;
          setDisabledFlag = true;
        }
        delay(1);
      }

    } else if (!triggerEnabledByTouch) {
      // setEnabledFlag = false;
      // setDisabledFlag = false;
      touchFlag = false;
      triggerPulledByFinger = false;
      Serial.printf("pullThread - stopped Touching Trigger: %d\n", triggerTouchValue);
    }

    delay(threadSetTriggerPulledStateRoutineSleepDuration);
  }
}

void shootOnTouchAndTriggerRoutine(void* param) {
  while (true) {
    if (triggerEnabledByTouch && triggerPulledByFinger) {
      shootAction(true, fireMode);
    }
    delay(threadShootOnTouchAndTriggerRoutineSleepDuration);  // time to sleep between each iteration
  }
}

void shotDetectSensor(void* param) {
  bool incrementedCountFlag = false;
  while (true) {
    if (digitalRead(pistonSensorReadPin) == 0 && incrementedCountFlag == false) {
      // shotsFired++;
      while (digitalRead(pistonSensorReadPin) == 0) {
        delay(1);  // wait until piston leaves the barrier to prevent multiple increasements in one shot
      }
    }
  }
}

// ----------------------------------------------------------------------------
// SETUP
// ----------------------------------------------------------------------------
void setup() {
  Serial.begin(115200);

  ledcSetup(shotChannel, freq, resolution);
  ledcAttachPin(pistonSensorReadPin, shotChannel);
  pinMode(pistonSensorReadPin, INPUT_PULLDOWN);
  pinMode(triggerPullReadPin, INPUT_PULLDOWN);
  pinMode(motorControlPin, OUTPUT);

  Serial.printf("initial touch: %d, initial pull: %d\n", touchRead(triggerTouchReadPin), digitalRead(triggerPullReadPin));

  disableCore0WDT();  // Disable WatchDogTimeout, so threads can run as long as they want
  disableCore1WDT();  // Same

  xTaskCreatePinnedToCore(triggerTouchSensor, "triggerTouchSensor", 10000, NULL, 20, &triggerTouchSensorThreadHandle, 1);
  xTaskCreatePinnedToCore(triggerPullSensor, "triggerPullSensor", 10000, NULL, 20, &triggerPullSensorThreadHandle, 1);
  xTaskCreatePinnedToCore(shotDetectSensor, "shotSensor", 10000, NULL, 20, &shotCountDetectSensorThreadHandle, 1);

  xTaskCreatePinnedToCore(shootActionRoutine, "shotSensor", 10000, NULL, 20, &shootActionRoutineThreadHandle, 1);
}
// ----------------------------------------------------------------------------
// MAIN LOOP
// ----------------------------------------------------------------------------
void loop() {}