#ifndef INTERNET_H
#define INTERNET_H

#include "config.h"

void resetStates()
{
    isForward = false;
    isBackward = false;
    isLeft = false;
    isRight = false;
}

void switchToAutomatic()
{
    resetStates();
    currentMode = AUTOMATIC;
    Serial.println("Switched to Automatic Mode");
}

void setupManual()
{
    target_velocity = 0;
    target_angle = yaw;
}

void switchToManual()
{
    resetStates();
    currentMode = MANUAL;
    Serial.println("Switched to Manual Mode");
    setupManual();
}

void handleManualCommand(String command)
{
    if (command == "forward")
    {
        isForward = true;
        isBackward = false;
        isStop = false;
        isLeft = false;
        isRight = false;
    }
    else if (command == "backward")
    {
        isBackward = true;
        isForward = false;
        isStop = false;
        isLeft = false;
        isRight = false;
    }
    else if (command == "left")
    {
        isLeft = true;
        isForward = false;
        isStop = false;
        isBackward = false;
        isRight = false;
    }
    else if (command == "right")
    {
        isRight = true;
        isForward = false;
        isStop = false;
        isBackward = false;
        isLeft = false;
    }
    else if (command == "stop")
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
            request->send(200, "text/plain", "update variable");
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
    StaticJsonDocument<200> jsonResponse;
    jsonResponse["pitch"] = pitch;
    jsonResponse["velocity"] = velocity1;
    jsonResponse["yaw"] = yaw;

    String response;
    serializeJson(jsonResponse, response);

    request->send(200, "application/json", response);
}

void setupWifi()
{
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    Serial.println("Connected to WiFi");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
}

void setupServer()
{
    server.addHandler(&events);
    server.on("/setVariable", HTTP_POST, handleSetVariable);
    server.on("/data", HTTP_GET, handleData);
    server.on("/getVariables", HTTP_GET, handleGetVariables);
    server.on("/controller", HTTP_POST, handleController);
    server.begin();
    Serial.println("HTTP server started");
}

#endif // INTERNET_H
