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
#define STEPPER1_DIR_PIN 18  // Arduino D6
#define STEPPER1_STEP_PIN 19 // Arduino D5
#define STEPPER2_DIR_PIN 4   // Arduino D11
#define STEPPER2_STEP_PIN 14 // Arduino D12
#define STEPPER_EN 15        // Arduino D12

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN 32 // Arduino A4

#define trigPin 26 // Arduino A1
#define echoPin 25 // Arduino A2

const int PRINT_INTERVAL = 500;
const int SERVER_INTERVAL = 10;
const int LOOP_INTERVAL_INNER = 8; // strongly affect stability!!!!!!!!!!!!!!
const int LOOP_INTERVAL_OUTER = 10;
const int TURN_INTERVAL = 20;
const int STEPPER_INTERVAL_US = 20;
const int CONTROLLER_INTERVAL = 100;
const int ACTION_INTERVAL = 5000;

// PID control gains
float vertical_kp = 200;  // 200//200//300//300
float vertical_kd = 300;  // 300//400//400//375
float velocity_kp = 0.04; // 0.04//0.03//0.015//0.03
float velocity_ki = velocity_kp / 200;
float turn_kp = 0; // 1
float turn_kd = 0; // 2
float turn_speed = 0.0;
float turn_direction = 0.0;
float camera_kp = 0.000015;
float camera_kd = -0.5;
bool color_detected = false;
bool turning = false;
bool back_to_track = false;

// vertical loop
float pitch;
float bias = 0.07;

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
// tracking trigger
bool tracking = false;

// ultrasonic sensor

long duration;
int distance;
bool ultrasonic_flag = false;
bool stop_flag = false;

// Motion target values
float target_velocity = 0.0;
float last_target_velocity = 0;
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

// Buzzer

const int buzzerPin = 27; // Arduino A5

#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_ALARM 1000

enum BuzzerState
{
    IDLE,
    PLAYING,
    WAITING
};

struct BuzzerTask
{
    int melody[8];
    int noteDurations[8];
    int currentNote;
    unsigned long lastUpdateTime;
    BuzzerState state;
};

BuzzerTask *currentBuzzerTask = nullptr;

BuzzerTask beat1 = {
    {NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C4},
    {8, 8, 8, 8, 8, 8, 8, 8},
    0,
    0,
    IDLE};

BuzzerTask alarm1 = {
    {NOTE_ALARM, NOTE_ALARM, NOTE_ALARM, NOTE_ALARM, NOTE_ALARM, NOTE_ALARM, NOTE_ALARM, NOTE_ALARM},
    {8, 8, 8, 8, 8, 8, 8, 8},
    0,
    0,
    IDLE};

// Global objects
ESP32Timer ITimer(3);
Adafruit_MPU6050 mpu;
step step1(STEPPER_INTERVAL_US, STEPPER1_STEP_PIN, STEPPER1_DIR_PIN);
step step2(STEPPER_INTERVAL_US, STEPPER2_STEP_PIN, STEPPER2_DIR_PIN);
// wifi
AsyncWebServer server(80);
AsyncEventSource events("/events");

// const char *ssid = "x7";
// const char *password = "ctx20040510";
const char *ssid = "VM0459056";
const char *password = "p6zTqmm6vxqc";
// const char *ssid = "201Wi-Fi";
// const char *password = "123456789";
#endif // CONFIG_H
