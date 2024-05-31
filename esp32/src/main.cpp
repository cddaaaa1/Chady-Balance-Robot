#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <TimerInterrupt_Generic.h>
#include "manual.h"
#include "automatic.h"

// The Stepper pins
#define STEPPER1_DIR_PIN 16   //Arduino D9
#define STEPPER1_STEP_PIN 17  //Arduino D8
#define STEPPER2_DIR_PIN 4    //Arduino D11
#define STEPPER2_STEP_PIN 14  //Arduino D10
#define STEPPER_EN 15         //Arduino D12

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN  32        //Arduino A4
#define LOOP_INTERVAL_ms 10


// Define motion flags
bool isForward = false;
bool isBackward = false;
bool isLeft = false;
bool isRight = false;

// WiFi credentials
const char* ssid = "x7";
const char* password = "ctx20040510";

// Initialize the web server on port 80
WebServer server(80);

enum Mode { AUTOMATIC, MANUAL, STOP };
Mode currentMode = MANUAL; // Default mode


void resetStates() {
    isForward = false;
    isBackward = false;
    isLeft = false;
    isRight = false;
}

// void stopAutomaticOperations() {
//     ITimer.detachInterrupt(); 
//     Serial.println("Interrupts detached.");
// }

void switchToAutomatic() {
    resetStates();  // Reset manual states
    //stopAutomaticOperations();  // Stop any possibly lingering automatic operations
    // delay(100);
    currentMode = AUTOMATIC;
    Serial.println("Switched to Automatic Mode");
    //setupAutomatic();  // Re-initialize automatic mode settings, including re-attaching interrupts
}

void switchToManual() {
    //stopAutomaticOperations();  // Ensure automatic operations are stopped
    resetStates();  // Reset any automatic states if needed
    currentMode = MANUAL;
    Serial.println("Switched to Manual Mode");
    setupManual();  // Re-initialize manual mode settings
}

void stopMoving() {
    digitalWrite(STEPPER1_STEP_PIN, LOW);
    digitalWrite(STEPPER2_STEP_PIN, LOW);
    digitalWrite(STEPPER_EN, HIGH);  // Ensure motor drivers are disabled
}

void handleRoot() {
    if (server.hasArg("command")) {
        String command = server.arg("command");
        if (command == "switch_to_auto") {
            switchToAutomatic();
            //currentMode = AUTOMATIC;
            Serial.println("Switched to Automatic Mode2");
        } else if (command == "switch_to_manual") {
            switchToManual();
            //currentMode = MANUAL;
            Serial.println("Switched to Manual Mode");
        } else if (command == "stop") {
            currentMode = STOP;
            stopMoving();
        } else {
            if (currentMode == MANUAL) {
                handleManualCommand(command);
            } // Consider else if for AUTOMATIC if needed
        }
        server.send(200, "text/plain", "Command received: " + command);
    } else {
        server.send(200, "text/plain", "No command received");
    }
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("Connected to WiFi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());


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
    // step1.setTargetSpeedRad(19.0);
    // step2.setTargetSpeedRad(19.0);

    // // Initialize motor control pins
    // pinMode(STEPPER1_DIR_PIN, OUTPUT);
    // pinMode(STEPPER1_STEP_PIN, OUTPUT);
    // pinMode(STEPPER2_DIR_PIN, OUTPUT);
    // pinMode(STEPPER2_STEP_PIN, OUTPUT);
    // pinMode(STEPPER_EN, OUTPUT);
    // digitalWrite(STEPPER_EN, LOW); // Enable the stepper motor drivers

  
    //setupAutomatic();  // Initialize automatic mode

    server.on("/", HTTP_GET, handleRoot);
    server.begin();
    Serial.println("Web server started");
}


void loop() {
    server.handleClient();  // Handle web requests

    switch (currentMode) {
        case AUTOMATIC:
            pinMode(STEPPER_EN, OUTPUT);
            digitalWrite(STEPPER_EN, false);
            loopAutomatic();
            break;
        case MANUAL:
            setupManual();  // Initialize manual mode
            loopManual();
            break;
        case STOP:
            stopMoving();
            break;
        default:
            // Optionally handle any undefined mode
            Serial.println("Undefined mode");
            break;
    }
    
}





