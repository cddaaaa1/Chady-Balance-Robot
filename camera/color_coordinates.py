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
    """find the largest """
    red_points = np.where(np.all((frame >= RED_MIN) & (frame <= RED_MAX), axis=-1))
    if red_points[0].size > 0:
        y_center = np.mean(red_points[0])
        x_center = np.mean(red_points[1])
        return (x_center, y_center)
    return None

try:
    while True:
        # 捕获图像到NumPy数组
        frame = picam2.capture_array()
        
        # 找到红色区域的中心点
        blob_center = find_max_blob(frame[..., :3])
        if blob_center:
            screen_center = (frame.shape[1] / 2, frame.shape[0] / 2)
            x_offset = blob_center[0] - screen_center[0]
            h_offset = blob center[1] - screen_center[1]

            # 输出X和H的偏移量
            print(f"X offset: {x_offset}, H offset: {h_offset}")
        else:
            print("No significant red blob found")

except KeyboardInterrupt:
    # 安全关闭摄像头
    picam2.stop()
