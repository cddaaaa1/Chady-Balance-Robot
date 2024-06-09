#include <Arduino.h>
#include <Wire.h>
#include <TimerInterrupt_Generic.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <step.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

// The Stepper pins
#define STEPPER1_DIR_PIN 16  // Arduino D9
#define STEPPER1_STEP_PIN 17 // Arduino D8
#define STEPPER2_DIR_PIN 4   // Arduino D11
#define STEPPER2_STEP_PIN 14 // Arduino D10
#define STEPPER_EN 15        // Arduino D12

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN 32 // Arduino A4

// IR tracking pins
#define left_track_PIN 27  // Arduino D0
#define middle_track_PIN 5 // Arduino D7
#define right_track_PIN 18 // Arduino D6

const int PRINT_INTERVAL = 500;
const int SERVER_INTERVAL = 10;
const int LOOP_INTERVAL_INNER = 4;
const int LOOP_INTERVAL_OUTER = 10;
const int STEPPER_INTERVAL_US = 20;
const int CONTROLLER_INTERVAL = 100;

// PID control gains
static float vertical_kp = 300;   // proportional gain 450 //200//315//300
static float vertical_kd = 350;   // differential gain 450//290//375//350
static float velocity_kp = 0.015; // proportional gain 0.075//0.08//0.015//0.017
static float velocity_ki = velocity_kp / 200;
static float turn_kp = 1; // 0.5
static float turn_kd = 1; // 1
static float turn_speed = 0.2;
static float turn_direction = 0.0; // either +-1 based on direction

// vertical loop
static float pitch;
static float bias = 0.005;

// velocity loop
static float velocity1;
static float velcoity2;
static float velocity_input = 0.0;

// turn loop
static float yaw = 0.0;
static float yaw_rate;
static float yaw_p_bias = 0.16;
static float yaw_bias = -0.05;
static float cam_rho = 0.0;
static float cam_theta = 0.0;
static float gyro_x = 0.0;
static float current_yaw = 0.0;
static float previous_yaw = 0.0;
const bool continue_turning = false;

// IR tracking
static bool tracking_mode = true;
static int decide = 0;
static int sensor[3];
static int track_error = 0;

// Motion target values
static float target_velocity = 0; // 1.3//1.15
static float target_angle = 0.0;  // 90 degree

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
Adafruit_MPU6050 mpu; // Default pins for I2C are SCL: IO22/Arduino D3, SDA: IO21/Arduino D4

step step1(STEPPER_INTERVAL_US, STEPPER1_STEP_PIN, STEPPER1_DIR_PIN);
step step2(STEPPER_INTERVAL_US, STEPPER2_STEP_PIN, STEPPER2_DIR_PIN);
// wifi
AsyncWebServer server(80);
AsyncEventSource events("/events");

const char *ssid = "DuaniPhone";
const char *password = "88888888";
// const char *ssid = "VM0459056";
// const char *password = "p6zTqmm6vxqc";

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

void resetStates()
{
  isForward = false;
  isBackward = false;
  isLeft = false;
  isRight = false;
}

void switchToAutomatic()
{
  resetStates(); // Reset manual states
  currentMode = AUTOMATIC;
  Serial.println("Switched to Automatic Mode");
}

void setupManual()
{
  // Set the motor acceleration values
  target_velocity = 0;
  target_angle = yaw;
}

void switchToManual()
{
  resetStates(); // Reset any automatic states if needed
  currentMode = MANUAL;
  Serial.println("Switched to Manual Mode");
  setupManual(); // Re-initialize manual mode settings
}

// target_velocity, target_angle 大小和极性待调整
void handleManualCommand(String command)
{
  if (command = "forward")
  {
    isForward = true;
    isBackward = false;
    isStop = false;
    isLeft = false;
    isRight = false;
  }
  else if (command = "backward")
  {
    isBackward = true;
    isForward = false;
    isStop = false;
    isLeft = false;
    isRight = false;
  }
  else if (command = "left")
  {
    isLeft = true;
    isForward = false;
    isStop = false;
    isBackward = false;
    isRight = false;
  }
  else if (command = "right")
  {
    isRight = true;
    isForward = false;
    isStop = false;
    isBackward = false;
    isLeft = false;
  }
  else if (command = "stop")
  {
    isForward = false;
    isBackward = false;
    isStop = true;
    isLeft = false;
    isRight = false;
  }
  else
  {
    isForward = false;
    isBackward = false;
    isLeft = false;
    isRight = false;
    isStop = false;
  }
}

void handleController(AsyncWebServerRequest *request)
{
  if (request->hasArg("command"))
  {
    String command = request->arg("command");
    if (command == "switch_to_auto")
    {
      switchToAutomatic();
      Serial.println("Switched to Automatic Mode");
    }
    else if (command == "switch_to_manual")
    {
      switchToManual();
      Serial.println("Switched to Manual Mode");
    }
    else
    {
      if (currentMode == MANUAL)
      {
        handleManualCommand(command);
      }
    }
    request->send(200, "text/plain", "Command received: " + command);
  }
  else
  {
    request->send(200, "text/plain", "No command received");
  }
}

void loopAutomatic()
{
  tracking_mode = true;
}

void loopManual()
{
  if (isForward)
  {
    target_velocity = 1;
  }
  else if (isBackward)
  {
    target_velocity = -1;
  }
  else if (isStop)
  {
    target_velocity = 0;
  }
  else if (isLeft)
  {
    target_angle = 0.4;
  }
  else if (isRight)
  {
    target_angle = -0.4;
  }
}

// curl -X GET "http://172.20.10.2/setVariable?cmd=velocity_kp=1.0"
void handleSetVariable(AsyncWebServerRequest *request)
{
  if (request->hasArg("cmd"))
  {
    String cmd = request->arg("cmd");
    int eqIndex = cmd.indexOf('=');
    if (eqIndex != -1)
    {
      String varName = cmd.substring(0, eqIndex);
      float varValue = cmd.substring(eqIndex + 1).toFloat();
      if (varName == "vertical_kp")
      {
        vertical_kp = varValue;
      }
      else if (varName == "vertical_kd")
      {
        vertical_kd = varValue;
      }

      else if (varName == "velocity_kp")
      {
        velocity_kp = varValue;
        velocity_ki = velocity_kp / 200;
      }
      else if (varName == "velocity_ki")
      {
        velocity_ki = varValue;
      }
      else if (varName == "turn_kp")
      {
        turn_kp = varValue;
      }
      else if (varName == "turn_kd")
      {
        turn_kd = varValue;
      }
      else if (varName == "turn_speed")
      {
        turn_speed = varValue;
      }
      else if (varName == "turn_direction")
      {
        turn_direction = varValue;
      }
      else if (varName == "target_velocity")
      {
        target_velocity = varValue;
      }
      else if (varName == "target_angle")
      {
        target_angle = varValue;
      }
      else if (varName == "bias")
      {
        bias = varValue;
      }
      else if (varName == "yaw_bias")
      {
        yaw_bias = varValue;
      }
      else
      {
        request->send(400, "text/plain", "Invalid Variable Name");
        return;
      }
      request->send(200, "text/plain", "update varible");
      Serial.println("setvariable received");
    }
    else
    {
      request->send(400, "text/plain", "Invalid Command Format");
    }
  }
  else
  {
    request->send(400, "text/plain", "Invalid Request");
  }
}

void handleGetVariables(AsyncWebServerRequest *request)
{
  // Create a JSON response with current variable values
  StaticJsonDocument<500> jsonResponse;
  jsonResponse["vertical_kp"] = vertical_kp;
  jsonResponse["vertical_kd"] = vertical_kd;
  jsonResponse["velocity_kp"] = velocity_kp;
  jsonResponse["velocity_ki"] = velocity_ki;
  jsonResponse["turn_kp"] = turn_kp;
  jsonResponse["turn_kd"] = turn_kd;
  jsonResponse["turn_speed"] = turn_speed;
  jsonResponse["turn_direction"] = turn_direction;
  jsonResponse["target_velocity"] = target_velocity;
  jsonResponse["target_angle"] = target_angle;
  jsonResponse["bias"] = bias;
  jsonResponse["yaw_bias"] = yaw_bias;

  String response;
  serializeJson(jsonResponse, response);

  request->send(200, "application/json", response);
}

void handleData(AsyncWebServerRequest *request)
{
  // Create a JSON response
  StaticJsonDocument<200> jsonResponse;
  jsonResponse["pitch"] = pitch;
  jsonResponse["velocity"] = velocity1;

  String response;
  serializeJson(jsonResponse, response);

  request->send(200, "application/json", response);
}

// void handleCamera(AsyncWebServerRequest *request)
// {
//   if (request->hasParam("x", true))
//   {
//     cam_rho = request->getParam("x", true)->value().toFloat();
//     // Serial.print("rho: ");
//     // Serial.println(cam_rho);
//   }
//   if (request->hasParam("y", true))
//   {
//     cam_theta = request->getParam("y", true)->value().toFloat();
//     // Serial.print("theta: ");
//     // Serial.println(cam_theta);
//   }

//   // Send a response back to the client indicating data was received
//   request->send(200, "text/plain", "Data received");
// }

void setup()
{
  Serial.begin(115200);
  pinMode(TOGGLE_PIN, OUTPUT);
  pinMode(left_track_PIN, INPUT);
  pinMode(middle_track_PIN, INPUT);
  pinMode(right_track_PIN, INPUT);

  // wifi setup
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.addHandler(&events);
  // server.on("/command", HTTP_GET, handleCommand);
  server.on("/setVariable", HTTP_POST, handleSetVariable);
  server.on("/data", HTTP_GET, handleData);
  server.on("/getVariables", HTTP_GET, handleGetVariables);
  // server.on("/camera", HTTP_POST, handleCamera);
  server.on("/controller", HTTP_POST, handleController);
  server.begin();
  Serial.println("HTTP server started");

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

  // Set motor acceleration values
  step1.setAccelerationRad(10.0);
  step2.setAccelerationRad(10.0);

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
  static float yaw = 0.0;     // yaw angle
  // Calculate the angle from accelerometer data
  theta_a = atan(a.acceleration.z / a.acceleration.x);

  // Calculate the angle change from gyroscope data
  theta_g = g.gyro.y * (LOOP_INTERVAL_INNER / 1000); // adjust dt into second

  // Complementary filter to combine accelerometer and gyroscope data
  theta = (alpha * (theta + theta_g)) + ((1 - alpha) * theta_a);

  return theta;
}

float yawAngle(sensors_event_t a, sensors_event_t g)
{
  static float alpha_yaw = 0.98;
  yaw += g.gyro.x * (LOOP_INTERVAL_INNER / 1000); // adjust dt into second
  yaw = alpha_yaw * (yaw + g.gyro.x * LOOP_INTERVAL_INNER / 1000) + (1 - alpha_yaw) * yaw;
  gyro_x = g.gyro.x;
  return yaw;
}
// float yawCalibration (sensors_event_t a, sensors_event_t g)
// {
//   static unsigned long drift_summation = 0;
//   static unsigned long sample_count = 1;
//   static float average_drift;
//   if(abs(g.gyro.x) < 0.01)
//   {
//     drift_summation += g.gyro.x;
//   }

//     average_drift = drift_summation/sample_count;
//     sample_count++;

//   Serial.print("average_drift ");
//   Serial.println(average_drift);
//   return average_drift;
// }
float vertical(float angle_input, float gyro_y) // angle bias, current tilt angle, gyro_y
{
  static float output;

  output = (angle_input - pitch) * vertical_kp - gyro_y * vertical_kd;

  return output;
}

static float err;

float veloctiy(float step1_velocity, float step2_velocity)
{
  static float output;
  static float velocity_err;
  static float velocity_err_last;
  const float a = 0.7; // low pass filter coefficient
  static float velocity_err_integ;

  velocity_err = velocity_input - (step1_velocity + step2_velocity) / 2;
  velocity_err = (1 - a) * velocity_err + a * velocity_err_last;
  velocity_err_last = velocity_err;

  // if (velocity_err < 5 && velocity_err > -5) // only integrate when velocity error is small
  // {
  //   velocity_err_integ += velocity_err;
  //   err = velocity_err_integ;
  // }
  if (velocity_err_integ > 0.1) // limit the integral
  {
    velocity_err_integ = 0.1;
  }
  if (velocity_err_integ < -0.1)
  {
    velocity_err_integ = 1;
  }
  if (pitch > 0.5 || pitch < -0.5) // clear the integral when robot falls
  {
    velocity_err_integ = 0;
  }
  output = velocity_kp * velocity_err + velocity_ki * velocity_err_integ;
  return output;
}

// float turn(float gyro_x, float yaw)
// {
//   static float output;
//   if (tracking_mode)
//   {
//     static float last_cam_theta;
//     static float filter_coeff = 0.8;
//     cam_theta = (1 - filter_coeff) * cam_theta + filter_coeff * last_cam_theta;
//     // output = -(cam_theta - yaw_p_bias) * turn_kp - gyro_x * turn_kd;
//     output = (cam_theta - 1.55) * turn_kp - gyro_x * turn_kd;
//   }
//   else
//   {
//     if (continue_turning)
//     {
//       output = turn_direction * turn_speed;
//     }
//     else
//     {
//       output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
//     }
//   }

//   return output;
// }

// void read_sensor_values()
// {
//   sensor[0] = digitalRead(left_track_PIN);
//   sensor[1] = digitalRead(middle_track_PIN);
//   sensor[2] = digitalRead(right_track_PIN);
//   if (sensor[0] == 1 && sensor[1] == 1 && sensor[2] == 1)
//   {
//     decide = 1; // go across the cross
//   }
//   else if (sensor[0] == 0 && sensor[1] == 1 && sensor[2] == 1)
//   {
//     decide = 2; // turn right ( see right angle )
//   }
//   else if (sensor[0] == 1 && sensor[1] == 1 && sensor[2] == 0)
//   {
//     decide = 3; // turn left (see right angle)
//   }
//   else if (sensor[0] == 1 && sensor[1] == 0 && sensor[2] == 0)
//   {
//     track_error = 1; // turn left
//   }
//   else if (sensor[0] == 0 && sensor[1] == 0 && sensor[2] == 1)
//   {
//     track_error = -1; // turn right
//   }
//   else if (sensor[0] == 0 && sensor[1] == 1 && sensor[2] == 0)
//   {
//     track_error = 0; // go forward
//   }
//   else if (sensor[0] == 0 && sensor[1] == 0 && sensor[2] == 0)
//   {
//     if (track_error == -1)
//     {
//       track_error = -2; // left overshoot
//     }
//     else if (track_error == 1)
//     {
//       track_error = 2; // right overshoot
//     }
//   }
//   // Serial.println(sensor[0]);
// }
float turn(float gyro_x, float yaw)
{
  sensor[0] = digitalRead(left_track_PIN);
  sensor[1] = digitalRead(middle_track_PIN);
  sensor[2] = digitalRead(right_track_PIN);
  static float output;
  static bool already_set;
  if (tracking_mode)
  {
    if (sensor[1] == 1)
    {
      output = turn_speed * 1;
    }
    if (sensor[1] == 0)
    {
      output = turn_speed * -1;
    }
  }
  return output;
}

// float turn(float gyro_x, float yaw)
// {
//   static float output;
//   static bool already_set;
//   // to make sure target value wont change before done the action,
//   // without setting it, target angle will change all the time, so insetead of moving to current_yaw - some_angle
//   // it will change continuously
//   // reset after middle sensor see the line, and waithing for set next target angle.
//   if (tracking_mode)
//   {
//     read_sensor_values();
//     if (decide == 3)
//     {
//       if (already_set == false)
//       {
//         target_angle = yaw + (90 / 180 * 3.14);
//         already_set = true;
//         target_velocity = 0;
//       }
//       // 加了个decide清零，具体值得调
//       // if ((target_angle - yaw < 0.05 || yaw - target_angle < 0.05) && (gyro_x < 1 || gyro_x > -1))
//       // {
//       //   decide == 0;
//       //   already_set = false;
//       // }
//       output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
//     }
//     if (decide == 2 && already_set == false)
//     {
//       if (already_set == false)
//       {
//         target_angle = yaw - (90 / 180 * 3.14);
//         already_set = true;
//       }
//       // 加了个decide清零，具体值得调
//       // if ((target_angle - yaw < 0.05 || yaw - target_angle < 0.05) && (gyro_x < 1 || gyro_x > -1))
//       // {
//       //   decide == 0;
//       //   already_set = false;
//       // }
//       output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
//     }
//     if (decide == 1)
//     {
//       target_angle = yaw;
//       already_set = false;
//       output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
//     }
//     // 看不懂 error 和track_error的区别? error ==1 和error !=0 重复了？
//     if (track_error == 0)
//     {
//       output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
//     }
//     if (track_error == 1)
//     { // continuously turn, stop when middle sensor see the line
//       output = turn_speed * -1;
//     }
//     if (track_error == -1)
//     {
//       output = turn_speed * +1;
//     }
//     if (track_error == -2)
//     {
//       output = turn_speed * +1;
//     }
//     if (track_error == 2)
//     {
//       output = turn_speed * -1;
//     }
//   }
//   else
//   {
//     if (continue_turning)
//     {
//       output = turn_direction * turn_speed;
//     }
//     else
//     {
//       output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
//     }
//   }

//   return output;
// }

void loop()
{
  // Static variables are initialised once and then the value is remembered betweeen subsequent calls to this function
  static unsigned long printTimer = 0;      // time of the next print
  static unsigned long loopTimer_inner = 0; // time of the next control update
  static unsigned long serverTimer = 0;
  static unsigned long loopTimer_outter = 0;
  static unsigned long controllerTimer = 0;

  // vertical loop
  static float vertical_output;

  // velocity loop
  static float velocity_output;

  // turn loop
  static float turn_output;
  static float yaw_drift;
  static float current_target_angle;
  static float drift_summation = 0;
  static unsigned long sample_count = 1;
  static float average_drift;

  // PWM output
  static float acc_input1;
  static float acc_input2;

  // Web Controller loop
  if (millis() > controllerTimer)
  {
    switch (currentMode)
    {
    case AUTOMATIC:
      pinMode(STEPPER_EN, OUTPUT);
      digitalWrite(STEPPER_EN, false);
      loopAutomatic();
      break;
    case MANUAL:
      setupManual(); // Initialize manual mode
      loopManual();
    }
  }

  // Run the control loop every LOOP_INTERVAL ms
  if (millis() > loopTimer_inner)
  {
    loopTimer_inner += LOOP_INTERVAL_INNER;

    // Fetch data from MPU6050
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    pitch = titltAngle(a, g);
    yaw = yawAngle(a, g);
    if (abs(g.gyro.x) < 0.01)
    {
      drift_summation += (yaw - previous_yaw);
      average_drift = drift_summation / sample_count;
      sample_count++;
      yaw -= average_drift;
      previous_yaw = yaw;
    }
    else
    {
      previous_yaw = yaw;
      drift_summation = 0;
      sample_count = 1;
      average_drift = 0;
    }

    // stepper motor velocity
    velocity1 = step1.getSpeedRad();
    velcoity2 = step2.getSpeedRad();

    // turn loop

    turn_output = turn(g.gyro.x, yaw);

    if (millis() > loopTimer_outter)
    {
      loopTimer_outter += LOOP_INTERVAL_OUTER;
      velocity_input = target_velocity;
      velocity_output = veloctiy(velocity1, velcoity2);
    }

    // vertical loop
    vertical_output = vertical(bias + velocity_output, g.gyro.y);

    // acceleration input
    acc_input1 = vertical_output + turn_output;
    acc_input2 = vertical_output - turn_output;

    // Set target motor acceleration proportional to tilt angle
    if (pitch > 0.4 || pitch < -0.4)
    {
      step1.setTargetSpeedRad(0);
      step2.setTargetSpeedRad(0);
      // Serial.println("fall detected");
      step1.setAccelerationRad(100);
      step2.setAccelerationRad(100);
    }
    else
    {
      step1.setAccelerationRad(acc_input1);
      step2.setAccelerationRad(acc_input2);
      if (acc_input1 > 0 && acc_input2 > 0)
      {
        step1.setTargetSpeedRad(-20);
        step2.setTargetSpeedRad(-20);
      }
      else if (acc_input1 < 0 && acc_input2 < 0)
      {
        step1.setTargetSpeedRad(20);
        step2.setTargetSpeedRad(20);
      }
      else if (acc_input1 > 0 && acc_input2 < 0)
      {
        step1.setTargetSpeedRad(-20);
        step2.setTargetSpeedRad(20);
      }
      else if (acc_input1 < 0 && acc_input2 > 0)
      {
        step1.setTargetSpeedRad(20);
        step2.setTargetSpeedRad(-20);
      }
    }

    // Print updates every PRINT_INTERVAL ms
    if (millis() > printTimer)
    {
      printTimer += PRINT_INTERVAL;
      Serial.print("theta: ");
      Serial.println(cam_theta / 3.1415926 * 180);
      Serial.print("yaw: ");
      Serial.println(yaw / 3.1415926 * 180);
      Serial.print("ratio ");
      Serial.println((cam_theta - yaw_p_bias) / yaw);
      Serial.print("decide:");
      Serial.println(decide);
      Serial.print("track_error:");
      Serial.println(track_error);
      Serial.print("sensor[0]: ");
      Serial.println(sensor[0]);
      Serial.print("sensor[1]: ");
      Serial.println(sensor[1]);
      Serial.print("sensor[2]: ");
      Serial.println(sensor[2]);
      Serial.print("yaw");
      Serial.println(pitch);
    }
  }
}
