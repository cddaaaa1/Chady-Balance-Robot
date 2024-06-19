import numpy as np
from picamera2 import Picamera2, Preview
from time import sleep

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

def send_data_packet(x, y):
    # adding codes to send packets to esp32
    print(f"Sending packet with x: {x}, y: {y}")

picam2 = Picamera2()
config = picam2.create_preview_configuration(main={"size": (320, 240)})
picam2.configure(config)
picam2.start()

while True:
    frame = picam2.capture_array()
    gray = np.dot(frame[...,:3], [0.299, 0.587, 0.114])  # turn into grayscale
    binary = np.where(gray < 60, 255, 0).astype(np.uint8)  # turn to black and white image

    # simple line tracking algorithm
    lines = []
    for i in range(binary.shape[0]):
        if np.any(binary[i, :]):
            indices = np.where(binary[i, :] == 255)[0]
            line_center = np.mean(indices)
            lines.append((i, line_center))

    if lines:
        # calculate the mean
        line_y, line_x = zip(*lines)
        avg_x = np.mean(line_x)
        rho_err = avg_x - binary.shape[1] / 2

        if len(lines) > 1:
            theta_err = np.arctan2(line_y[-1] - line_y[0], line_x[-1] - line_x[0])
        else:
            theta_err = 0

        # reset integral
        rho_pid.integral = 0
        theta_pid.integral = 0

        if abs(rho_err) > 4:
            rho_output = rho_pid.get_pid(rho_err, 1)
            theta_output = theta_pid.get_pid(theta_err, 1)
            output = rho_output + theta_output
            print(f"rho_output: {rho_output}, theta_output: {theta_output}")

            send_data_packet(int(rho_output * 100), int(theta_output * 100))
        else:
            print("Line detected but magnitude too low, stopping.")
            send_data_packet(0, -1)
    else:
        print("No line found.")
        send_data_packet(-1, 0)

    sleep(0.1)  # control frequency


picam2.stop()
