#ifndef UTILS_H
#define UTILS_H

#include "config.h"
#include "internet.h"
#include "pid.h"

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

void setupSystem()
{
    Serial.begin(115200);
    pinMode(TOGGLE_PIN, OUTPUT);
    pinMode(left_track_PIN, INPUT);
    pinMode(middle_track_PIN, INPUT);
    pinMode(right_track_PIN, INPUT);

    setupWifi();
    setupServer();

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

    if (!ITimer.attachInterruptInterval(STEPPER_INTERVAL_US, TimerHandler))
    {
        Serial.println("Failed to start stepper interrupt");
        while (1)
            delay(10);
    }
    Serial.println("Initialised Interrupt for Stepper");

    step1.setAccelerationRad(10.0);
    step2.setAccelerationRad(10.0);

    pinMode(STEPPER_EN, OUTPUT);
    digitalWrite(STEPPER_EN, false);
}

void controlLoop()
{
    static unsigned long printTimer = 0;
    static unsigned long loopTimer_inner = 0;
    static unsigned long serverTimer = 0;
    static unsigned long loopTimer_outter = 0;
    static unsigned long controllerTimer = 0;

    static float vertical_output;
    static float velocity_output;
    static float turn_output;
    static float yaw_drift;
    static float current_target_angle;
    static float drift_summation = 0;
    static unsigned long sample_count = 1;
    static float average_drift;
    static float acc_input1;
    static float acc_input2;

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
            setupManual();
            loopManual();
        }
    }

    if (millis() > loopTimer_inner)
    {
        loopTimer_inner += LOOP_INTERVAL_INNER;

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

        velocity1 = step1.getSpeedRad();
        velcoity2 = step2.getSpeedRad();
        turn_output = turn(g.gyro.x, yaw);

        if (millis() > loopTimer_outter)
        {
            loopTimer_outter += LOOP_INTERVAL_OUTER;
            velocity_input = target_velocity;
            velocity_output = velocity(velocity1, velcoity2);
        }

        vertical_output = vertical(bias + velocity_output, g.gyro.y);

        acc_input1 = vertical_output + turn_output;
        acc_input2 = vertical_output - turn_output;

        if (pitch > 0.4 || pitch < -0.4)
        {
            step1.setTargetSpeedRad(0);
            step2.setTargetSpeedRad(0);
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

        if (millis() > printTimer)
        {
            printTimer += PRINT_INTERVAL;
            Serial.print("Pitch: ");
            Serial.println(pitch);
        }
    }
}

#endif // UTILS_H
