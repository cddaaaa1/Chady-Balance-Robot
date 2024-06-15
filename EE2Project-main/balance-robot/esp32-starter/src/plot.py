import requests
import time
import datetime
import tkinter as tk
from tkinter import ttk
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.ticker import MaxNLocator
import numpy as np
import struct

ESP32_IP = "192.168.0.47"  # Replace with your ESP32's IP address
DATA_URL = f"http://{ESP32_IP}/data"
SET_VARIABLE_URL = f"http://{ESP32_IP}/setVariable"
GET_VARIABLES_URL = f"http://{ESP32_IP}/getVariables"

# Initialize data lists
timestamps = []
pitch = []
velocity = []
target_speed_values = []
yaw = []

# List of variables to select from
variables = [
    "vertical_kp", "vertical_kd", "velocity_kp", "velocity_ki",
    "turn_kp", "turn_kd",  "camera_kp", "camera_kd", 
    "target_velocity", "target_angle","bias","yaw_bias","tracking","color_detected","back_to_track",
]

# Flag for square wave generation
square_wave_active = False
square_wave_value = 0
square_wave_interval = 50000  # in milliseconds

def get_sensor_data():
    try:
        response = requests.get(DATA_URL)
        if response.status_code == 200:
            binary_data = response.content
            # Unpack the binary data
            pitch, velocity, yaw = struct.unpack('fff', binary_data)
            return {'pitch': pitch, 'velocity': velocity, 'yaw': yaw}
        else:
            print(f"Failed to retrieve data, status code: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None

def update_data(frame):
    global square_wave_value, square_wave_active
    data = get_sensor_data()
    if data:
        current_time = datetime.datetime.now().strftime("%H:%M:%S.%f")[:-3]
        timestamps.append(current_time)
        pitch.append(data['pitch'])
        velocity.append(data['velocity'])
        yaw.append(data['yaw'])
        target_speed_values.append(square_wave_value if square_wave_active else None)
        
        # Keep the last 20 data points
        if len(timestamps) > 100:
            timestamps.pop(0)
            pitch.pop(0)
            velocity.pop(0)
            yaw.pop(0)
            target_speed_values.pop(0)
        
        ax1.clear()
        ax2.clear()
        
        ax1.plot(timestamps, pitch, label='pitch (radius)', color='r')
        ax2.plot(timestamps, velocity, label='velocity (m/s)', color='b')
        
        if any(target_speed_values):
            ax2.plot(timestamps, target_speed_values, label='target_speed (m/s)', color='g')
        
        ax1.set_title('Real-time pitch')
        ax2.set_title('Real-time velocity')
        
        ax1.set_xlabel('Time')
        ax1.set_ylabel('pitch (radius)')
        ax2.set_xlabel('Time')
        ax2.set_ylabel('velocity (m/s)')
        
        ax1.xaxis.set_major_locator(MaxNLocator(nbins=10))  # nbins specifies the maximum number of ticks
        ax2.xaxis.set_major_locator(MaxNLocator(nbins=10))

        plt.setp(ax1.xaxis.get_majorticklabels(), rotation=45)
        plt.setp(ax2.xaxis.get_majorticklabels(), rotation=45)

        ax1.legend(loc='upper left')
        ax2.legend(loc='upper left')

         # Update the compass for yaw
        compass_ax.clear()
        yaw_angle_rad = data['yaw']
        yaw_angle = np.rad2deg(yaw_angle_rad)
        compass_ax.set_ylim(0, 1)
        compass_ax.set_yticklabels([])  # Hide the radial ticks
        compass_ax.arrow(yaw_angle_rad, 0, 0, 1, head_width=0.1, head_length=0.2, fc='purple', ec='purple')
        compass_ax.text(yaw_angle_rad, 1.1, f"Yaw: {yaw_angle}°", horizontalalignment='center', verticalalignment='center', color='black')
        compass_ax.set_title('Yaw Angle (Compass)')

        canvas.draw()
        compass_canvas.draw()  # Add this line to update the compass canvas

def clear_graph():
    global timestamps, pitch, velocity, yaw, target_speed_values
    timestamps, pitch, velocity, yaw, target_speed_values = [], [], [], [], []
    ax1.clear()
    ax2.clear()
    compass_ax.clear()
    ax1.set_title('Real-time pitch')
    ax2.set_title('Real-time velocity')
    compass_ax.set_title('Yaw Angle (Compass)')
    ax1.set_xlabel('Time')
    ax1.set_ylabel('pitch (radius)')
    ax2.set_xlabel('Time')
    ax2.set_ylabel('velocity (m/s)')
    compass_ax.set_ylim(0, 1)
    compass_ax.set_yticklabels([])  # Hide the radial ticks
    canvas.draw()
    compass_canvas.draw()

def set_variable(variable, value):
    try:
        payload = {'cmd': f'{variable}={value}'}
        print(f"Sending payload: {payload}")  # Debug print
        response = requests.post(SET_VARIABLE_URL, data=payload)
        print(f"Response status code: {response.status_code}, Response text: {response.text}")  # Debug print
        if response.status_code == 200:
            status_label.config(text=f"Variable {variable} set to {value}")
        else:
            status_label.config(text=f"Failed to set variable, status code: {response.status_code}")
    except Exception as e:
        status_label.config(text=f"Error: {e}")
        print(f"Error: {e}")

def set_variable_button():
    variable = variable_combobox.get()
    value = value_entry.get()
    set_variable(variable, value)

def toggle_square_wave():
    global square_wave_active
    square_wave_active = not square_wave_active
    if square_wave_active:
        square_wave_button.config(text="Stop Square Wave")
        root.after(square_wave_interval, square_wave)
    else:
        square_wave_button.config(text="Start Square Wave")

def square_wave():
    global square_wave_value
    if square_wave_active:
        square_wave_value = 3 if square_wave_value == 0 else 0
        set_variable("target_velocity", square_wave_value)
        root.after(square_wave_interval, square_wave)

def get_variables():
    try:
        response = requests.get(GET_VARIABLES_URL)
        if response.status_code == 200:
            data = response.json()
            return data
        else:
            print(f"Failed to retrieve variables, status code: {response.status_code}")
            return None
    except Exception as e:
        print(f"Error: {e}")
        return None
    
def update_variables_display():
    variables_data = get_variables()
    if variables_data:
        variables_text = "\n".join([f"{key}: {value}" for key, value in variables_data.items()])
        variables_display.config(state=tk.NORMAL)
        variables_display.delete(1.0, tk.END)
        variables_display.insert(tk.END, variables_text)
        variables_display.config(state=tk.DISABLED)

# Create the main window
root = tk.Tk()
root.title("ESP32 Control Interface")

# Create frames for layout
input_frame = ttk.Frame(root)
input_frame.grid(row=0, column=0, padx=5, pady=5, sticky="ew")

variable_frame = ttk.Frame(root)
variable_frame.grid(row=1, column=0, padx=5, pady=5, sticky="ew")

status_frame = ttk.Frame(root)
status_frame.grid(row=2, column=0, padx=5, pady=5, sticky="ew")

display_frame = ttk.Frame(root)
display_frame.grid(row=3, column=0, padx=5, pady=5, sticky="ew")

graph_frame = ttk.Frame(root)
graph_frame.grid(row=4, column=0, padx=5, pady=5, sticky="nsew")

root.grid_rowconfigure(4, weight=1)
root.grid_columnconfigure(0, weight=1)

# Create input fields for variable and value
ttk.Label(input_frame, text="Variable:").grid(row=0, column=0, padx=5, pady=5)
variable_combobox = ttk.Combobox(input_frame, values=variables)
variable_combobox.grid(row=0, column=1, padx=5, pady=5)
variable_combobox.current(0)  # Set default selection

ttk.Label(input_frame, text="Value:").grid(row=1, column=0, padx=5, pady=5)
value_entry = ttk.Entry(input_frame)
value_entry.grid(row=1, column=1, padx=5, pady=5)

# Create a button to send the command
send_button = ttk.Button(input_frame, text="Set Variable", command=set_variable_button)
send_button.grid(row=2, column=0, columnspan=2, padx=5, pady=5)

# Create a button to toggle the square wave
square_wave_button = ttk.Button(input_frame, text="Start Square Wave", command=toggle_square_wave)
square_wave_button.grid(row=3, column=0, columnspan=2, padx=5, pady=5)

# Create a button to clear the graph
clear_graph_button = ttk.Button(input_frame, text="Clear Graph", command=clear_graph)
clear_graph_button.grid(row=4, column=0, columnspan=2, padx=5, pady=5)

# Create a label to display status messages
status_label = ttk.Label(status_frame, text="")
status_label.grid(row=0, column=0, columnspan=2, padx=5, pady=5)

# Create a text widget to display the current variable values
variables_display = tk.Text(variable_frame, height=10, width=50, state=tk.DISABLED)
variables_display.grid(row=0, column=0, columnspan=2, padx=5, pady=5)

# Create a button to update the variable display
update_button = ttk.Button(variable_frame, text="Update Variables", command=update_variables_display)
update_button.grid(row=1, column=0, columnspan=2, padx=5, pady=5)

# Create subframes for the graph frame
line_graph_frame = ttk.Frame(graph_frame)
line_graph_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

compass_frame = ttk.Frame(graph_frame)
compass_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

# Create Matplotlib figure and axes for line graphs
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 6))
canvas = FigureCanvasTkAgg(fig, master=line_graph_frame)
canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)

# Create a separate figure for the compass
compass_fig, compass_ax = plt.subplots(subplot_kw={'projection': 'polar'})
compass_canvas = FigureCanvasTkAgg(compass_fig, master=compass_frame)
compass_canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)


# Create the animation
# ani = animation.FuncAnimation(fig, update_data, interval=1000)

plt.tight_layout()
root.mainloop()
