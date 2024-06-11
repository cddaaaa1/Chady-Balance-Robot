#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Wire.h>
#include <TimerInterrupt_Generic.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <step.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// The Stepper pins
#define STEPPER1_DIR_PIN 16
#define STEPPER1_STEP_PIN 17
#define STEPPER2_DIR_PIN 4
#define STEPPER2_STEP_PIN 14
#define STEPPER_EN 15

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN 32

// IR tracking pins
#define left_track_PIN 27
#define middle_track_PIN 5
#define right_track_PIN 18

const int PRINT_INTERVAL = 500;
const int SERVER_INTERVAL = 10;
const int LOOP_INTERVAL_INNER = 4;
const int LOOP_INTERVAL_OUTER = 10;
const int STEPPER_INTERVAL_US = 20;
const int CONTROLLER_INTERVAL = 100;

// PID control gains
float vertical_kp = 200;  // 200
float vertical_kd = 300;  // 300
float velocity_kp = 0.04; // 0.04
float velocity_ki = velocity_kp / 200;
float turn_kp = -0.5;
float turn_kd = -1;
float turn_speed = 0.0;
float turn_direction = 0.0;
float camera_kp = 0.5;
float camera_kd = 1;

// vertical loop
float pitch;
float bias = 0.04;

// velocity loop
float velocity1;
float velcoity2;
float velocity_input = 0.0;

// turn loop
float yaw = 0.0;
float yaw_rate;
float camera_bias = 0.16;
float yaw_bias = -0.05;
float cam_rho = 0.0;
float cam_theta = 0.0;
float gyro_x = 0.0;
float current_yaw = 0.0;
float previous_yaw = 0.0;
const bool continue_turning = false;

// IR tracking
bool tracking_mode = false;
int decide = 0;
int sensor[3];
int track_error = 0;

// Motion target values
float target_velocity = 0;
float target_angle = 0.0;

// web controller
enum Mode
{
    AUTOMATIC,
    MANUAL,
    STOP
};

Mode currentMode = AUTOMATIC;

bool isForward;
bool isBackward;
bool isLeft;
bool isRight;
bool isStop;

// Global objects
ESP32Timer ITimer(3);
Adafruit_MPU6050 mpu;
step step1(STEPPER_INTERVAL_US, STEPPER1_STEP_PIN, STEPPER1_DIR_PIN);
step step2(STEPPER_INTERVAL_US, STEPPER2_STEP_PIN, STEPPER2_DIR_PIN);
// wifi
AsyncWebServer server(80);
AsyncEventSource events("/events");

const char *ssid = "DuaniPhone";
const char *password = "88888888";
// const char *ssid = "VM0459056";
// const char *password = "p6zTqmm6vxqc";
#endif // CONFIG_H
