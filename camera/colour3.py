import cv2
from picamera2 import Picamera2
import numpy as np
import time

# 初始化 Picamera2
picam2 = Picamera2()
preview_config = picam2.create_preview_configuration()
picam2.configure(preview_config)
picam2.start()

def color_detection(img, color_name):
    """检测图像中特定颜色的物体并返回其占比"""
    hsv_img = cv2.cvtColor(img, cv2.COLOR_BGR2HSV)
    
    # 创建颜色直方图
    hist = cv2.calcHist([hsv_img], [0, 1], None, [180, 256], [0, 180, 0, 256])

    # 获取颜色范围
    lower_bound, upper_bound = get_color_threshold(color_name)

    # 根据颜色范围创建掩码
    mask = cv2.inRange(hsv_img, lower_bound, upper_bound)

    # 使用直方图反向投影
    dst = cv2.calcBackProject([hsv_img], [0, 1], hist, [0, 180, 0, 256], 1)

    # 对掩码进行位运算
    res = cv2.bitwise_and(dst, dst, mask=mask)

    # 归一化处理
    cv2.normalize(res, res, 0, 255, cv2.NORM_MINMAX)

    # 转换为二值图像
    _, thresholded = cv2.threshold(res, 50, 255, 0)

    # 使用形态学操作去除噪点
    kernel = np.ones((5, 5), np.uint8)
    opening = cv2.morphologyEx(thresholded, cv2.MORPH_OPEN, kernel, iterations=2)
    closing = cv2.morphologyEx(opening, cv2.MORPH_CLOSE, kernel, iterations=3)

    # 找到轮廓并绘制
    contours, _ = cv2.findContours(closing, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
    output = cv2.drawContours(img.copy(), contours, -1, (0, 0, 255), 3)

    # 计算占比
    total_pixels = np.prod(mask.shape[:2])
    color_pixels = cv2.countNonZero(closing)
    color_percentage = (color_pixels / total_pixels) * 100
    return output, color_percentage

def get_color_threshold(color_name):
    """根据颜色名称返回相应的颜色阈值"""
    color_thresholds = {
        "red": ((110, 100, 100), (130, 255, 255)),
        "yellow": (90, 100, 100), (100, 255, 255)),
        "blue": ((0, 100, 100), (10, 255, 255)),
        "green": ((35, 25, 25), (86, 255, 255)),
        "purple":((150, 43, 64), (170, 255, 255))
    }
    return color_thresholds.get(color_name.lower())

try:334
    while True:
        # 获取用户输入的颜色名称
        color_name = input("请输入要检测的颜色名称 (red/yellow/blue/green/purple): ")
        if color_name.lower() not in ["red", "yellow", "blue", "green", "purple"]:
            print("请输入正确的颜色名称！")
            continue
        
        while True:
            # 捕获图像到NumPy数组
            frame = picam2.capture_array()

            # 进行颜色检测并获取占比
            result, color_percentage = color_detection(frame, color_name)
            print(f"{color_name.capitalize()}占比：{color_percentage:.2f}%")

            # 显示结果图像
            cv2.imshow("Result", result)

            # 检查是否达到占比30%
            if color_percentage >= 30:
                print("找到了！")
                break

            # 暂停一段时间以降低处理频率
            time.sleep(0.1)

            # 检查是否按下 'q' 键
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        # 检查是否按下 'q' 键
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

except KeyboardInterrupt:
    # 安全关闭摄像头
    picam2.stop()
    cv2.destroyAllWindows()
