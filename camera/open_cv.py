import cv2
from picamera2 import Picamera2
import numpy as np
import time

def color_detection(img):
    hsv_img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    bound_lower = np.array([150, 0, 0])
    bound_upper = np.array([255, 50, 50])

    mask_green = cv2.inRange(hsv_img, bound_lower, bound_upper)
    kernel = np.ones((7, 7), np.uint8)

    mask_green = cv2.morphologyEx(mask_green, cv2.MORPH_CLOSE, kernel)
    mask_green = cv2.morphologyEx(mask_green, cv2.MORPH_OPEN, kernel)

    seg_img = cv2.bitwise_and(img, img, mask=mask_green)
    contours, hier = cv2.findContours(
        mask_green.copy(), cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE
    )
    output = cv2.drawContours(seg_img, contours, -1, (0, 0, 255), 3)

    cv2.imshow("Result", output)

# 配置和启动摄像头
picam2 = Picamera2()
config = picam2.create_preview_configuration(main={"size": (640, 480)})
picam2.configure(config)
picam2.start()

try:
    while True:
        # 捕获图像
        frame = picam2.capture_array()
        # 进行颜色识别
        color_detection(frame)
        # 显示结果帧
        #cv2.imshow("Color Detection", result)
        if cv2.waitKey(1) & 0xFF == ord('q'):  # 按 'q' 键退出
            break
        time.sleep(0.1)
finally:
    picam2.stop()
    cv2.destroyAllWindows()
