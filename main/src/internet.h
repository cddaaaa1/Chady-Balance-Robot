#ifndef INTERNET_H
#define INTERNET_H

#include "config.h"

//reset manual control states
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
    tracking = false;
    target_angle = yaw;
}

void switchToManual()
{
    resetStates();
    currentMode = AUTOTOMANUAL;
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

// Handle HTTP POST request from web controller
void handleController(AsyncWebServerRequest *request)
{
    if (request->hasArg("cmd"))
    {
        String command = request->arg("cmd");
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

//handle camera data for tracking
void handleCamera(AsyncWebServerRequest *request)
{
    if (request->hasParam("x", true))
    {
        cam_rho = request->getParam("x", true)->value().toFloat();
        // Serial.print("rho: ");
        // Serial.println(cam_rho);
    }
    if (request->hasParam("y", true))
    {
        cam_theta = request->getParam("y", true)->value().toFloat();
        Serial.print("cam_theta: ");
        Serial.println(cam_theta, 6);
    }

    // Send a response back to the client indicating data was received
    request->send(200, "text/plain", "Data received");
}

//handle color data for door detection
void handleColor(AsyncWebServerRequest *request)
{
    if (request->hasArg("ifFound"))
    {
        String ifFound = request->arg("ifFound");
        if (ifFound == "true")
        {
            color_detected = true;
            Serial.println("Color found");
        }
        else
        {
            // continue moving in the direction of the door
        }
        request->send(200, "text/plain", "Color handling completed");
    }
    else
    {
        request->send(400, "text/plain", "Bad Request");
    }
}

//handle setting variables
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
            else if (varName == "camera_kp")
            {
                camera_kp = varValue;
            }
            else if (varName == "camera_kd")
            {
                camera_kd = varValue;
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
            else if (varName == "tracking")
            {
                if (varValue == 1)
                {
                    tracking = true;
                }
                else
                {
                    tracking = false;
                }
            }
            else if (varName == "color_detected")
            {
                if (varValue == 1)
                {
                    color_detected = true;
                }
                else
                {
                    color_detected = false;
                }
            }
            else if (varName == "back_to_track")
            {
                if (varValue == 1)
                {
                    back_to_track = true;
                }
                else
                {
                    back_to_track = false;
                }
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

//handle getting variables
void handleGetVariables(AsyncWebServerRequest *request)
{
    StaticJsonDocument<500> jsonResponse;
    jsonResponse["vertical_kp"] = vertical_kp;
    jsonResponse["vertical_kd"] = vertical_kd;
    jsonResponse["velocity_kp"] = velocity_kp;
    jsonResponse["velocity_ki"] = velocity_ki;
    jsonResponse["turn_kp"] = turn_kp;
    jsonResponse["turn_kd"] = turn_kd;
    jsonResponse["camera_kp"] = camera_kp;
    jsonResponse["camera_kd"] = camera_kd;
    jsonResponse["target_velocity"] = target_velocity;
    jsonResponse["target_angle"] = target_angle;
    jsonResponse["bias"] = bias;
    jsonResponse["yaw_bias"] = yaw_bias;
    jsonResponse["tracking"] = tracking;
    jsonResponse["color_detected"] = color_detected;
    jsonResponse["back_to_track"] = back_to_track;

    String response;
    serializeJson(jsonResponse, response);

    request->send(200, "application/json", response);
}

//handle sending real time data
void handleData(AsyncWebServerRequest *request)
{
    uint8_t buffer[12];
    memcpy(buffer, &pitch, 4);         // Copy float pitch
    memcpy(buffer + 4, &velocity1, 4); // Copy float velocity1
    memcpy(buffer + 8, &yaw, 4);       // Copy float yaw

    request->send(200, "application/octet-stream", String((char *)buffer, 12));
}

//setup wifi
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

//setup server
void setupServer()
{
    server.addHandler(&events);
    server.on("/setVariable", HTTP_POST, handleSetVariable);
    server.on("/data", HTTP_GET, handleData);
    server.on("/getVariables", HTTP_GET, handleGetVariables);
    server.on("/controller", HTTP_POST, handleController);
    server.on("/color", HTTP_POST, handleColor);
    server.on("/camera", HTTP_POST, handleCamera);
    server.begin();
    Serial.println("HTTP server started");
}

#endif // INTERNET_H
