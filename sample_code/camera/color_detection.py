import cv2
from picamera2 import Picamera2
import numpy as np
import time

# initialize Picamera2
picam2 = Picamera2()
preview_config = picam2.create_preview_configuration()
picam2.configure(preview_config)
picam2.start()

def color_detection(img, color_name):
    """detect the target color and return the proportion"""
    hsv_img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    
    # create histogram
    hist = cv2.calcHist([hsv_img], [0, 1], None, [180, 256], [0, 180, 0, 256])

    # get the color thresholds
    lower_bound, upper_bound = get_color_threshold(color_name)

    # create mask
    mask = cv2.inRange(hsv_img, lower_bound, upper_bound)

    # back projection
    dst = cv2.calcBackProject([hsv_img], [0, 1], hist, [0, 180, 0, 256], 1)

    # calculate using mask
    res = cv2.bitwise_and(dst, dst, mask=mask)
    
    cv2.normalize(res, res, 0, 255, cv2.NORM_MINMAX)

    # turn the image to grayscale
    _, thresholded = cv2.threshold(res, 50, 255, 0)

    # morphological operations
    kernel = np.ones((5, 5), np.uint8)
    opening = cv2.morphologyEx(thresholded, cv2.MORPH_OPEN, kernel, iterations=2)
    closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel, iterations=3)

    # find the contours
    contours, _ = cv2.findContours(closing, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    output = cv2.drawContours(img.copy(), contours, -1, (0, 0, 255), 3)

    # calculate the proportion
    total_pixels = np.prod(mask.shape[:2])
    color_pixels = cv2.countNonZero(closing)
    color_percentage = (color_pixels / total_pixels) * 100
    return output, color_percentage

def get_color_threshold(color_name):
    """return the threshold according to the name of colors"""
    color_thresholds = {
        "red": ((110, 100, 100), (130, 255, 255)),
        # "yellow": ((90, 100, 100), (100, 255, 255)),
        "blue": ((0, 100, 100), (10, 255, 255)),
        "green": ((35, 25, 25), (86, 255, 255)),
        "purple":((150, 43, 64), (170, 255, 255))
    }
    return color_thresholds.get(color_name.lower())

try:
    while True:
        color_name = input("Please input the color needs to be detected (red/yellow/blue/green/purple): ")
        if color_name.lower() not in ["red", "yellow", "blue", "green", "purple"]:
            print("Please input the right name！")
            continue
        
        while True:
            # turn the image into NumPy array
            frame = picam2.capture_array()

            # color detection and return the proportion
            result, color_percentage = color_detection(frame, color_name)
            print(f"{color_name.capitalize()}占比：{color_percentage:.2f}%")

            # show the image
            cv2.imshow("Result", result)

            # check whether the proportion is larger than 30%
            if color_percentage >= 30:
                print("Found！")
                break

            # control the frequency
            time.sleep(0.1)

            # stop if 'q' pressed
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
                
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

except KeyboardInterrupt:
    # close the camera safely
    picam2.stop()
    cv2.destroyAllWindows()
