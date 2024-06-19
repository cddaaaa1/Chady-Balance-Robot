from flask import Flask, request, jsonify
import requests
import numpy as np
import cv2
from picamera2 import Picamera2
from time import sleep, time
import time
import threading
import spidev
import RPi.GPIO as GPIO
import os

ESP32_IP = '172.20.10.10'  # Replace with your ESP32 IP
ESP32_PORT = 80  # HTTP server default port

current_color = None

app = Flask(__name__)

# MCP3208 pin connections
cs_pin = 8  # GPIO8 (Pin 24 on the Raspberry Pi) - Chip Select (SS)
miso_pin = 9  # GPIO9 (Pin 21 on the Raspberry Pi) - Master In Slave Out (MISO)
mosi_pin = 10  # GPIO10 (Pin 19 on the Raspberry Pi) - Master Out Slave In (MOSI)
clk_pin = 11  # GPIO11 (Pin 23 on the Raspberry Pi) - Serial Clock (SCK)

# Define MCP3208 channels for each terminal
VOM_CHANNEL = 1
VOL_CHANNEL = 2
VB_CHANNEL = 0
VL_CHANNEL = 3

# Constants
MAX_ADC = 5.0
MAX_12bits = 4095
R = 0.01  # 10mΩ resistor
VtoI_motor = 1  # voltage to current ratio
VtoI_logic = 1  # voltage to current ratio
PD_motor = 5.0  # Potential divider of Vm
PD_logic = 2.0  # Potential divider of Vl
PD_Vb = 4.0  # Potential divider of Vb

numPoints = 29
capacity_discharged = [0, 2.5, 5, 7.5, 10, 12.5, 15, 17.5, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 82.5, 85, 87.5, 90, 92.5, 95, 97.5, 100]
voltage = [16.8, 16.525, 16.25, 16.0725, 15.93, 15.8, 15.63, 15.5275, 15.4, 15.25, 15.13, 15.035, 14.94, 14.845, 14.775, 14.7275, 14.68, 14.6325, 14.585, 14.5375, 14.4, 14.3, 14.2425, 14.15, 13.98, 13.8, 13.6, 12.9975, 10]
soc = 0  # State of Charge (%)
lastUpdateTime = 0  # To track time intervals
accumulatedCharge = 0.0  # mAh
BATTERY_CAPACITY = 2000  # Battery capacity in mAh
CAPACITY_FILE = 'battery_capacity.txt'  # File to store battery capacity

# Battery variables
chargeSoc = 0
PM = 0
PL = 0

# Initialize SPI
spi = spidev.SpiDev()
spi.open(0, 0)
spi.max_speed_hz = 100000

# Initialize GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(cs_pin, GPIO.OUT)
GPIO.output(cs_pin, GPIO.HIGH)

class PID:
    def __init__(self, p, i, d):
        self.p = p
        self.i = i
        self.d = d
        self.prev_error = 0
        self.integral = 0

    def get_pid(self, error, dt):
        self.integral += error * dt
        derivative = (error - self.prev_error) / dt
        output = self.p * error + self.i * self.integral + self.d * derivative
        self.prev_error = error
        return output

rho_pid = PID(p=1.1, i=0.6, d=0.08)
theta_pid = PID(p=0.001, i=0.1, d=0.01)

class BatteryData:
    def __init__(self):
        self._lock = threading.Lock()
        self.chargeSoc = 0
        self.PM = 0
        self.PL = 0

    def update(self, chargeSoc, PM, PL):
        with self._lock:
            self.chargeSoc = chargeSoc
            self.PM = PM
            self.PL = PL

    def get_data(self):
        with self._lock:
            return self.chargeSoc, self.PM, self.PL

def send_data_packet(x, y, ip, port):
    url = f"http://{ip}:{port}/camera"
    payload = {'x': x, 'y': y}
    try:
        response = requests.post(url, data=payload)
        print(f"Sent packet with x: {x}, y: {y}, response: {response.status_code}")
    except requests.RequestException as e:
        print(f"Error sending packet: {e}")

# Initialize Picamera2
picam2 = Picamera2()
preview_config = picam2.create_preview_configuration()
picam2.configure(preview_config)
picam2.start()

def read_voltage(channel):
    if channel < 0 or channel > 7:
        return -1  # Invalid channel
    
    # Perform SPI transaction and store the returned bits
    GPIO.output(cs_pin, GPIO.LOW)
    adc = spi.xfer2([0x06 | ((channel & 0x04) >> 2), (channel & 0x03) << 6, 0x00])
    GPIO.output(cs_pin, GPIO.HIGH)
    
    # Combine the three bytes to get the result
    adc_out = ((adc[1] & 0x0F) << 8) | adc[2]
    voltage = (adc_out / MAX_12bits) * MAX_ADC
    return voltage

def voltage_SOC(v_battery):
    if v_battery >= voltage[0]:
        return capacity_discharged[0]
    if v_battery <= voltage[numPoints - 1]:
        return capacity_discharged[numPoints - 1]
    
    for i in range(numPoints - 1):
        if v_battery <= voltage[i] and v_battery > voltage[i + 1]:
            soc = capacity_discharged[i] + (capacity_discharged[i + 1] - capacity_discharged[i]) * (v_battery - voltage[i]) / (voltage[i + 1] - voltage[i])
            return soc
    return 0  # Should never reach here

def read_stored_capacity():
    if os.path.exists(CAPACITY_FILE):
        with open(CAPACITY_FILE, 'r') as file:
            return float(file.read())
    return BATTERY_CAPACITY

def store_capacity(capacity):
    with open(CAPACITY_FILE, 'w') as file:
        file.write(str(capacity))

def reset_to_full_capacity():
    global accumulatedCharge, soc
    accumulatedCharge = 0
    soc = 100
    store_capacity(accumulatedCharge)
    print("Battery capacity reset to 100%")

def reset_based_on_voltage():
    global accumulatedCharge, soc
    Vb = read_voltage(VB_CHANNEL) * PD_Vb
    soc = 100 - voltage_SOC(Vb)
    voltage_SOC(Vb)
    accumulatedCharge = (1 - (soc / 100)) * BATTERY_CAPACITY
    store_capacity(accumulatedCharge)
    print(f"Battery capacity reset based on voltage to {soc}%")

battery_data = BatteryData()

def battery_monitoring():
    global accumulatedCharge, soc, PM, PL, chargeSoc
    initialVb = 0
    while initialVb <= 13.0:
        initialVb = read_voltage(VB_CHANNEL) * PD_Vb
        if initialVb <= 13.0:
            print("Voltage not detected or low battery, retrying....")
            time.sleep(1)

    stored_capacity = read_stored_capacity()
    accumulatedCharge = (1 - (soc / 100)) * stored_capacity  # Initial accumulated charge
    lastUpdateTime = time.time()

    try:
        while True:
            VOM = read_voltage(VOM_CHANNEL)
            VOL = read_voltage(VOL_CHANNEL)
            Vb = read_voltage(VB_CHANNEL) * PD_Vb
            VL = read_voltage(VL_CHANNEL) * PD_logic

            IM = VOM * VtoI_motor  # Motor current
            VM = Vb - R * IM #Motor voltage from battery voltage
            IL = VOL * VtoI_logic  # Logic current
            PM = IM * VM  # Motor power
            PL = IL * VL  # Logic power

            currentTime = time.time()
            elapsedTime = currentTime - lastUpdateTime
            lastUpdateTime = currentTime

            I_bat = IM + (IL * (VL / Vb)) * 0.93 
            Ibat_mA = I_bat * 1000.0  # Convert to mA
            accumulatedCharge += (Ibat_mA * (elapsedTime / 3600.0))  # Charge in mAh

            chargeSoc = 100 - ((accumulatedCharge / BATTERY_CAPACITY) * 100.0)
            print(f"Charge SOC: {chargeSoc}")

            battery_data.update(chargeSoc, PM, PL)

            time.sleep(1)  # Delay for 1 second before next measurement

    except KeyboardInterrupt:
        pass

    finally:
        store_capacity(accumulatedCharge)
        spi.close()
        GPIO.cleanup()

def color_detection(img, color_name):
    """Detect the specified color in the image and return its percentage."""
    hsv_img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    
    hist = cv2.calcHist([hsv_img], [0, 1], None, [180, 256], [0, 180, 0, 256])

    lower_bound, upper_bound = get_color_threshold(color_name)

    mask = cv2.inRange(hsv_img, lower_bound, upper_bound)

    dst = cv2.calcBackProject([hsv_img], [0, 1], hist, [0, 180, 0, 256], 1)

    res = cv2.bitwise_and(dst, dst, mask=mask)

    cv2.normalize(res, res, 0, 255, cv2.NORM_MINMAX)

    _, thresholded = cv2.threshold(res, 50, 255, 0)

    kernel = np.ones((5, 5), np.uint8)
    opening = cv2.morphologyEx(thresholded, cv2.MORPH_OPEN, kernel, iterations=2)
    closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel, iterations=3)

    contours, _ = cv2.findContours(closing, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    output = cv2.drawContours(img.copy(), contours, -1, (0, 0, 255), 3)

    total_pixels = np.prod(mask.shape[:2])
    color_pixels = cv2.countNonZero(closing)
    color_percentage = (color_pixels / total_pixels) * 100
    return output, color_percentage

def get_color_threshold(color_name):
    """Return the HSV color thresholds for a given color name."""
    color_thresholds = {
        "red": ((110, 100, 100), (130, 255, 255)),
        "yellow": ((90, 100, 100), (100, 255, 255)),
        "blue": ((0, 100, 100), (10, 255, 255)),
        "green": ((35, 25, 25), (86, 255, 255)),
        "purple": ((130, 43, 64), (170, 255, 255))
    }
    return color_thresholds.get(color_name.lower())

def color_detection_thread():
    global current_color
    while True:
        if current_color:
            frame = picam2.capture_array()
            picture, color_percentage = color_detection(frame, current_color)
            print(f"{current_color.capitalize()} percentage: {color_percentage:.2f}%")

            if color_percentage >= 25:
                print("Color threshold exceeded, sending stop command to ESP32.")
                try:
                    response = requests.post(f'http://{ESP32_IP}/color', data={'ifFound': 'true'}, timeout=5)
                    print(f"ESP32 response: {response.text}")
                except requests.RequestException as e:
                    print(f"Error sending command to ESP32: {e}")
                current_color = None  # Reset current color to stop checking
            else:
                # Send false if color not found
                try:
                    response = requests.post(f'http://{ESP32_IP}/color', data={'ifFound': 'false'}, timeout=5)
                    print(f"ESP32 response: {response.text}")
                except requests.RequestException as e:
                    print(f"Error sending status update to ESP32: {e}")

            time.sleep(0.1)  

def path_finding_thread():
    while True:
        frame = picam2.capture_array()
        gray = np.dot(frame[...,:3], [0.299, 0.587, 0.114])  # Convert to grayscale
        binary = np.where(gray < 60, 255, 0).astype(np.uint8)  # Binarize image

        lines = []
        for i in range(binary.shape[0]):
            if np.any(binary[i, :]):
                indices = np.where(binary[i, :] == 255)[0]
                line_center = np.mean(indices)
                lines.append((i, line_center))

        if lines:
            # calculate average line position
            line_y, line_x = zip(*lines)
            avg_x = np.mean(line_x)
            rho_err = avg_x - binary.shape[1] / 2

            if len(lines) > 1:
                theta_err = np.arctan2(line_y[-1] - line_y[0], line_x[-1] - line_x[0])
            else:
                theta_err = 0

            rho_pid.integral = 0
            theta_pid.integral = 0

            if abs(rho_err) > 0:
                rho_output = rho_pid.get_pid(rho_err, 1)
                theta_output = theta_pid.get_pid(theta_err, 1)
                print(f"rho_output: {rho_output}, theta_output: {theta_output}")

                send_data_packet(float((rho_output + 20) * 10), float(theta_output * 10-1.6), ESP32_IP, ESP32_PORT)
            else:
                print("Line detected but magnitude too low, stopping.")
                send_data_packet(0, 0, ESP32_IP, ESP32_PORT)
        else:
            print("No line found.")
            send_data_packet(0, 0, ESP32_IP, ESP32_PORT)

        time.sleep(0.1) 

@app.route('/send_command', methods=['POST'])
def send_command():
    if 'command' in request.json:
        command = request.json['command']
        try:
            response = requests.post(f'http://{ESP32_IP}/controller', data={'cmd': command}, timeout=5)
            return response.text
        except requests.RequestException as e:
            return jsonify({'error': str(e)}), 500
    else:
        return jsonify({'error': 'No command provided'}), 400


@app.route('/set_variable', methods=['POST'])
def set_variable():
    if 'variable' in request.json and 'value' in request.json:
        variable = request.json['variable']
        value = request.json['value']
        try:
            response = requests.post(f'http://{ESP32_IP}/setVariable', data={'variable': variable, 'value': value}, timeout=5)
            return response.text
        except requests.RequestException as e:
            return jsonify({'error': str(e)}), 500
    else:
        return jsonify({'error': 'Variable or value not provided'}), 400

@app.route('/data', methods=['GET'])
def get_data():
    try:
        response = requests.get(f'http://{ESP32_IP}/data', timeout=5)
        
        if response.status_code == 200:
            return jsonify(response.json())
        return 'Failed to retrieve data', 500
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/get_variables', methods=['GET'])
def get_variables():
    try:
        response = requests.get(f'http://{ESP32_IP}/getVariables', timeout=5)
        if response.status_code == 200:
            return jsonify(response.json())
        return 'Failed to retrieve variables', 500
    except requests.RequestException as e:
        return jsonify({'error': str(e)}), 500

@app.route('/color', methods=['POST'])
def color():
    global current_color
    if 'color' in request.json:
        color = request.json['color']
        current_color = color 
        return jsonify({'status': 'Color detection started', 'color': color}), 200
    else:
        return jsonify({'error': 'No color provided'}), 400

@app.route('/battery', methods=['GET'])
def get_battery_data():
    chargeSoc, PM, PL = battery_data.get_data()
    return jsonify({'chargeSoc': chargeSoc, 'PM': PM, 'PL': PL})


@app.route('/reset_battery', methods=['POST'])
def reset_battery():
    action = request.json.get('action')
    if action == 'reset_to_full':
        reset_to_full_capacity()
    elif action == 'reset_based_on_voltage':
        reset_based_on_voltage()
    else:
        return jsonify({'error': 'Invalid action'}), 400
    return jsonify({'status': 'success'}), 200


if __name__ == '__main__':
    # color detection thread
    color_thread = threading.Thread(target=color_detection_thread)
    color_thread.daemon = True

    # path-finding thread
    path_thread = threading.Thread(target=path_finding_thread)
    path_thread.daemon = True

    # battery thread
    battery_thread = threading.Thread(target=battery_monitoring)
    battery_thread.daemon = True

    server_thread = threading.Thread(target=lambda: app.run(host='0.0.0.0', port=8000))

    server_thread.start()
    color_thread.start()
    path_thread.start()
    battery_thread.start()

    server_thread.join()
    color_thread.join()
    path_thread.join() 
    battery_thread.join()




