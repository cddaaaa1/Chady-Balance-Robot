#include <WiFi.h>
#include <WebServer.h>
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

enum Mode { AUTOMATIC, MANUAL };
Mode currentMode = MANUAL; // Default mode

void handleRoot() {
    if (server.hasArg("command")) {
        String command = server.arg("command"); // Correctly using String
        if (command == "switch_to_auto") {
            currentMode = AUTOMATIC;
            server.send(200, "text/plain", "Switched to Automatic Mode");
        } else if (command == "switch_to_manual") {
            currentMode = MANUAL;
            server.send(200, "text/plain", "Switched to Manual Mode");
        } else {
            // Assume handleManualCommand(command) and other functions also use String
            if (currentMode == MANUAL) {
                handleManualCommand(command);
            }
            server.send(200, "text/plain", "Command received: " + command);
        }
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

    // Initialize motor control pins
    pinMode(STEPPER1_DIR_PIN, OUTPUT);
    pinMode(STEPPER1_STEP_PIN, OUTPUT);
    pinMode(STEPPER2_DIR_PIN, OUTPUT);
    pinMode(STEPPER2_STEP_PIN, OUTPUT);
    pinMode(STEPPER_EN, OUTPUT);
    digitalWrite(STEPPER_EN, LOW); // Enable the stepper motor drivers

    setupManual();  // Initialize manual mode
    setupAutomatic();  // Initialize automatic mode

    server.on("/", HTTP_GET, handleRoot);
    server.begin();
    Serial.println("Web server started");
}

void loop() {
    server.handleClient();  // Handle web requests

    switch (currentMode) {
        case MANUAL:
            loopManual();
            break;
        case AUTOMATIC:
            loopAutomatic();
            break;
    }
}




