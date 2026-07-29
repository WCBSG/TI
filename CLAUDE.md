# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 项目概述

竞赛小车项目，**OpenART Plus（视觉色块识别）+ STC32G144K246（主控）** 双芯片架构。OpenART 负责摄像头图像采集和色块检测，通过 UART 将检测结果发送给 STC32，STC32 负责电机控制、传感器读取和整车逻辑。

## 目录结构

```
OpenART Plus/          ← 视觉端 MicroPython 代码（OpenMV IDE 开发）
  camera_openart.py    ← 当前主程序：绿色小球色块检测
  cmm_cfg.csv          ← 引脚映射配置（LED/UART/I2C/SPI/ADC/PWM）
  cmm_load.py          ← 引脚映射加载器
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

## 双芯片通信架构

```
[OpenART Plus (RT117x)]
  - 运行 camera_openart.py
  - sensor 采集图像 → find_blobs() 色块检测
  - UART12 (LPSR_04/LPSR_05) 发送检测结果给 STC32
  - OpenMV IDE 帧缓冲窗查看画面（无需外接屏）

       ↓ UART (115200bps)

[STC32G144K246 (C251 @ 96MHz)]
  - UARTx 接收 OpenART 发来的坐标/颜色/距离
  - 解析数据 → 电机/PWM/舵机控制
  - USB-CDC (Type-C) 连接 PC 串口助手调试
  - 内部晶振最大 120MHz（默认 96MHz）
```

## 开发环境

| 端 | IDE | 语言 | 编译器/运行时 |
|---|---|---|---|
| OpenART | OpenMV IDE | MicroPython | MicroPython on RT117x |
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

## OpenART 编程约定

### 色块检测模式

基于 `image.find_blobs([threshold])` ，当前 `camera_openart.py` 检测绿色小球：
- `GREEN_THRESHOLD = (42, 100, -128, -17, 6, 66)` (LAB 色彩空间)
- 过滤条件：填充率 60%~90%、宽高比 ≥ 0.7
- 优先帧中心 60px 半径内最近球体，按面积降序
- 在 OpenMV IDE 帧缓冲窗实时预览

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
