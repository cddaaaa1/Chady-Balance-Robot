from flask import Flask, request, jsonify
import requests
import numpy as np
import cv2
from picamera2 import Picamera2
from time import sleep
import threading

ESP32_IP = '172.20.10.8'  # Replace with your ESP32 IP
ESP32_PORT = 80  # HTTP server default port

current_color = None

app = Flask(__name__)

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

def color_detection(img, color_name):
    """Detect the specified color in the image and return its percentage."""
    hsv_img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    
    # Create color histogram
    hist = cv2.calcHist([hsv_img], [0, 1], None, [180, 256], [0, 180, 0, 256])

    # Get color threshold
    lower_bound, upper_bound = get_color_threshold(color_name)

    # Create mask
    mask = cv2.inRange(hsv_img, lower_bound, upper_bound)

    # Back project histogram
    dst = cv2.calcBackProject([hsv_img], [0, 1], hist, [0, 180, 0, 256], 1)

    # Apply mask
    res = cv2.bitwise_and(dst, dst, mask=mask)

    # Normalize
    cv2.normalize(res, res, 0, 255, cv2.NORM_MINMAX)

    # Thresholding
    _, thresholded = cv2.threshold(res, 50, 255, 0)

    # Morphological operations
    kernel = np.ones((5, 5), np.uint8)
    opening = cv2.morphologyEx(thresholded, cv2.MORPH_OPEN, kernel, iterations=2)
    closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel, iterations=3)

    # Find contours
    contours, _ = cv2.findContours(closing, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    output = cv2.drawContours(img.copy(), contours, -1, (0, 0, 255), 3)

    # Calculate color percentage
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

            sleep(0.1)  # Add a short delay to reduce CPU usage

def path_finding_thread():
    while True:
        frame = picam2.capture_array()
        gray = np.dot(frame[...,:3], [0.299, 0.587, 0.114])  # Convert to grayscale
        binary = np.where(gray < 60, 255, 0).astype(np.uint8)  # Binarize image

        # Simple line detection algorithm
        lines = []
        for i in range(binary.shape[0]):
            if np.any(binary[i, :]):
                indices = np.where(binary[i, :] == 255)[0]
                line_center = np.mean(indices)
                lines.append((i, line_center))

        if lines:
            # Calculate average line position
            line_y, line_x = zip(*lines)
            avg_x = np.mean(line_x)
            rho_err = avg_x - binary.shape[1] / 2

            if len(lines) > 1:
                theta_err = np.arctan2(line_y[-1] - line_y[0], line_x[-1] - line_x[0])
            else:
                theta_err = 0

            # Reset integral term
            rho_pid.integral = 0
            theta_pid.integral = 0

            if abs(rho_err) > 4:
                rho_output = rho_pid.get_pid(rho_err, 1)
                theta_output = theta_pid.get_pid(theta_err, 1)
                print(f"rho_output: {rho_output}, theta_output: {theta_output}")

                send_data_packet(float((rho_output - 220) * 10), float(theta_output * 10-1.6), ESP32_IP, ESP32_PORT)
            else:
                print("Line detected but magnitude too low, stopping.")
                send_data_packet(0, -1, ESP32_IP, ESP32_PORT)
        else:
            print("No line found.")
            send_data_packet(-1, 0, ESP32_IP, ESP32_PORT)

        sleep(0.1)  # Control frame rate

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

if __name__ == '__main__':
    # Start the color detection thread
    color_thread = threading.Thread(target=color_detection_thread)
    color_thread.daemon = True

    # Start the path-finding thread
    path_thread = threading.Thread(target=path_finding_thread)
    path_thread.daemon = True

    server_thread = threading.Thread(target=lambda: app.run(host='0.0.0.0', port=8000))

    server_thread.start()
    color_thread.start()
    path_thread.start()

    server_thread.join()
    color_thread.join()
    path_thread.join()













