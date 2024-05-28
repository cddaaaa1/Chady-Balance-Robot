#ifndef AUTOMATIC_H
#define AUTOMATIC_H

#include <Arduino.h>
#include <TimerInterrupt_Generic.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <step.h>

const int PRINT_INTERVAL = 500;
const int LOOP_INTERVAL = 10;
const int STEPPER_INTERVAL_US = 20;

const float kx = 20.0;

// The Stepper pins
#define STEPPER1_DIR_PIN 16   //Arduino D9
#define STEPPER1_STEP_PIN 17  //Arduino D8
#define STEPPER2_DIR_PIN 4    //Arduino D11
#define STEPPER2_STEP_PIN 14  //Arduino D10
#define STEPPER_EN 15         //Arduino D12

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN  32        //Arduino A4

// Global objects
ESP32Timer ITimer(3);
Adafruit_MPU6050 mpu;  // Default pins for I2C are SCL: IO22/Arduino D3, SDA: IO21/Arduino D4

step step1(STEPPER_INTERVAL_US, STEPPER1_STEP_PIN, STEPPER1_DIR_PIN);
step step2(STEPPER_INTERVAL_US, STEPPER2_STEP_PIN, STEPPER2_DIR_PIN);

bool TimerHandler(void * timerNo) {
  static bool toggle = false;

  step1.runStepper();
  step2.runStepper();

  digitalWrite(TOGGLE_PIN, toggle);
  toggle = !toggle;
  return true;
}

void setupAutomatic() {
  Serial.begin(115200);
  pinMode(TOGGLE_PIN, OUTPUT);

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  if (!ITimer.attachInterruptInterval(STEPPER_INTERVAL_US, TimerHandler)) {
    Serial.println("Failed to start stepper interrupt");
    while (1) delay(10);
  }
  Serial.println("Initialised Interrupt for Stepper");

  step1.setAccelerationRad(10.0);
  step2.setAccelerationRad(10.0);
  pinMode(STEPPER_EN, OUTPUT);
  digitalWrite(STEPPER_EN, false);
}

void loopAutomatic() {
  static unsigned long printTimer = 0;
  static unsigned long loopTimer = 0;
  static float tiltx = 0.0;

  if (millis() > loopTimer) {
    loopTimer += LOOP_INTERVAL;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    tiltx = a.acceleration.z / 9.67;
    step1.setTargetSpeedRad(tiltx * kx);
    step2.setTargetSpeedRad(-tiltx * kx);
  }

  if (millis() > printTimer) {
    printTimer += PRINT_INTERVAL;
    Serial.print(tiltx * 1000);
    Serial.print(' ');
    Serial.print(step1.getSpeedRad());
    Serial.println();
  }
}

#endif // AUTOMATIC_H




