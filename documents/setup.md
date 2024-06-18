# Project Setup Instructions

## Prerequisites

Before you start, ensure you have the following software installed on your local machine:

- [Node.js and npm](https://nodejs.org/) (for Frontend setup)
- [Python 3.x](https://www.python.org/downloads/) (for Raspberry Pi and Server setup)
- [PlatformIO](https://platformio.org/install) (for ESP32 setup)

Make sure all devices (ESP32, Raspberry Pi, and your local machine) are connected to the same WiFi network.


## 1. ESP32 Setup with PlatformIO

1. **Install PlatformIO:**
   - Follow the instructions [here](https://platformio.org/install) to install PlatformIO.

2. **Set Up Your PlatformIO Project:**
   - Open Visual Studio Code (VSCode).
   - Install the PlatformIO extension from the VSCode marketplace.
   - 

3. **Configure PlatformIO:**
   - Open the `platformio.ini` file in the root of your project.
   - Ensure it has the correct configurations for your ESP32. It should look something like this:
     ```ini
     [env:esp32dev]
     platform = espressif32
     board = esp32dev
     framework = arduino
     monitor_speed = 115200
     ```
4. **Build and Upload the Code:**
   - Connect your ESP32 to your computer via USB.

## 2. Raspberry Pi Setup

1. **Connect to Raspberry Pi via SSH:**
   - Open a terminal on your local machine and connect to the same Wi-Fi with ESP32 to read the IP address of the raspberry-pi(open the terminal on Raspberry Pi desktop and run the command).
     ```sh
     hostname -I
     ```    
   - Update the `ESP32_IP` variable with the IP address of your real ESP32
   - Navigate to "raspi" directory and send file `all.py` to Raspberry Pi using:
     ```sh
     scp all.py chady2@<Raspberry_Pi_IP>:/home/chady2/
     ```
   - SSH into the Raspberry Pi using:
     ```sh
     ssh chady2@<Raspberry_Pi_IP>
     ```

2. **Running the file:**
   - Run the script to start reading data from ESP32, make sure the IP address is correctly modified:
     ```sh
     python3 all.py
     ```

## 3. Server Setup

1. **Install Python Dependencies:**
   - Navigate to the directory "server" containing `app.py` file.
   - Install the required packages from `requirements.txt`:
     ```sh
     pip install -r requirements.txt
     ```

2. **Update Raspberry Pi IP and PORT in Server File:**
   - Open the `app.py` file.
   - Update the `RASPBERRY_PI_IP` variable to the real IP in the server file.
   - Start the server:
     ```sh
     python3 app.py
     ```

## 4. Frontend Setup

1. **Install Node.js Dependencies:**
   - Navigate to the directory "frontend" containing your `App.js` file.
   - Install the required packages:
     ```sh
     npm install
     ```

2. **Update Server IP and PORT in Frontend:**
   - Open the `App.js` file.
   - Update the `SERVER_IP` variable with the IP address and port of your server.

3. **Start the Frontend Application:**
   - To run the application locally:
     ```sh
     npm start
     ```
   - The locol host will be opened automatically or connect to the WiFi before and type the http address to access the webpage
   - Ensure the application is connected to the same WiFi network.

## General Notes

- Make sure all parts (ESP32, Raspberry Pi, Server, and Frontend) are on the same WiFi network.
- Double-check IP addresses and port numbers to ensure proper communication between components.
- Refer to individual documentation for each component for more detailed troubleshooting and configuration options.