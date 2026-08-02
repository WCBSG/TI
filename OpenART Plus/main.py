"""
OpenART 钢球检测 — TFLite 模型 + UART 输出
===========================================
使用 yolo3_iou_smartcar_final 模型检测钢球，
每帧通过 UART12 发送球 X 坐标给 STC32。

协议:
  B,<cx>\n   — 检测到球，cx = 像素 X 坐标 (0-319)
  N\n        — 未检测到

上电即自动持续发送（无 START/STOP 命令门控，STC32 无需发开始指令）。

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

W, H        = 320, 240    # 暂时全帧 — 调试裁剪区域
CX          = W // 2       # 160

# 推理 ROI（模型输入裁剪区域）——必须与模型输入 [1,32,320,3] 完全一致（320×32），否则缩放失真导致 X 偏移
ROI_X, ROI_Y, ROI_W, ROI_H = 0, H//2-16, 320, 32   # 320×32 等比匹配模型，y ∈ [104, 136]

CLR_BALL    = (0, 255, 0)
CLR_TEXT    = (255, 255, 255)
CLR_RED     = (255, 80, 80)

# =============================================================================
# UART 通信
# =============================================================================

uart = UART(12, baudrate=115200)


def send_ball(cx):
    uart.write("B,%d\n" % cx)


def send_none():
    uart.write("N\n")


# =============================================================================
# 主循环
# =============================================================================

def main():
    print("\n  OpenART TFLite Ball Detect + UART12")
    print("  Model: %s" % MODEL_PATH)
    print("  ROI: %dx%d  Score min: %.2f" % (ROI_W, ROI_H, SCORE_MIN))
    print("  " + "-" * 40)

    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)       # 320×240 全帧（调试裁剪区域）
    sensor.set_brightness(500)
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
        # 模型输入为 [1, 32, 320, 3] RGB 三通道——直接取 ROI 传入，不转灰度
        img_small = img.copy(roi=(ROI_X, ROI_Y, ROI_W, ROI_H))

        best_cx   = 0
        best_score = 0.0
        found = False

        for obj in tf.detect(net, img_small):
            x1, y1, x2, y2, label, score = obj

            if score < SCORE_MIN:
                continue

            # 归一化坐标 → 全帧像素（ROI_X=0，X 即全帧 x；ROI 等比映射保证 X 准确）
            bx = int(x1 * ROI_W)
            bw = int((x2 - x1) * ROI_W)

            cx = bx + bw // 2

            if score > best_score:
                best_score = score
                best_cx = cx
                found = True

        # 发送 UART（上电即持续发送，无需开始命令）
        if found:
            send_ball(best_cx)
        else:
            send_none()

        # ── 辅助线：中心线 + 实际推理 ROI 边界（青线） ──
        img.draw_line(W//2, 0, W//2, H-1, color=(255,0,0), thickness=1)  # 竖中线
        img.draw_line(0, H//2, W-1, H//2, color=(0,0,255), thickness=1)  # 横中线
        img.draw_line(0, ROI_Y, W-1, ROI_Y, color=(0,255,255), thickness=1)
        img.draw_line(0, ROI_Y + ROI_H, W-1, ROI_Y + ROI_H, color=(0,255,255), thickness=1)

        # ── 调试：在帧上画检测结果（Y 固定 H//2 为设计，X 由等比 ROI 映射保证准确） ──
        if found:
            img.draw_cross(best_cx, H // 2,
                           color=CLR_BALL, size=5, thickness=2)
            img.draw_circle(best_cx, H // 2, 8,
                            color=CLR_BALL, thickness=2)

        fps = clock.fps()
        img.draw_string(2, 2, "%.1ffps TX:ON" % fps,
                        color=CLR_BALL, scale=1)
        if found:
            img.draw_string(2, 16, "X=%d s=%.2f" % (best_cx, best_score),
                            color=CLR_BALL, scale=1)
        else:
            img.draw_string(2, 16, "Ball: --",
                            color=CLR_RED, scale=1)

        gc.collect()


if __name__ == "__main__":
    main()
