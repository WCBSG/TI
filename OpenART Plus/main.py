"""
OpenART 钢球检测 — TFLite + UART12（ROI 裁剪版）
协议: B,<cx>\n 检测到球 / N\n 未检测到；上电即持续发送
依赖: SD 卡根目录 yolo3_iou_smartcar_final_with_post_processing.tflite
"""
import sensor, tf, gc
from machine import UART

uart = UART(12, baudrate=115200)

def main():
    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)          # 全帧 320×240
    # 不再使用 set_windowing，改为每次从全帧中拷贝 ROI
    sensor.skip_frames(time=500)

    net = tf.load('/sd/yolo3_iou_smartcar_final_with_post_processing.tflite')
    gc.collect()

    ROI = (0, 104, 320, 32)                    # 与模型输入尺寸一致

    while True:
        img = sensor.snapshot()                # 全帧
        roi_img = img.copy(roi=ROI)            # 裁剪出 320×32 推理区域

        best_cx = -1
        best_score = 0.0

        for obj in tf.detect(net, roi_img):
            x1, y1, x2, y2, _, score = obj
            if score < 0.40 or x2 <= x1:
                continue
            cx = int((x1 + x2) / 2 * 320)      # 归一化中心 × 宽度
            if cx < 0 or cx >= 320:
                continue
            if score > best_score:
                best_score = score
                best_cx = cx

        if best_cx >= 0:
            uart.write("B,%d\n" % best_cx)
        else:
            uart.write("N\n")

        gc.collect()

if __name__ == "__main__":
    main()
