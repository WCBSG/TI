"""
OpenART time 模块 — 时钟与延时。
"""

class Clock:
    """帧率/性能计时器。"""

    def __init__(self) -> None:
        """创建计时器并启动。"""
        ...

    def tick(self) -> int:
        """标记一帧结束，返回上一帧经过的毫秒数。"""
        ...

    def fps(self) -> float:
        """返回帧率（FPS）。"""
        ...

    def reset(self) -> None:
        """重置计时器。"""
        ...

    def avg(self) -> float:
        """返回平均帧率。"""
        ...


def sleep(seconds: float) -> None:
    """延时指定秒数。"""
    ...

def sleep_ms(ms: int) -> None:
    """延时指定毫秒数。"""
    ...

def sleep_us(us: int) -> None:
    """延时指定微秒数。"""
    ...

def ticks_ms() -> int:
    """返回毫秒计数器值（会溢出回绕）。"""
    ...

def ticks_us() -> int:
    """返回微秒计数器值（会溢出回绕）。"""
    ...

def ticks_diff(new: int, old: int) -> int:
    """计算两次 ticks 之间的差值（正确处理溢出回绕）。"""
    ...

def time() -> float:
    """返回自启动以来的秒数（浮点）。"""
    ...

def time_ns() -> int:
    """返回自启动以来的纳秒数。"""
    ...
