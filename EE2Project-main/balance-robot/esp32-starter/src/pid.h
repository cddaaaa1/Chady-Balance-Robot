#ifndef PID_H
#define PID_H

#include "config.h"

float titltAngle(sensors_event_t a, sensors_event_t g)
{
    const float alpha = 0.98;   // Complementary filter coefficient
    static float theta = 0.0;   // current tilt angle
    static float theta_a = 0.0; // angle measured by accelerometer
    static float theta_g = 0.0; // angle measured by gyroscope

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
    const float alpha_yaw = 0.98;
    static float yaw_angle = 0;
    static float drift_summation = 0;
    static float average_drift = 0;
    static unsigned long sample_count = 1;
    yaw_angle += g.gyro.x * (LOOP_INTERVAL_INNER / 1000); // adjust dt into second
    yaw_angle = alpha_yaw * (previous_yaw + g.gyro.x * LOOP_INTERVAL_INNER / 1000) + (1 - alpha_yaw) * yaw_angle;
    if (yaw_angle > 360)
    {
        yaw_angle -= 360;
    }
    if (yaw_angle < -360)
    {
        yaw_angle += 360;
    }
    gyro_x = g.gyro.x;
    if (abs(g.gyro.x) < 0.02)
    {
        drift_summation += (yaw_angle - previous_yaw);
        average_drift = drift_summation / sample_count;
        sample_count++;
        yaw_angle -= average_drift;
    }
    else
    {
        drift_summation = 0;
        sample_count = 1;
        average_drift = 0;
    }
    previous_yaw = yaw_angle;
    return yaw_angle;
}

float vertical(float angle_input, float gyro_y)
{
    static float output;
    output = (angle_input - pitch) * vertical_kp - gyro_y * vertical_kd;
    return output;
}

float velocity(float step1_velocity, float step2_velocity)
{
    static float output;
    static float velocity_err;
    static float velocity_err_last;
    const float a = 0.7; // low pass filter coefficient
    static float velocity_err_integ;

    velocity_err = velocity_input - (step1_velocity + step2_velocity) / 2;
    velocity_err = (1 - a) * velocity_err + a * velocity_err_last;
    velocity_err_last = velocity_err;

    if (velocity_err_integ > 0.1)
    {
        velocity_err_integ = 0.1;
    }
    if (velocity_err_integ < -0.1)
    {
        velocity_err_integ = 1;
    }
    if (pitch > 0.5 || pitch < -0.5)
    {
        velocity_err_integ = 0;
    }
    output = velocity_kp * velocity_err + velocity_ki * velocity_err_integ;
    return output;
}

float turn(float gyro_x, float yaw)
{
    static float output;
    if (tracking)
    {
        //  tracking
        output = (cam_rho + cam_theta) * camera_kp + gyro_x * camera_kd;
    }
    else
    {
        if (continue_turning)
        {
            output = turn_direction * turn_speed;
        }
        else
        {
            output = (target_angle - yaw) * turn_kp - gyro_x * turn_kd;
        }
    }

    return output;
}

#endif // PID_H
