"""
OpenART machine 模块 — 硬件外设（Pin / UART / PWM / I2C / SPI / ADC / Timer）。
"""

from typing import Optional, List, Tuple


# ============================================================
# Pin（GPIO）
# ============================================================

class Pin:
    OUT: int
    IN: int
    IN_PUP: int      # 上拉输入
    IN_PDN: int      # 下拉输入
    OPEN_DRAIN: int  # 开漏输出
    IRQ_FALLING: int
    IRQ_RISING: int
    IRQ_LOW: int
    IRQ_HIGH: int

    def __init__(self, name: str, mode: int, *, pull: int = None, value: int = None) -> None:
        """
        初始化引脚。
        - name: 引脚名，如 "M12", "LPSR_04", "AD_26"
        - mode: Pin.OUT / Pin.IN / Pin.IN_PUP / Pin.IN_PDN / Pin.OPEN_DRAIN
        """
        ...

    def value(self, val: Optional[int] = None) -> Optional[int]:
        """
        读取 / 设置引脚电平。
        - 无参数 → 返回 0/1
        - value(0) 输出低电平，value(1) 输出高电平
        """
        ...

    def on(self) -> None:
        """输出高电平（等价于 value(1)）。"""
        ...

    def off(self) -> None:
        """输出低电平（等价于 value(0)）。"""
        ...

    def irq(self, handler, trigger: int) -> None:
        """设置引脚中断回调。"""
        ...


# ============================================================
# UART（串口）
# ============================================================

class UART:
    def __init__(self, uart_id: int, *, baudrate: int = 115200,
                 tx_pin: str = None, rx_pin: str = None,
                 bits: int = 8, parity: int = None, stop: int = 1) -> None:
        """
        初始化串口。
        - uart_id: 串口编号（5, 11, 12 等，对应 cmm_cfg.csv）
        """
        ...

    def write(self, data) -> int:
        """
        发送数据。
        - data 可以是 str / bytes / bytearray
        - 返回发送的字节数
        """
        ...

    def any(self) -> int:
        """返回接收缓冲区的可读字节数。"""
        ...

    def read(self, n: int) -> bytes:
        """读取 n 字节（无数据则阻塞）。"""
        ...

    def readchar(self) -> int:
        """读取一个字节（无数据则阻塞），返回 0~255。"""
        ...

    def readline(self) -> bytes:
        """读取一行（以 \\n 结尾）。"""
        ...

    def flush(self) -> None:
        """清空缓冲区。"""
        ...

    def deinit(self) -> None:
        """关闭串口并释放引脚。"""
        ...


# ============================================================
# PWM
# ============================================================

class PWM:
    def __init__(self, pin_name: str, *, freq: int = 50, duty: int = 0) -> None:
        """
        初始化 PWM。
        - freq: 频率（Hz）
        - duty: 占空比（0~100）
        """
        ...

    def duty(self, value: int) -> None:
        """设置占空比（0~100）。"""
        ...

    def freq(self, value: int) -> None:
        """设置频率（Hz）。"""
        ...

    def deinit(self) -> None:
        """关闭 PWM。"""
        ...


# ============================================================
# I2C
# ============================================================

class I2C:
    def __init__(self, i2c_id: int, *, freq: int = 100000,
                 sda_pin: str = None, scl_pin: str = None) -> None:
        """初始化 I2C。"""
        ...

    def scan(self) -> List[int]:
        """扫描总线，返回从机地址列表。"""
        ...

    def readfrom(self, addr: int, nbytes: int) -> bytes:
        """从指定地址读取 n 字节。"""
        ...

    def writeto(self, addr: int, buf, *, stop: bool = True) -> None:
        """向指定地址写入数据。"""
        ...

    def readfrom_mem(self, addr: int, memaddr: int, nbytes: int) -> bytes:
        """从寄存器地址读取数据。"""
        ...

    def writeto_mem(self, addr: int, memaddr: int, buf) -> None:
        """向寄存器地址写入数据。"""
        ...

    def deinit(self) -> None:
        """关闭 I2C。"""
        ...


# ============================================================
# SPI
# ============================================================

class SPI:
    def __init__(self, spi_id: int, *, baudrate: int = 1000000,
                 polarity: int = 0, phase: int = 0,
                 sck_pin: str = None, mosi_pin: str = None,
                 miso_pin: str = None, cs_pin: str = None) -> None:
        """初始化 SPI。"""
        ...

    def write(self, buf) -> None:
        """发送数据。"""
        ...

    def read(self, nbytes: int) -> bytes:
        """读取数据。"""
        ...

    def write_readinto(self, write_buf, read_buf) -> None:
        """同时发送和接收。"""
        ...

    def deinit(self) -> None:
        """关闭 SPI。"""
        ...


# ============================================================
# ADC
# ============================================================

class ADC:
    def __init__(self, pin_name: str) -> None:
        """初始化 ADC 通道。"""
        ...

    def read(self) -> int:
        """读取 12 位 ADC 值（0 ~ 4095）。"""
        ...

    def read_voltage(self) -> float:
        """读取电压值（V）。"""
        ...

    def deinit(self) -> None:
        """关闭 ADC。"""
        ...


# ============================================================
# Timer（定时器）
# ============================================================

class Timer:
    def __init__(self, tim_id: int, *, freq: int = 1,
                 callback=None, period: int = 0) -> None:
        """初始化定时器。"""
        ...

    def deinit(self) -> None:
        """关闭定时器。"""
        ...
