"""
OpenART sensor 模块 — 摄像头图像传感器。
"""

from typing import Optional

# ---- 像素格式常量 ----
BAYER: int
RGB565: int
GRAYSCALE: int
JPEG: int

# ---- 帧尺寸常量 ----
QQCIF: int       # 88×72
QCIF: int        # 176×144
CIF: int         # 352×288
QQVGA: int       # 160×120
QVGA: int        # 320×240
VGA: int         # 640×480
SVGA: int        # 800×600
SXGA: int        # 1280×1024
UXGA: int        # 1600×1200
HD: int           # 1280×720
FHD: int          # 1920×1080

# ---- 帧速率 ----
FPS_30: int
FPS_60: int
FPS_120: int

def reset() -> None:
    """复位并初始化摄像头传感器。"""
    ...

def set_pixformat(fmt: int) -> None:
    """设置像素格式，如 sensor.RGB565。"""
    ...

def set_framesize(size: int) -> None:
    """设置帧尺寸，如 sensor.QVGA（320×240）。"""
    ...

def set_framerate(rate: int) -> None:
    """设置帧率。"""
    ...

def skip_frames(n: int = 10, *, time: int = 0) -> None:
    """跳过 n 帧（或等待 time 毫秒），等待摄像头稳定。"""
    ...

def snapshot() -> "Image":
    """采集一帧图像，返回 Image 对象。"""
    ...

def set_auto_exposure(enable: bool, *, exposure_us: int = 0) -> None:
    """设置自动曝光。"""
    ...

def set_auto_whitebal(enable: bool, *, rgb_gain_db: tuple = (0, 0, 0)) -> None:
    """设置自动白平衡。"""
    ...

def set_auto_gain(enable: bool, *, gain_db: float = 0.0) -> None:
    """设置自动增益。"""
    ...

def set_brightness(brightness: int) -> None:
    """设置亮度 (-3 ~ 3)。"""
    ...

def set_contrast(contrast: int) -> None:
    """设置对比度 (-3 ~ 3)。"""
    ...

def set_saturation(saturation: int) -> None:
    """设置饱和度 (-3 ~ 3)。"""
    ...

def set_windowing(roi: tuple[int, int, int, int]) -> None:
    """设置传感器开窗 ROI (x, y, w, h)。"""
    ...

def get_id() -> int:
    """获取摄像头传感器型号 ID。"""
    ...

def width() -> int:
    """获取当前帧宽度。"""
    ...

def height() -> int:
    """获取当前帧高度。"""
    ...

def set_vflip(enable: bool) -> None:
    """垂直翻转。"""
    ...

def set_hmirror(enable: bool) -> None:
    """水平镜像。"""
    ...
