"""
OpenART image 模块 — 图像对象及视觉算法。
"""

from typing import Optional, List, Tuple

class Image:
    """摄像头采集的图像帧，支持 blob 检测与绘图。"""

    def width(self) -> int:
        """图像宽度（像素）。"""
        ...

    def height(self) -> int:
        """图像高度（像素）。"""
        ...

    def get_pixel(self, x: int, y: int) -> Tuple[int, int, int]:
        """返回 (r, g, b) 元组。"""
        ...

    def set_pixel(self, x: int, y: int, color: Tuple[int, int, int]) -> None:
        """设置像素颜色。"""
        ...

    # ---- 绘制 ----

    def draw_line(
        self,
        x0: int, y0: int, x1: int, y1: int,
        *, color: Tuple[int, int, int] = (255, 255, 255),
        thickness: int = 1,
    ) -> None:
        """绘制直线。"""
        ...

    def draw_circle(
        self,
        x: int, y: int, radius: int,
        *, color: Tuple[int, int, int] = (255, 255, 255),
        thickness: int = 1,
        fill: bool = False,
    ) -> None:
        """绘制圆（fill=True 填充）。"""
        ...

    def draw_rectangle(
        self,
        x: int, y: int, w: int, h: int,
        *, color: Tuple[int, int, int] = (255, 255, 255),
        thickness: int = 1,
        fill: bool = False,
    ) -> None:
        """绘制矩形（fill=True 填充）。"""
        ...

    def draw_cross(
        self,
        x: int, y: int,
        *, color: Tuple[int, int, int] = (255, 255, 255),
        size: int = 5,
        thickness: int = 1,
    ) -> None:
        """绘制十字准星。"""
        ...

    def draw_string(
        self,
        x: int, y: int, text: str,
        *, color: Tuple[int, int, int] = (255, 255, 255),
        scale: int = 1,
        mono_space: bool = False,
    ) -> None:
        """绘制字符串。"""
        ...

    def draw_arrow(
        self,
        x0: int, y0: int, x1: int, y1: int,
        *, color: Tuple[int, int, int] = (255, 255, 255),
        thickness: int = 1,
    ) -> None:
        """绘制箭头。"""
        ...

    def draw_image(
        self,
        img: "Image", x: int, y: int,
        *, alpha: int = 256,
    ) -> None:
        """将另一张图像绘制到当前图像上。"""
        ...

    # ---- 二值化 / 色块 ----

    def binary(self, thresholds: List[Tuple[int, int, int, int, int, int]]) -> "Image":
        """按阈值列表二值化，返回新 Image。"""
        ...

    def find_blobs(
        self,
        thresholds: List[Tuple[int, int, int, int, int, int]],
        *,
        pixels_threshold: int = 10,
        area_threshold: int = 10,
        merge: bool = True,
        margin: int = 0,
        invert: bool = False,
    ) -> List["Blob"]:
        """查找符合阈值范围的色块。"""
        ...

    def find_circles(
        self,
        *,
        threshold: int = 2000,
        x_margin: int = 10,
        y_margin: int = 10,
        r_margin: int = 10,
    ) -> List[Tuple[int, int, int]]:
        """霍夫圆检测，返回 [(x, y, r), ...]。"""
        ...

    def find_rects(
        self,
        *,
        threshold: int = 10000,
    ) -> List[Tuple[int, int, int, int]]:
        """查找矩形，返回 [(x, y, w, h), ...]。"""
        ...

    def find_lines(
        self,
        *,
        threshold: int = 1000,
        theta_margin: int = 25,
        rho_margin: int = 25,
    ) -> List[Tuple[int, int, int, int]]:
        """霍夫直线检测，返回 [(x0, y0, x1, y1), ...]。"""
        ...

    def find_apriltags(
        self,
        *,
        families: int = 0,
        fx: float = 0.0, fy: float = 0.0,
        cx: float = 0.0, cy: float = 0.0,
    ) -> List["Apriltag"]:
        """查找 AprilTags。"""
        ...

    # ---- 图像变换 ----

    def copy(self, *, roi: tuple = None) -> "Image":
        """复制图像（可选 ROI）。"""
        ...

    def resize(self, w: int, h: int) -> "Image":
        """缩放图像。"""
        ...

    def crop(self, x: int, y: int, w: int, h: int) -> "Image":
        """裁剪 ROI。"""
        ...

    def rotation_corr(self, *, x_rotation: float = 0.0, y_rotation: float = 0.0,
                      z_rotation: float = 0.0) -> "Image":
        """旋转校正。"""
        ...

    def lens_corr(self, strength: float = 1.0) -> "Image":
        """镜头畸变校正。"""
        ...

    def mean_pooled(self, new_w: int, new_h: int) -> "Image":
        """均值池化降采样。"""
        ...

    # ---- 统计 ----

    def get_histogram(self) -> "Histogram":
        """获取 RGB565 三通道直方图。"""
        ...

    def get_statistics(self, *, thresholds: list = None) -> "Statistics":
        """获取图像统计信息（均值、中位数等）。"""
        ...

    # ---- 格式转换 ----

    def to_rgb565(self) -> "Image":
        """转换为 RGB565。"""
        ...

    def to_grayscale(self) -> "Image":
        """转换为灰度。"""
        ...

    def compress(self, quality: int = 50) -> bytes:
        """JPEG 压缩，返回字节串。"""
        ...

    def save(self, path: str, *, quality: int = 50) -> None:
        """保存为 BMP/PGM/PPM/JPEG 到 SD 卡。"""
        ...


class Blob:
    """find_blobs 返回的色块对象。

    blob[0] = x, blob[1] = y, blob[2] = w, blob[3] = h,
    blob[4] = area(pixels), blob[5] = cx, blob[6] = cy,
    blob[7] = rotation_deg
    """

    def x(self) -> int: ...
    def y(self) -> int: ...
    def w(self) -> int: ...
    def h(self) -> int: ...
    def area(self) -> int: ...
    def cx(self) -> int: ...
    def cy(self) -> int: ...
    def rotation(self) -> float: ...

    def rect(self) -> Tuple[int, int, int, int]:
        """返回 (x, y, w, h)。"""
        ...

    def corners(self) -> List[Tuple[int, int]]:
        """返回四个角 [(x0,y0), (x1,y1), (x2,y2), (x3,y3)]。"""
        ...

    def code(self) -> int:
        """16 位二进制编码（用于二维码）。"""
        ...

    def count(self) -> int:
        """子色块数量。"""
        ...

    def density(self) -> float:
        """密度（面积 / 外接矩形面积）。"""
        ...

    # 支持下标索引
    def __getitem__(self, idx: int) -> int: ...
    def __len__(self) -> int: ...


class Apriltag:
    """find_apriltags 返回的标签对象。"""

    def id(self) -> int: ...
    def cx(self) -> int: ...
    def cy(self) -> int: ...
    def x_translation(self) -> float: ...
    def y_translation(self) -> float: ...
    def z_translation(self) -> float: ...
    def x_rotation(self) -> float: ...
    def y_rotation(self) -> float: ...
    def z_rotation(self) -> float: ...


class Histogram:
    """RGB565 直方图。"""

    def get_threshold(self, channel: int) -> int: ...


class Statistics:
    """图像统计信息。"""

    def mean(self) -> Tuple[int, int, int]: ...
    def median(self) -> Tuple[int, int, int]: ...
    def mode(self) -> Tuple[int, int, int]: ...
    def min(self) -> Tuple[int, int, int]: ...
    def max(self) -> Tuple[int, int, int]: ...
    def stdev(self) -> Tuple[int, int, int]: ...
