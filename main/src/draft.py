import matplotlib.pyplot as plt
import numpy as np

# Create a figure and a subplot with a polar projection
fig, ax = plt.subplots(subplot_kw={'projection': 'polar'})

# Define the yaw angle in degrees
yaw_angle = 45  # Example yaw angle, you can replace this with the actual value

# Convert the yaw angle to radians
yaw_angle_rad = np.deg2rad(yaw_angle)

# Clear the plot
ax.clear()

# Plot a circle to represent the compass
ax.set_ylim(0, 1)
ax.set_yticklabels([])  # Hide the radial ticks

# Plot the arrow representing the yaw angle
ax.arrow(yaw_angle_rad, 0, 0, 1, head_width=0.1, head_length=0.2, fc='purple', ec='purple')

# Display the yaw angle as text
ax.text(yaw_angle_rad, 1.1, f"Yaw: {yaw_angle}°", horizontalalignment='center', verticalalignment='center', color='black')

# Set the title
ax.set_title('Yaw Angle (Compass)')

# Show the plot
plt.show()