#include <Arduino.h>
#include <Wire.h>
#include <TimerInterrupt_Generic.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <step.h>

// The Stepper pins
#define STEPPER1_DIR_PIN 16  // Arduino D9
#define STEPPER1_STEP_PIN 17 // Arduino D8
#define STEPPER2_DIR_PIN 4   // Arduino D11
#define STEPPER2_STEP_PIN 14 // Arduino D10
#define STEPPER_EN 15        // Arduino D12

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN 32 // Arduino A4

const int PRINT_INTERVAL = 500;
const int LOOP_INTERVAL = 10;
const int STEPPER_INTERVAL_US = 20;

// PID control gains
const float vertical_kp = 300; // proportional gain 300
const float vertical_kd = 200; // differential gain 200
const float velocity_kp = 0.015;
const float velocity_ki = velocity_kp / 200;
const float turn_kp = 0.0;
const float turn_kd = -0.1;

// sensor data
static float pitch;

// Motion target values
static float target_velocity = 0.0;
static float target_angle = 0.0;

// Global objects
ESP32Timer ITimer(3);
Adafruit_MPU6050 mpu; // Default pins for I2C are SCL: IO22/Arduino D3, SDA: IO21/Arduino D4

step step1(STEPPER_INTERVAL_US, STEPPER1_STEP_PIN, STEPPER1_DIR_PIN);
step step2(STEPPER_INTERVAL_US, STEPPER2_STEP_PIN, STEPPER2_DIR_PIN);

// Interrupt Service Routine for motor update
// Note: ESP32 doesn't support floating point calculations in an ISR
bool TimerHandler(void *timerNo)
{
  static bool toggle = false;

  // Update the stepper motors
  step1.runStepper();
  step2.runStepper();

  // Indicate that the ISR is running
  digitalWrite(TOGGLE_PIN, toggle);
  toggle = !toggle;
  return true;
}

void setup()
{
  Serial.begin(115200);
  pinMode(TOGGLE_PIN, OUTPUT);

  // Try to initialize Accelerometer/Gyroscope
  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1)
    {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_44_HZ);

  // Attach motor update ISR to timer to run every STEPPER_INTERVAL_US μs
  if (!ITimer.attachInterruptInterval(STEPPER_INTERVAL_US, TimerHandler))
  {
    Serial.println("Failed to start stepper interrupt");
    while (1)
      delay(10);
  }
  Serial.println("Initialised Interrupt for Stepper");

  step1.setTargetSpeedRad(19);
  step2.setTargetSpeedRad(19);

  // Enable the stepper motor drivers
  pinMode(STEPPER_EN, OUTPUT);
  digitalWrite(STEPPER_EN, false);
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
  theta_g = g.gyro.y * (LOOP_INTERVAL / 1000); // adjust dt into second

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

static float err;

float veloctiy(float step1_velocity, float step2_velocity)
{
  static float output;
  static float velocity_err;
  static float velocity_err_last;
  float a = 0.5; // low pass filter coefficient
  static float velocity_err_integ;

  velocity_err = target_velocity - (step1_velocity + step2_velocity) / 2;
  velocity_err = (1 - a) * velocity_err + a * velocity_err_last;
  velocity_err_last = velocity_err;

  if (velocity_err < 5 && velocity_err > -5) // only integrate when velocity error is small
  {
    velocity_err_integ += velocity_err;
    err = velocity_err_integ;
  }
  if (velocity_err_integ > 20) // limit the integral
  {
    velocity_err_integ = 20;
  }
  if (velocity_err_integ < -20)
  {
    velocity_err_integ = 20;
  }
  if (pitch > 1.5 || pitch < -1.5) // clear the integral when robot falls
  {
    velocity_err_integ = 0;
    Serial.println("fall detected!");
  }
  output = velocity_kp * velocity_err + velocity_ki * velocity_err_integ;
  return output;
}

float turn(float gyro_x)
{
  static float output;
  if (target_angle == 0.0) // suppresss the turn
  {
    output = turn_kd * gyro_x;
  }
  else // turn to target angle
  {
    output = turn_kp * target_angle;
  }

  return output;
}

void loop()
{
  // Static variables are initialised once and then the value is remembered betweeen subsequent calls to this function
  static unsigned long printTimer = 0; // time of the next print
  static unsigned long loopTimer = 0;  // time of the next control update

  static float bias = 0.05; // radis bias when robot is vertical

  // vertical loop
  static float vertical_output;

  // velocity loop
  static float velocity1;
  static float velcoity2;
  static float velocity_output;

  // turn loop
  static float turn_output;

  // PWM output
  static float acc_input1;
  static float acc_input2;

  // Run the control loop   every LOOP_INTERVAL ms
  if (millis() > loopTimer)
  {
    loopTimer += LOOP_INTERVAL;

    // Fetch data from MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    pitch = titltAngle(a, g);

    // stepper motor velocity
    velocity1 = step1.getSpeedRad();
    velcoity2 = step2.getSpeedRad();

    // velocity loop
    velocity_output = veloctiy(velocity1, velcoity2);

    // vertical loop
    vertical_output = vertical(bias + velocity_output, g.gyro.y);

    // turn loop
    turn_output = turn(g.gyro.x);

    // acceleration input
    acc_input1 = vertical_output + turn_output;
    acc_input2 = vertical_output - turn_output;

    // Set target motor acceleration proportional to tilt angle
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

    velocity1 = step1.getSpeedRad();
    velcoity2 = step2.getSpeedRad();

    // Print updates every PRINT_INTERVAL ms
    if (millis() > printTimer)
    {
      printTimer += PRINT_INTERVAL;
      Serial.print("err:");
      Serial.println(err);
    }
  }
}
