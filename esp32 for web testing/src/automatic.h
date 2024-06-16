#ifndef AUTOMATIC_H
#define AUTOMATIC_H

#include <Arduino.h>
#include <TimerInterrupt_Generic.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <step.h>
#include <Ticker.h>

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

#define LOOP_INTERVAL_ms 10
static float pitch; //sensor result

static float vertical_kp = 300;
static float vertical_kd = 200;

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




  if (!ITimer.attachInterruptInterval(STEPPER_INTERVAL_US, TimerHandler)) {
    Serial.println("Failed to start stepper interrupt");
    while (1) delay(10);
  }

  Serial.println("Initialised Interrupt for Stepper");

  step1.setAccelerationRad(10.0);
  step2.setAccelerationRad(10.0);
  pinMode(STEPPER_EN, OUTPUT);
  digitalWrite(STEPPER_EN, LOW);

}

float titltAngle(sensors_event_t a, sensors_event_t g)
{
  float alpha = 0.98;         // Complementary filter coefficient, C in git hub page
  static float theta = 0.0;   // current tilt angle, tiltx in orginal code
  static float theta_a = 0.0; // angle measured by accelerometer
  static float theta_g = 0.0; // angle measured by gyroscope

  // Calculate the angle from accelerometer data
  theta_a = atan(a.acceleration.z / sqrt(pow(a.acceleration.y, 2) + pow(a.acceleration.x, 2)));

  // Calculate the angle change from gyroscope data
  theta_g = g.gyro.y * (LOOP_INTERVAL_ms / 1000); // adjust dt into second

  // Complementary filter to combine accelerometer and gyroscope data
  theta = (alpha * (theta + theta_g)) + ((1 - alpha) * theta_a);

  return theta;
}

float vertical(float bias, float gyro_y) // angle bias, current tilt angle, gyro_y
{
  static float output = 0.0;

  output = (bias - pitch) * vertical_kp - gyro_y * vertical_kd;

  return output;
}

void loopAutomatic() {
  static unsigned long printTimer = 0;
  static unsigned long loopTimer = 0;
  static float bias = 0.0;

  static float vertical_output;



  if (millis() > loopTimer) {
    loopTimer += LOOP_INTERVAL;

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    vertical_output = vertical(bias, g.gyro.y);
    step1.setAccelerationRad(vertical_output);
    step2.setAccelerationRad(vertical_output);
    if (vertical_output > 0)
    {
      step1.setTargetSpeedRad(-18);
      step2.setTargetSpeedRad(-18);
    }
    else
    {
      step1.setTargetSpeedRad(18);
      step2.setTargetSpeedRad(18);
    }
  }

  if (millis() > printTimer) {
    printTimer += PRINT_INTERVAL;
    // Serial.print(tiltx * 1000);
    // Serial.print(' ');
    // Serial.print(step1.getSpeedRad());
    // Serial.println();
  }
}

#endif // AUTOMATIC_H




