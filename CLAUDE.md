# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 项目概述

竞赛小车项目（H题 — 车载平衡滚球运动控制系统），**双摄像头 + STC32G144K246（主控）** 三芯片架构：

### 题目要点

- **摆杆**：4 根 PPR 水管（绿色 + 白色，外径 2cm，长 25cm）组成十字形轨道，钢球在管内滚动
- **赛道**：白色边线（宽 1.8±0.2cm）环形跑道，AB/CD 直道各 1.5m，BC/DA 半圆弧半径 0.5m
- **小球**：直径约 1cm 钢球，不可涂色
- **摄像头**：固定于车体，从上方拍摄摆杆区域 → 实时显示摆杆角度 + 钢球位置，并录像
- **小车**：长宽 ≤35×25cm，电赛专用车模，电池供电

### 任务要求

| 序号 | 任务 | 时间限制 | 精度 | 分值 |
|------|------|----------|------|------|
| 1 | 基本要求（设计报告等） | — | — | 20 |
| 2 | 从 A 点出发，顺时针一圈，停在 A | ≤20s | ±2cm | 16 |
| 3 | 静止时球稳定在中心 O ±5cm 内 | ≤5s | ±1cm | 13 |
| 4 | A→B 行驶，球稳定在中心 | ≤8s | ±1cm | 20 |
| 5 | 顺时针一圈，球稳定在中心 | ≤30s | ±1cm | 20 |
| 6 | 顺时针一圈，球稳定在指定位置 | ≤30s | ±1cm | 20 |
| 7 | 其他加分项 | — | — | 5 |

### 芯片分工

| 芯片 | 职责 |
|------|------|
| **OMV-RT5 (RT1062)** | 第一视角 WiFi MJPEG 图传，AP 热点供操作手查看（安装朝向赛道前方） |
| **OpenART Plus (RT117x)** | 从上方拍摄摆杆：色块识别水管（绿/白）得摆杆角度 + TFLite/色块检测钢球位置 |
| **STC32G144K246 (C251)** | 主控：接收检测结果 → PID 控制 → 电机/舵机输出；USB-CDC 调试 |

> **钢球不可涂色** → 需模型或灰度检测。**水管为绿色+白色** → `find_blobs` 色块识别即可跟踪摆杆角度，比 TFLite 快一个数量级。

## 目录结构

```
OMV-RT5/              ← 图传端 MicroPython 代码（OpenMV IDE 开发）
  main.py               ← WiFi AP MJPEG 图传（156行，精简版）
  omv-rt-visual_module/ ← 厂商资料 V2.0（例程/固件/手册，不纳入版本控制）

OpenART Plus/          ← 识别端 MicroPython 代码（OpenMV IDE 开发）
  camera_openart.py    ← 主程序：TFLite 模型钢球检测 + UART 输出
  cmm_cfg.csv          ← 引脚映射配置（LED/UART/I2C/SPI/ADC/PWM）
  cmm_load.py          ← 引脚映射加载器
  stubs/               ← Python 类型存根（sensor/image/machine/time/cmm）
  [例程]OpenART Plus例程/  ← 官方例程（AI模型/apriltag/基础外设/外置外设）

STC32G144K246_100Pin_Library/  ← STC32 官方开源库（只读参考，勿修改）
  Seekfree_STC32G144K_100Pin_Opensource_Library/
    libraries/
      zf_common/       ← 公共模块（时钟/FIFO/调试/中断/字体/typedef）
      zf_driver/       ← 底层驱动（GPIO/UART/PWM/SPI/ADC/Timer/Encoder/PIT/USB-CDC）
      zf_device/       ← 器件驱动（IMU/IPS屏幕/TFT/摄像头/无线模块/蓝牙）
      zf_components/   ← 中间件（USB协议栈/逐飞助手协议）
    project/           ← 工程模板
      user/main.c      ← 空白模板入口
      mdk/seekfree.uvproj  ← MDK for C251 工程文件
    Example/Coreboard_Demo/  ← 外设例程（E01~E13）

OurProject/         ← 当前 STC32 工程（在此开发）
  libraries/           ← 库文件（从 STC32 库复制）
  project/
    code/              ← 用户外设驱动代码
    mdk/               ← MDK 工程文件
    user/              ← main.c / isr.c / isr.h

_archive/              ← 存档（M0项目/K230项目/模型项目）
```

## 双摄像头 + 主控架构

```
[OMV-RT5 (RT1062)]                      [OpenART Plus (RT117x)]
  朝前拍摄：第一视角图传                      从上方拍摄摆杆区域
  WiFi AP MJPEG                           色块识别水管(绿/白) → 摆杆角度
  http://192.168.4.1:8000/                TFLite/灰度检测 → 钢球位置
       │                                         │
       │ WiFi (操作手手机/PC 查看)                 │ UART (115200bps)
       ↓                                         ↓
    [操作手]                            [STC32G144K246 (C251 @ 96MHz)]
                                          接收：摆杆角度 + 球坐标
                                          PID 控制 → 电机/舵机
                                          USB-CDC → PC 串口调试
```

- **OMV-RT5**：独立 WiFi AP，仅图传，不与 STC32 通信
- **OpenART Plus**：从上方垂直拍摄摆杆 → 色块识别水管姿态 + 检测钢球 → UART 发送给 STC32
- **STC32**：接收角度+球坐标 → 平衡/巡线控制算法 → 执行器输出

## 开发环境

| 端 | IDE | 语言 | 编译器/运行时 |
|---|---|---|---|
| OMV-RT5 | OpenMV IDE | MicroPython | MicroPython on RT1062 |
| OpenART Plus | OpenMV IDE | MicroPython | MicroPython on RT117x |
| STC32 | MDK for C251 V5.60 | C (C251) | Keil C251 |

- **VSCode 设置**：在 `.vscode/settings.json` 项目级修改，不要改用户级
- **VSCode 导航**：`#include` 行上的文件名用 Ctrl+单击跳转；函数/宏/类型用 F12
- **STC32 下载**：按住 P32 引脚上电进入 USB 下载模式（无需专用下载器）
- **STC32 头文件路径**：限定在 `OurProject/libraries` 与 `OurProject/project` 内，避免多副本同名文件导致跳转歧义

## STC32 编程约定

### 入口模板

```c
#include "zf_common_headfile.h"  // 包含所有库头文件

void main(void) {
    clock_init(SYSTEM_CLOCK_96M);  // 系统时钟初始化（务必保留）
    debug_init();                   // 调试串口初始化

    // 用户外设初始化代码

    while(1) {
        // 循环执行代码
    }
}
```

### UART 接收（中断+FIFO 模式）

```c
fifo_struct uart_fifo;
uint8 rx_buf[64];
uint8 get_buf[64];

fifo_init(&uart_fifo, FIFO_DATA_8BIT, rx_buf, 64);
uart_init(UART_5, 115200, UART5_TX_P05, UART5_RX_P04);
uart_rx_interrupt(UART_5, ZF_ENABLE, uart_rx_handler);  // 中断回调在 isr.c

// 在 while(1) 中轮询 FIFO
uint32 cnt = fifo_used(&uart_fifo);
if (cnt) {
    interrupt_global_disable();
    fifo_read_buffer(&uart_fifo, get_buf, &cnt, FIFO_READ_AND_CLEAN);
    interrupt_global_enable();
    // 处理 get_buf 中的数据
}
```

### 常用 API

| 功能 | 函数 |
|------|------|
| 时钟 | `clock_init(SYSTEM_CLOCK_96M)` / `SYSTEM_CLOCK_120M` |
| 延时 | `system_delay_ms(ms)` / `system_delay_us(us)` |
| GPIO | `gpio_set(pin, level)` / `gpio_get(pin)` |
| PWM | `pwm_init(pwm_n, freq, duty)` |
| 编码器 | `encoder_dir_init(...)` / `encoder_quad_init(...)` |
| 调试输出 | `debug_write_string(...)` (USB-CDC) |

## OMV-RT5 编程约定

### WiFi MJPEG 图传

OMV-RT5 仅负责实时图传，不参与检测：

- 运行 `main.py`：WiFi AP 模式，单页应用 MJPEG 推流
- 热点：`OMVRT5` / `12345678`，地址 `http://192.168.4.1:8000/`
- 分辨率 320×240，JPEG 质量 30
- 不主动 GC，不显示 FPS，不与 STC32 通信
- 用户通过手机/PC 浏览器查看小车第一视角画面

### OMV-RT5 引脚 (RT1062)

| 功能 | 引脚 | 备注 |
|------|------|------|
| LED | LED1~4 | 板载 |
| UART1 | TX=P1_01, RX=P1_02 | 未使用（不与 STC32 通信） |
| WiFi | 内置 | AP 模式 |

---

## OpenART 编程约定

OpenART Plus 摄像头从车体上方**垂直向下拍摄摆杆区域**，同时运行两类检测：

- **色块识别** → 跟踪摆杆姿态（绿色/白色 PPR 水管）
- **钢球检测** → 定位摆杆上的钢球位置（TFLite 或灰度色块）

### 色块识别（摆杆水管跟踪）

水管为绿色和白色 PPR 管，颜色鲜明稳定，`find_blobs` 速度快：

```python
# 绿色水管 LAB 阈值（需实地校准）
GREEN_THRESHOLD = [(30, 75, -50, -10, 5, 50)]
# 白色水管 LAB 阈值（需实地校准）
WHITE_THRESHOLD = [(80, 100, -15, 10, -15, 15)]

blobs = img.find_blobs(GREEN_THRESHOLD + WHITE_THRESHOLD,
                       pixels_threshold=200, area_threshold=200,
                       merge=True, margin=10)
for b in blobs:
    # b.cx(), b.cy() — 色块中心，可拟合摆杆中心线算出偏转角
    # b.rotation_deg() — 色块旋转角，直接对应摆杆方向
```

通过检测到的水管色块位置和角度，可实时计算出摆杆相对于车体坐标系的偏转角和位移。

### 钢球检测

钢球（直径 ~1cm）在水管凹槽内滚动，不可涂色。两种可选方案：

| 方案 | 方法 | 优点 | 缺点 |
|------|------|------|------|
| **TFLite 模型** | `tf.detect()` 目标检测 | 准确率高，已有训练好的模型 | 速度较慢（~50-100ms） |
| **灰度色块** | `find_blobs` 灰度阈值 | 速度快（~5ms），球在浅色水管上对比度高 | 可能受光照干扰 |

- TFLite 模型: `_合并-2tflite.tflite` (YOLOv3 MobileNetV2, INT8, 112×112, 含 NMS)
- 灰度方案：球在白色/绿色水管背景上为暗色圆形，ROI 限制在摆杆区域后 `find_blobs` 灰度阈值即可检出

> 推荐在摆杆色块定位后，**在摆杆 ROI 区域内做灰度色块检测钢球**，兼顾速度和精度。

### UART 输出格式

OpenART 通过 UART12 发送检测结果给 STC32（115200bps）：

```
A,angle\n                 ← 摆杆偏转角 (度，相对于车体中心线)
B,cx,cy,score\n           ← 钢球在摆杆上的位置 (像素坐标, 置信度)
N\n                       ← 未检测到
```

> 坐标原点建议以摆杆中心 O 为参考，STC32 接收到后做 PID 控制。

### OpenART 引脚映射 (cmm_cfg.csv)

| 功能 | 引脚 | 备注 |
|------|------|------|
| LED1~4 | DISP_B2_03/B1_07/B1_05/B2_08 | 板载 LED |
| UART12 | TX=LPSR_04, RX=LPSR_05 | 与 STC32 通信 |
| UART5 | TX=AD_28, RX=AD_29 | 备用串口 |
| UART11 | TX=LPSR_04, RX=LPSR_05 | 共用 UART12 引脚 |
| I2C5 | SDA=LPSR_04, SCL=LPSR_05 | 共用 UART12 引脚 |
| I2C6 | SDA=LPSR_06, SCL=LPSR_07 | |
| SPI6 | SCK=LPSR_10, SDO=LPSR_11, SDI=LPSR_12, CS=LPSR_09 | |
| ADC A0~A3 | AD_26~AD_29 | |
| PWM CH1~4 | AD_26~AD_29 (FlexPWM2) | |

> **注意**：UART12、UART11、I2C5 共用 LPSR_04/LPSR_05 引脚，默认用于与 STC32 通信的 UART12，不可同时使用。

## 比赛题目

H题 — 车载平衡滚球运动控制系统（见 `H题_车载平衡滚球运动控制系统.pdf`）
