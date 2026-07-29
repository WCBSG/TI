"""
OpenART 绿色小球检测 — OpenMV IDE 调试
======================================

K230 camera.py → OpenART 转换, 去除外接屏, 在 OpenMV IDE 帧缓冲窗查看。
"""

import sensor, image, time

# =============================================================================
# 配置
# =============================================================================

W, H            = 320, 240   # sensor.QVGA
CX, CY          = W // 2, H // 2

GREEN_THRESHOLD  = (42, 100, -128, -17, 6, 66)
CENTER_RADIUS    = 60
FILL_MIN         = 0.60
FILL_MAX         = 0.90
ASPECT_MIN       = 0.70

CLR_CROSS   = (255, 255, 255)
CLR_RING    = (255, 255, 0)
CLR_BALL    = (0, 255, 0)
CLR_TEXT    = (255, 255, 255)


# =============================================================================
# 检测
# =============================================================================

def find_best_spherical_green_blob(img):
    blobs = img.find_blobs([GREEN_THRESHOLD])
    if not blobs:
        return (None, 0.0, 0, 0)

    green_n = len(blobs)

    spheres = []
    for b in blobs:
        bw, bh, area = b[2], b[3], b[4]
        if bw > 0 and bh > 0 and area > 0:
            fill = area / (bw * bh)
            aspect = (bw / bh) if bw < bh else (bh / bw) if bw > bh else 1.0
            if FILL_MIN <= fill <= FILL_MAX and aspect >= ASPECT_MIN:
                spheres.append(b)

    if not spheres:
        return (None, 0.0, green_n, 0)

    def _dist(b):
        return ((b[5] - CX) ** 2 + (b[6] - CY) ** 2) ** 0.5

    inner = [b for b in spheres if _dist(b) <= CENTER_RADIUS]
    if inner:
        inner.sort(key=lambda b: b[4], reverse=True)
        target = inner[0]
    else:
        spheres.sort(key=_dist)
        target = spheres[0]

    return (target, _dist(target), green_n, len(spheres))


# =============================================================================
# 绘制
# =============================================================================

def draw_center_crosshair(img):
    half = 15
    img.draw_line(CX - half, CY, CX + half, CY,
                  color=CLR_CROSS, thickness=1)
    img.draw_line(CX, CY - half, CX, CY + half,
                  color=CLR_CROSS, thickness=1)
    img.draw_circle(CX, CY, 4, color=CLR_CROSS, thickness=1)
    img.draw_circle(CX, CY, CENTER_RADIUS,
                    color=(128, 128, 128), thickness=1)


def draw_target_circle(img, blob, distance):
    cx, cy = int(blob[5]), int(blob[6])
    r = max(blob[2], blob[3]) // 2
    img.draw_circle(cx, cy, r + 1, color=CLR_RING, thickness=3)
    img.draw_cross(cx, cy, color=CLR_BALL, thickness=2)
    img.draw_string(max(cx - 15, 2), max(cy - r - 15, 2),
                    "%dpx" % int(distance), color=CLR_RING, scale=1)


def draw_info_panel(img, fps, center_rgb, ball_rgb, distance,
                    green_n, sphere_n, has_target):
    img.draw_rectangle(0, 0, W, 54, color=(20, 20, 30),
                       thickness=-1, fill=True)

    img.draw_string(4, 2, "FPS:%.1f" % fps, color=CLR_TEXT, scale=1)

    if center_rgb:
        r, g, b = center_rgb
        img.draw_string(4, 16, "C(%d,%d,%d)" % (r, g, b),
                        color=(200, 200, 200), scale=1)

    if has_target and ball_rgb:
        r, g, b = ball_rgb
        img.draw_string(4, 30, "B(%d,%d,%d)" % (r, g, b),
                        color=CLR_BALL, scale=1)
    else:
        img.draw_string(4, 30, "Ball: --",
                        color=(150, 150, 150), scale=1)

    if has_target:
        img.draw_string(180, 16,
                        "G:%d S:%d d:%d" %
                        (green_n, sphere_n, int(distance)),
                        color=(180, 180, 180), scale=1)


# =============================================================================
# 主流程
# =============================================================================

def main():
    print("\n  OpenART 绿色小球检测 @ %dx%d" % (W, H))
    print("  OpenMV IDE 帧缓冲窗查看画面")
    print("  " + "-" * 32)

    sensor.reset()
    sensor.set_pixformat(sensor.RGB565)
    sensor.set_framesize(sensor.QVGA)
    sensor.skip_frames(time=200)

    clock = time.clock()

    while True:
        clock.tick()

        img = sensor.snapshot()     # IDE 自动捕获帧缓冲

        # 检测
        target, distance, green_n, sphere_n = \
            find_best_spherical_green_blob(img)
        has_target = target is not None

        # RGB 采样 (get_pixel 在 RGB565 模式直接返回 (r,g,b))
        center_rgb = img.get_pixel(CX, CY)
        if has_target:
            tx, ty = int(target[5]), int(target[6])
            ball_rgb = img.get_pixel(tx, ty)
        else:
            ball_rgb = None

        # 绘制
        draw_center_crosshair(img)
        if has_target:
            draw_target_circle(img, target, distance)

        fps = clock.fps()
        draw_info_panel(img, fps, center_rgb, ball_rgb, distance,
                       green_n, sphere_n, has_target)

        print("%.1f fps" % fps)


if __name__ == "__main__":
    main()
