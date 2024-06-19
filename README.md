# Chady-Balance-Robot

# Project Folder Structure

```plaintext
project-root/
├── esp32/
│   ├── src/
|   |   ├── config.h       # Global variables and setup
|   |   ├── internet.h     # Internet functions 
|   |   ├── pid.h          # PID control functions
|   |   ├── step.h         # Stepper motor functions
|   |   ├── utils.h        # Utility functions
│   │   └── main.cpp       # Main loop
│   └── platformio.ini     # PlatformIO configuration file
├── raspi/
│   ├── all.py             # File running on Raspberry Pi
│   └── requirements.txt   # Python dependencies for Raspberry Pi
├── server/
│   ├── app.py             # File running on the server
│   └── requirements.txt   # Python dependencies for the server
├── frontend/
│   ├── node_modules/      # Directory for installed Node.js modules
│   ├── public/
│   │   └── index.html     # Main HTML file
│   ├── src/
│   │   ├── App.js         # Main React component for running
│   │   ├── index.css      
│   │   ├── index.js       
│   │   ├── Logo.png       # Logo image
│   ├── package-lock.json  # npm lock file
│   └── package.json       # npm configuration file for installing
<<<<<<< HEAD
├── sample_code/
│   ├── power/
│   └── camera/
│   └── buzzer/
├── testing_tool/
│   ├── power/
│   └── camera/
│   └── buzzer/
├── README.md                     
```
