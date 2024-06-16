#ifndef MANUAL_H
#define MANUAL_H

#include <Arduino.h>
#include <step.h>

extern bool isForward;
extern bool isBackward;
extern bool isLeft;
extern bool isRight;

// The Stepper pins
#define STEPPER1_DIR_PIN 16   //Arduino D9
#define STEPPER1_STEP_PIN 17  //Arduino D8
#define STEPPER2_DIR_PIN 4    //Arduino D11
#define STEPPER2_STEP_PIN 14  //Arduino D10
#define STEPPER_EN 15         //Arduino D12

// Diagnostic pin for oscilloscope
#define TOGGLE_PIN  32        //Arduino A4

extern step step1;
extern step step2;

// Function to set motor pins for moving forward
void moveForward() {
    digitalWrite(STEPPER1_DIR_PIN, HIGH);
    digitalWrite(STEPPER2_DIR_PIN, HIGH);
    for (int i = 0; i < 1000; i++) {
        digitalWrite(STEPPER1_STEP_PIN, HIGH);
        digitalWrite(STEPPER2_STEP_PIN, HIGH);
        delayMicroseconds(1000);
        digitalWrite(STEPPER1_STEP_PIN, LOW);
        digitalWrite(STEPPER2_STEP_PIN, LOW);
        delayMicroseconds(1000);
    }
    Serial.println("Moving forward");
}

// Function to set motor pins for moving backward
void moveBackward() {
    digitalWrite(STEPPER1_DIR_PIN, LOW);
    digitalWrite(STEPPER2_DIR_PIN, LOW);
    for (int i = 0; i < 1000; i++) {
        digitalWrite(STEPPER1_STEP_PIN, HIGH);
        digitalWrite(STEPPER2_STEP_PIN, HIGH);
        delayMicroseconds(1000);
        digitalWrite(STEPPER1_STEP_PIN, LOW);
        digitalWrite(STEPPER2_STEP_PIN, LOW);
        delayMicroseconds(1000);
    }
    Serial.println("Moving backward");
}

// Function to set motor pins for turning left
void turnLeft() {
    digitalWrite(STEPPER1_DIR_PIN, LOW);
    digitalWrite(STEPPER2_DIR_PIN, HIGH);
    for (int i = 0; i < 1000; i++) {
        digitalWrite(STEPPER1_STEP_PIN, HIGH);
        digitalWrite(STEPPER2_STEP_PIN, HIGH);
        delayMicroseconds(1000);
        digitalWrite(STEPPER1_STEP_PIN, LOW);
        digitalWrite(STEPPER2_STEP_PIN, LOW);
        delayMicroseconds(1000);
    }
    Serial.println("Turning left");
}

// Function to set motor pins for turning right
void turnRight() {
    digitalWrite(STEPPER1_DIR_PIN, HIGH);
    digitalWrite(STEPPER2_DIR_PIN, LOW);
    for (int i = 0; i < 1000; i++) {
        digitalWrite(STEPPER1_STEP_PIN, HIGH);
        digitalWrite(STEPPER2_STEP_PIN, HIGH);
        delayMicroseconds(1000);
        digitalWrite(STEPPER1_STEP_PIN, LOW);
        digitalWrite(STEPPER2_STEP_PIN, LOW);
        delayMicroseconds(1000);
    }
    Serial.println("Turning right");
}

void setupManual() {
    while(step1.getSpeedRad() != 0 && step2.getSpeedRad() != 0){
        digitalWrite(STEPPER_EN, true);
        step1.setTargetSpeedRad(0);
        step2.setTargetSpeedRad(0);
    }
    digitalWrite(STEPPER_EN, false);
    pinMode(STEPPER1_DIR_PIN, OUTPUT);
    pinMode(STEPPER1_STEP_PIN, OUTPUT);
    pinMode(STEPPER2_DIR_PIN, OUTPUT);
    pinMode(STEPPER2_STEP_PIN, OUTPUT);
    pinMode(STEPPER_EN, OUTPUT);
}

void loopManual() {
    if (isForward) moveForward();
    if (isBackward) moveBackward();
    if (isLeft) turnLeft();
    if (isRight) turnRight();
}

void handleManualCommand(String command) {
    if (command == "forward") {
        isForward = true;
        isBackward = false;
        isLeft = false;
        isRight = false;
    } else if (command == "backward") {
        isBackward = true;
        isForward = false;
        isLeft = false;
        isRight = false;
    } else if (command == "left") {
        isLeft = true;
        isForward = false;
        isBackward = false;
        isRight = false;
    } else if (command == "right") {
        isRight = true;
        isForward = false;
        isBackward = false;
        isLeft = false;
    } 
}

#endif