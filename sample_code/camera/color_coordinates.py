from picamera2 import Picamera2
import numpy as np

# initialize Picamera2
picam2 = Picamera2()
preview_config = picam2.create_preview_configuration()
picam2.configure(preview_config)
picam2.start()

# red threshold
RED_MIN = (100, 20, 20)
RED_MAX = (255, 100, 100)

def is_red(pixel):
    """check whether red object exists"""
    r, g, b = pixel
    return r >= RED_MIN[0] and g <= RED_MAX[1] and b <= RED_MAX[2]

def find_max_blob(frame):
    """find the largest red part"""
    red_points = np.where(np.all((frame >= RED_MIN) & (frame <= RED_MAX), axis=-1))
    if red_points[0].size > 0:
        y_center = np.mean(red_points[0])
        x_center = np.mean(red_points[1])
        return (x_center, y_center)
    return None

try:
    while True:
        # turn the image into NumPy array
        frame = picam2.capture_array()
        
        # find the center of the object
        blob_center = find_max_blob(frame[..., :3])
        if blob_center:
            screen_center = (frame.shape[1] / 2, frame.shape[0] / 2)
            x_offset = blob_center[0] - screen_center[0]
            h_offset = blob center[1] - screen_center[1]

            # output the offset of X and H
            print(f"X offset: {x_offset}, H offset: {h_offset}")
        else:
            print("No significant red blob found")

except KeyboardInterrupt:
    picam2.stop()
