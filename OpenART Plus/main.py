"""
OpenART 钢球检测 — TFLite 模型 + UART 输出
===========================================
使用 yolo3_iou_smartcar_final 模型检测钢球，
每帧通过 UART12 发送球 X 坐标给 STC32。

协议:
  B,<cx>\n   — 检测到球，cx = 像素 X 坐标 (0-319)
  N\n        — 未检测到

控制命令 (STC32→OpenART):
  START\n    — 开始发送
  STOP\n    — 暂停发送
  CAL\n     — 校准零点

依赖:
  SD 卡根目录放入 yolo3_iou_smartcar_final_with_post_processing.tflite
"""

import sensor, image, time, tf, gc
from machine import UART

# =============================================================================
# 配置
# =============================================================================

MODEL_PATH  = '/sd/yolo3_iou_smartcar_final_with_post_processing.tflite'
SCORE_MIN   = 0.70       # 置信度阈值
SCALE       = 1.0        # 推理缩放 (裁剪后已很小，无需再缩；嫌慢可改 0.75)

W, H        = 320, 240    # 暂时全帧 — 调试裁剪区域
CX          = W // 2       # 160
CROP_Y1     = 80           # 计划裁剪上边界
CROP_Y2     = 160          # 计划裁剪下边界 (80+80)

CLR_BALL    = (0, 255, 0)
CLR_TEXT    = (255, 255, 255)
CLR_RED     = (255, 80, 80)

# =============================================================================
# UART 通信
# =============================================================================

uart = UART(12, baudrate=115200)
uart_sending = True


def send_ball(cx):
    if uart_sending:
        uart.write("B,%d\n" % cx)


def send_none():
    if uart_sending:
        uart.write("N\n")


def check_command():
    global uart_sending
    n = uart.any()
    if n == 0:
        return
    data = uart.read(n)
    if data is None:
        return
    try:
        cmd = data.decode().strip().upper()
    except:
        return

    if cmd == "START":
        uart_sending = True
    elif cmd == "STOP":
        uart_sending = False
    elif cmd == "CAL":
        pass  # TODO


# =============================================================================
# 主循环
# =============================================================================

def main():
    print("\n  OpenART TFLite Ball Detect + UART12")
    print("  Model: %s" % MODEL_PATH)
    print("  Crop: %dx%d  Score min: %.2f" % (W, H, SCORE_MIN))
    print("  " + "-" * 40)

    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)       # 320×240 全帧（调试裁剪区域）
    # sensor.set_windowing((0, 80, 320, 80))  # 确认裁剪区域后再启用
    sensor.skip_frames(time=500)

    # 加载模型
    print("  Loading model...")
    net = tf.load(MODEL_PATH)
    print("  Model loaded.")
    gc.collect()

    clock = time.clock()

    while True:
        clock.tick()

        img = sensor.snapshot()

        # 缩放推理（缩小图像加速）
        img_small = img.copy(SCALE, 1.0)

        best_cx   = 0
        best_score = 0.0
        found = False

        for obj in tf.detect(net, img_small):
            x1, y1, x2, y2, label, score = obj

            if score < SCORE_MIN:
                continue

            # 归一化坐标 → 像素坐标
            bx = int(x1 * W)
            bw = int((x2 - x1) * W)

            cx = bx + bw // 2

            if score > best_score:
                best_score = score
                best_cx = cx
                found = True

        # 发送 UART
        if found:
            send_ball(best_cx)
        else:
            send_none()

        # 检查命令
        check_command()

        # ── 裁剪区域指示线 ──
        img.draw_line(0, CROP_Y1, W-1, CROP_Y1, color=(255,255,0), thickness=1)  # 上边界
        img.draw_line(0, CROP_Y2, W-1, CROP_Y2, color=(255,255,0), thickness=1)  # 下边界
        img.draw_string(2, CROP_Y1-14, "crop y=%d" % CROP_Y1, color=(255,255,0), scale=1)
        img.draw_string(2, CROP_Y2+2,  "crop y=%d" % CROP_Y2, color=(255,255,0), scale=1)

        # ── 调试：在帧上画检测结果 ──
        if found:
            img.draw_cross(best_cx, H // 2,
                           color=CLR_BALL, size=5, thickness=2)
            img.draw_circle(best_cx, H // 2, 8,
                            color=CLR_BALL, thickness=2)

        fps = clock.fps()
        status = "TX:ON" if uart_sending else "TX:OFF"
        status_color = CLR_BALL if uart_sending else CLR_RED
        img.draw_string(2, 2, "%.1ffps %s" % (fps, status),
                        color=status_color, scale=1)
        if found:
            img.draw_string(2, 16, "X=%d s=%.2f" % (best_cx, best_score),
                            color=CLR_BALL, scale=1)
        else:
            img.draw_string(2, 16, "Ball: --",
                            color=CLR_RED, scale=1)

        gc.collect()


if __name__ == "__main__":
    main()
