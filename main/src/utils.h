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

// bool ultrasonicStop()
// {
//     digitalWrite(trigPin, LOW);
//     delayMicroseconds(2);
//     digitalWrite(trigPin, HIGH);
//     last_pulse_time = micros();
//     delayMicroseconds(10);
//     digitalWrite(trigPin, LOW);
//     if (digitalRead(echoPin) == true)
//     {
//         currentMicro = micros();
//         duration = currentMicro - last_pulse_time;
//     }

//     distance = duration * 0.034 / 2;

//     if (distance < 10)
//     {
//         return true;
//     }
//     else
//     {
//         return false;
//     }
// }

void startBuzzerTask(BuzzerTask *task, unsigned long currentMillis)
{
    currentBuzzerTask = task; // Set the currentBuzzerTask pointer to the provided task

    // Reset the task state to IDLE before starting or resuming
    task->state = IDLE;

    // Now proceed to start or resume the task
    task->currentNote = 0;
    task->state = PLAYING;
    task->lastUpdateTime = currentMillis;
}

void updateBuzzerTask(BuzzerTask *task, unsigned long currentMillis)
{
    if (task->state == IDLE)
    {
        // do nothing
    }

    if (task->state == PLAYING)
    {
        if (currentMillis - task->lastUpdateTime >= task->noteDurations[task->currentNote] * 150)
        {
            tone(buzzerPin, task->melody[task->currentNote], 150);
            task->lastUpdateTime = currentMillis;
            task->currentNote++;

            if (task->currentNote >= 8)
            {
                task->state = WAITING;
                task->lastUpdateTime = currentMillis;
            }
        }
    }

    if (task->state == WAITING)
    {
        if (currentMillis - task->lastUpdateTime >= 1000)
        {
            task->state = IDLE;
        }
    }
}

bool isBuzzerTaskIdle(BuzzerTask *task)
{
    return task->state == IDLE;
}

void resetBuzzerToIdle(BuzzerTask *task)
{
    task->state == IDLE;
}

// void loopAutomatic(unsigned long currentMillis)
// {
//     if (color_detected)
//     {
//         // tracking = false;
//         Serial.println("yes");
//         target_velocity = 0;
//         // target_angle = yaw + 1.57;
//         turning = true;
//     }

//     if ((target_angle - yaw < 0.05 && target_angle - yaw > -0.05) && (gyro_x < 0.05 && gyro_x > -0.05) && color_detected) // robot might move, so color_detected might turn to false
//     {
//         // buzzer
//         color_detected = false;
//         turning = false;
//         back_to_track = true;
//     }

//     if (back_to_track && !turning && isBuzzerTaskIdle(&beat1))
//     {
//         // target_angle = yaw - 1.57;
//         turning = true;
//     }

//     if ((target_angle - yaw < 0.05 && target_angle - yaw > -0.05) && (gyro_x < 0.05 && gyro_x > -0.05) && back_to_track)
//     {
//         // tracking = true;
//         target_velocity = 1.3;
//         back_to_track = false;
//         turning = false;
//     }
// }

void loopAutomatic(unsigned long currentMillis)
{
    switch (currentState)
    {
    case MOVING_FORWARD:
        tracking = true;
        target_velocity = -1.3;
        if (color_detected)
        {
            color_detected = false;
            currentState = STOPPED;
            target_velocity = 0;
        }
        break;

    case STOPPED:
        // Code to stop the robot
        // buzzer
        startBuzzerTask(&beat1, currentMillis);
        currentState = TURNING;
        break;

    case TURNING:
        // Code to turn the robot 90 degrees
        currentState = TURNED;
        break;

    case TURNED:
        // currentBuzzerTask = &beat1;
        // startBuzzerTask(&beat1, currentMillis);
        if (isBuzzerTaskIdle(currentBuzzerTask))
        {
            currentBuzzerTask = nullptr;
            currentState = TURNING_BACK;
        }
        break;

    case TURNING_BACK:
        // Code to turn the robot back to its original direction
        currentState = MOVING_FORWARD;
        break;

    default:
        // Handle unexpected states or errors
        break;
    }
}

void loopManual()
{
    if (isForward)
    {
        target_velocity = -1.3;
        isForward = false;
    }
    else if (isBackward)
    {
        target_velocity = 1.3;
        isBackward = false;
    }
    else if (isStop)
    {
        target_velocity = 0;
        // target_angle = yaw;
        isStop = false;
    }
    else if (isLeft)
    {
        target_angle = yaw + 1;
        isLeft = false;
    }
    else if (isRight)
    {
        target_angle = yaw - 1;
        isRight = false;
    }
}

void setupSystem()
{
    Serial.begin(115200);
    pinMode(TOGGLE_PIN, OUTPUT);

    // buzzer setup
    pinMode(buzzerPin, OUTPUT);

    // ultrasonic sensor setup
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);

    // wifi and server setup
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
    static unsigned long turnTimer = 0;
    static unsigned long actionTimer = 0;

    static float vertical_output;
    static float velocity_output;
    static float turn_output;
    static float yaw_drift;
    static float current_target_angle;
    static float acc_input1;
    static float acc_input2;

    unsigned long currentMillis = millis();

    if (currentBuzzerTask != nullptr)
    {
        updateBuzzerTask(currentBuzzerTask, currentMillis);
    }

    if (currentMillis > controllerTimer)
    {
        switch (currentMode)
        {
        case AUTOMATIC:
            // pinMode(STEPPER_EN, OUTPUT);
            // digitalWrite(STEPPER_EN, false);
            loopAutomatic(currentMillis);
            break;
        case MANUAL:
            setupManual();
            loopManual();
        }
    }

    if (currentMillis > loopTimer_inner)
    {
        loopTimer_inner += LOOP_INTERVAL_INNER;
        static unsigned long previous = 0;
        unsigned long currents = millis();
        unsigned long loopTime = currents - previous;
        previous = currents;
        if (loopTime > 50)
        {
            Serial.print("Loop time: ");
            Serial.println(loopTime);
        }

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);
        pitch = titltAngle(a, g);
        yaw = yawAngle(a, g);
        turn_output = turn(g.gyro.x, yaw);

        if (currentMillis > loopTimer_outter)
        {
            loopTimer_outter += LOOP_INTERVAL_OUTER;
            // stop_flag = ultrasonicStop();
            // if (stop_flag)
            // {
            //     if (!ultrasonic_flag)
            //     {
            //         last_target_velocity = target_velocity;
            //         startBuzzerTask(&alarm1, currentMillis);
            //         Serial.println("alarm1");
            //         ultrasonic_flag = true;
            //     }
            //     target_velocity = 0;
            // }
            // else if (ultrasonic_flag)
            // {
            //     currentBuzzerTask = nullptr;
            //     target_velocity = last_target_velocity;
            //     ultrasonic_flag = false;
            // }

            velocity1 = step1.getSpeedRad();
            velcoity2 = step2.getSpeedRad();
            velocity_output = velocity(velocity1, velcoity2);
        }

        vertical_output = vertical(bias + velocity_output, g.gyro.y);

        acc_input1 = vertical_output + turn_output;
        acc_input2 = vertical_output - turn_output;

        if (pitch > 0.6 || pitch < -0.6)
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
        // test for action

        if (currentMillis > printTimer)
        {
            printTimer += PRINT_INTERVAL;
            // Serial.print("pitch: ");
            Serial.println(pitch);
            // Serial.println(color_detected);
        }
    }
}

#endif // UTILS_H
