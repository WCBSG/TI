# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## 项目概述

竞赛小车项目（H题 — 车载平衡滚球运动控制系统），**OpenART Plus（视觉识别）+ OMV-RT5（WiFi 图传）+ STC32G144K246（主控）** 三芯片架构：

### 题目要点

- **摆杆**：**PPR 水管对半切开**（剖面 = 半圆弧形凹槽，绿色或白色，外径 2cm，长 25cm），**左端**用铰链/合页固定在车体平板上方（**h≥5cm**），钢球在半圆弧凹槽内滚动
- **赛道**：环形跑道，外边线圆弧半径 50cm，两侧白色边线（宽 1.8cm）。**小车巡线的黑线就是 AB/BC/CD/DA 四段首尾相接形成的封闭路径**：A→B 直线段（1.5m）、B→C 半圆弧（半径 0.5m）、C→D 直线段（1.5m）、D→A 半圆弧（半径 0.5m）；BC + DA 合起来是一个整圆
- **标记点**：
  - **A/B/C/D** 是**黑线（巡迹路径）**上的点（**和小车相关**）：A 在边线上，B 在 AB 直线段末端，C 在 BC 半圆弧末端（= CD 起点），D 在 CD 直线段末端；具体位置见图 2 赛道示意图
  - **O 点** 是**摆杆水管**的中点（**和钢球相关**），是球的初始/参考位置
- **小球**：直径约 1cm 钢球，**球表面不允许喷涂任何颜色**
- **摄像头（PDF 允许固定在摆杆凹槽上方或车体顶部；本项目设计：相对车身固定，不装摆杆上）**：
  - **设计理由**：相对车身固定 → **排除摆杆倾斜对识别的影响**（否则摆杆一动摄像头跟着动，钢球坐标会混入摆杆抖动），简化识别
  - **OpenART Plus（识别端）**：固定在**车体上**（**不装摆杆**），**俯视**拍摄摆杆区域，**只负责色块识别/TFLite 检测钢球位置**，通过 UART 发送给 STC32（**不负责实时显示与录像**）
  - **OMV-RT5（图传 + 显示 + 录像端，对应 PDF 任务 1）**：稳固安装在小车上作为**发送模块**，AP MJPEG 推流；接收模块（手机/PC）连显示存储装置置环形线路外，**实时显示钢球在凹槽中滚动画面 + 完整记录每次测试视频 + 按要求回放**（6 分）
- **按键 + 显示装置**：小车必须有**启动按键** + **≤2 英寸显示屏**，按键启动时计时系统开始计时并显示时间
- **小车**：长宽 ≤35cm × 25cm，电赛专用车模，电池供电
- **摆杆角度控制机构**（PDF 图 2 关键部件，官方名称）：摆杆**左端用铰链/合页固定**在车体平板上方（被动铰接），**右端连接摆杆角度控制机构**驱动以改变摆杆倾角；倾角大小决定钢球在凹槽内的加速度方向和大小（重力驱动）

### 任务要求（按 PDF 原文，8 项顺序一致）

| 序号 | 任务 | 时间限制 | 精度 | 分值 |
|------|------|----------|------|------|
| **1** | **安装摆球位置监测图传装置**：发送模块稳固装车上，接收模块连显示存储装置置环形线路外；实时显示钢球在凹槽中滚动画面 + 完整记录每次测试视频且能按要求回放 | — | 实时显示+录像+回放 | **6** |
| **2** | 小车置于 A 点，按键启动后沿黑线顺时针行驶一圈并**停到 A 点**；计时停止并显示行驶总时间 | ≤20s | 停车偏差 ≤2cm | **16** |
| **3** | **小车静止**，摆杆控制装置控制小球从 O 往 +5cm 运行，到达后折返到 -5cm 并稳定在该点附近 | ≤5s | ±5cm 处误差 ≤1cm | **13** |
| **4** | 小车置于 A 点，钢球置于中心点 O；按键启动后沿黑线顺时针行驶并**通过 B 位置** | AB 间 ≤8s | 行驶中球稳 O，误差 ≤1cm | **20** |
| **5** | 小车置于 A 点，钢球置于中心点 O；按键启动后沿黑线顺时针行驶一圈并**通过 A 位置** | 整圈 ≤30s | 行驶中球稳 O，误差 ≤1cm | **20** |
| **6** | 小车置于 A 点，**钢球置于摆杆任意指定位置**；按键启动后沿黑线顺时针行驶一圈并**通过 A 位置** | 整圈 ≤30s | 行驶中球稳任意指定位置，误差 ≤1cm | **20** |
| **7** | 其他 | — | — | **5** |
| **8** | 设计报告 | — | — | **20** |

**总分**：6 + 16 + 13 + 20 + 20 + 20 + 5 + 20 = **120 分**

### 摆杆角度控制机构（关键部件，PDF 官方名称）

根据 PDF 图 2，摆杆角度控制机构是控制摆杆倾角、进而控制钢球在凹槽内滚动的核心执行部件：

| 部件 | 位置 | 作用 |
|------|------|------|
| **合页/铰链（被动铰接）** | **摆杆左端** | 把摆杆左端固定在车体平板上方（**h≥5cm**），允许该端转动但不移动 |
| **摆杆角度控制装置（主动驱动）** | **摆杆右端** | 推杆伸缩 → 推动摆杆右端上下 → 摆杆绕合页轴倾斜 |
| **摆杆本体** | 中间 | 一根 PPR 水管**对半切开**（剖面 = 半圆弧形凹槽，25cm 长），钢球在凹槽内沿倾斜方向滚动 |

**控制原理**：

```
角度控制装置推杆伸出 → 摆杆右端抬高 → 摆杆绕左端合页轴倾斜 → 钢球在重力作用下向低侧（左/右）滚动
角度控制装置推杆缩回 → 摆杆右端降低 → 摆杆反向倾斜 → 钢球反向滚动
角度控制装置保持 → 摆杆水平 → 钢球保持当前位置（无加速度）
```

**STC32 驱动**：通过电机/PWM 控制角度控制装置的伸缩量（**开环，无编码器反馈**）→ 间接控制摆杆倾角 → 钢球位置由摄像头反馈闭环

> **注意**：摆杆倾斜角度是系统的**唯一控制量**——钢球既不会自己动，也不会被推，只能靠摆杆倾斜产生的重力分量驱动滚动。所有"球从 O 到 B/P"、"球稳定在 O"等任务，最终都落到"控制摆杆倾角随时间变化"这一个执行动作上。

### 详细规则（按 PDF 二、要求 7 条）

#### 1. 赛道场地

- 用**白色广告布喷绘**制成，环形线路为**黑线**
- 黑线**线宽 1.8±0.2cm**
- AB、CD 段各长 **1.5m**
- BC、DA 段半径 **0.5m** 半圆弧
- 整体幅面满足小车行驶；测试中**允许自带场地**

#### 2. 停车点 A

- A 点有**长度 5cm、线宽 1.8±0.2cm** 的与环路**垂直居中黑色启停线**
- 启停线沿中心轴线绘制**宽 0.1cm、长 30cm 的停车基准虚线**，便于判断停车位置

#### 3. 小车规格

- 车身尺寸**长宽 ≤35cm × 25cm**
- **轮式驱动**，**车载电池供电**
- 中心轴线上指定**唯一测试位置**（用于判断到达和起始位置偏差）
- **整个摆杆不超出车身**

#### 4. 行驶过程约束

- **不得人为干涉、遥控**小车运动
- 行驶中小车**投影必须在轨迹线**上
- 投影完全脱离轨迹线 → 此次测试**失败**
- **停车偏差** = 小车上指定测试位置与基准线（点）的绝对距离

#### 5. 循迹模块 + 显示装置

- 循迹**只能使用红外光电模块**，**数量不限**
- 必须有**启动按键** + **显示装置**
- 按键启动时**计时系统开始计时并显示时间**
- 显示屏尺寸**≤2 英寸**，安装在方便观测位置

#### 6. 图传装置（任务 1）

- 发送模块**稳固安装于车体**
- **摄像头可固定在摆杆凹槽上方**（PDF 允许）；**本项目设计：摄像头相对车身固定，不装摆杆上**，目的是排除摆杆倾斜对识别的影响
- 回传画面**覆盖整个摆杆**，能完整清晰看到钢球滚动轨迹并判定位置
- 接收模块 + 显示存储装置（PAD/笔记本电脑等）比赛结束后**与作品一起封存**

#### 7. 摆杆结构（PDF 图 3）

- 由 **25cm PPR 水管（4 分，外径 2cm × 壁厚 0.34cm）** 对半切开改造
- 槽内放**直径约 1cm 钢球**
- **整个摆杆平直**，**凹槽内壁表面光滑保持原状**（钢球可灵活滚动）
- 内壁有凹坑或做增摩擦改造 → **不予测试**
- 凹槽剖面**边沿**贴**刻度线**（**不允许贴凹槽内**），**刻度间距 0.1cm**
- **凹槽内钢球位置必须用摄像头检测**
- 截面：**外圆弧半径 ≈1cm，内圆弧半径 ≈0.65cm**

### 评分参考案例

#### 案例：BC 段球不稳 + 停车偏差 5cm（任务 2/4/5）

**场景**：

```
车 A 发车 → 球稳在 O 通过 A 点 → AB 段球稳 → BC 段小球不稳 → ...
→ 停车在 A 点前 5cm（停车偏差 = 5cm）
```

**逐项评分**（满分 16 + 20 + 20 = 56）：

| 任务 | 要求 | 实际 | 严格评分 | 较宽松评分 |
|------|------|------|---------|-----------|
| **任务 2** 一圈停车 | ≤20s + 停车偏差 ≤2cm | 偏差 5cm ❌ | **0** | ~8（完成一圈部分分） |
| **任务 4** A→B 通过 | AB 间 ≤8s + AB 段球稳 O | AB 段球稳 ✅ | **20** | 20 |
| **任务 5** 一圈 + 球稳 O | 整圈 ≤30s + **全程**球稳 O | BC 段不稳 ❌ | **0** | ~4~8（完成一圈部分分） |
| **小计** | — | — | **20** | **32~36** |

**关键认知**（避免新人/AI 误读）：

1. **"行驶过程中"= 全程**，不只是终点对就行 — BC 段一个不稳就毁掉任务 5
2. **任务 4 只考 AB 段**，BC 段翻车**不影响**任务 4 得分（任务颗粒度关键）
3. **停车偏差** 与 **球稳** 是**两个独立扣分项**，可同时不达标
4. **电赛惯例**：核心要求（精度/时间/球稳）不达标 → 该项 0 分，无部分得分（除非评委会酌情）

**实战意义**：

- 调试优先级：**球稳 > 时间 > 停车偏差**（球稳一旦失分，36 分瞬间蒸发）
- BC/DA 圆弧段单独调一组 PID 参数（过弯减速）

### 芯片分工

| 芯片 | 职责 |
|------|------|
| **OMV-RT5 (RT1062)** | WiFi AP MJPEG 图传，AP 热点供操作手查看（**仅图传，不参与识别**） |
| **OpenART Plus (RT117x)** | 负责所有视觉识别：从上方俯视拍摄摆杆区域，**色块识别/TFLite 检测钢球位置**，UART 输出给 STC32（**不负责显示/录像，那是 OMV-RT5 的活**） |
| **STC32G144K246 (C251)** | 主控：通过 UART 接收 OpenART Plus 检测结果 → PID 控制 → 电机/舵机输出；USB-CDC 调试 |

> **钢球不可涂色** → 需模型或灰度检测。**摄像头俯视**，色块识别/`find_blobs` 在摆杆 ROI 内识别钢球（球在水管背景上为暗色圆形，对比度高），比 TFLite 快一个数量级。**摆杆角度没有反馈**——伸缩装置无编码器，是开环控制（给多少就是多少），STC32 通过 PID 输出伸缩指令控制钢球位置。

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

OurProject/         ← 当前 STC32 工程（在此开发，why-456 维护）
  libraries/           ← 库文件（从 STC32 库复制）
  project/
    WcLibraries/       ← 用户封装库（WcTFT180 安全屏幕接口等）
    code/              ← 用户外设驱动代码
    mdk/               ← MDK 工程文件
    user/              ← main.c / isr.c / isr.h

docs/                 ← 项目文档
  stc32/
    README.md           ← STC32 系统总览（硬件资源/引脚分配/参考资料）
    八路红外循迹.md       ← 八路红外循迹模块详解（原理/算法/校准/排错）

_archive/              ← 存档（M0项目/K230项目/模型项目）
```

## 系统架构

```
[OMV-RT5 (RT1062)]      [OpenART Plus (RT117x)]     [8路红外循迹]
  WiFi AP MJPEG 图传      色块/TFLite 球       IRPHOTO
  AP → 手机/PC查看         UART12 → STC32                  直连 GPIO
       │                          │                          │
       │ WiFi（操作手查看）        │ UART（检测结果）         │ GPIO(加权求error)
       ↓                          ↓                          ↓
    [操作手]              ┌─────────────────────────────────┐
                           │  STC32G144K246 (C251 @ 96MHz)  │
                           │                                │
                           │  IRPHOTO.c: 加权算偏差+停车检测 │
                           │  control.c: 差速转向            │
                           │  Motor.c + PID.c: 编码器闭环    │
                           │  WiFi SPI: 逐飞助手协议调试     │
                           └─────────────────────────────────┘
```

- **OMV-RT5**：WiFi AP MJPEG 图传（不参与检测，仅供操作手查看）
- **OpenART Plus**：负责识别——色块识别/TFLite 检测钢球位置（俯视）
- **IRPHOTO**：8 路红外光电管，加权求和计算巡线偏差 + 停车标志检测
- **STC32**：通过 UART 接收 OpenART Plus 检测结果 → 控制 → 输出，WiFi SPI 发送调试数据到逐飞助手

> **注意**：OMV-RT5 仅图传，不参与识别；OpenART Plus 负责所有视觉识别。

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

> **STC32 维护者：why-456**。当前代码已实现 MT9V03x 直连、OTSU 钢球检测、八路红外循迹、编码器 PID 闭环、WiFi SPI 调试。

## STC32 编程约定

### 模块总览

| 文件 | 功能 | 说明 |
|------|------|------|
| `main.c` | 主程序 | WiFi SPI → 摄像头初始化 → 逐飞助手 → 循环采集检测发送 |
| `camera.c/h` | 灰度摄像头 | MT9V03x DMA 采集、裁切、**OTSU 二值化找钢球**（高亮中心+暗环） |
| `IRPHOTO.c/h` | 八路红外循迹 | 8 路 GPIO 输入，加权求和算偏差，停车标志检测 |
| `Motor.c/h` | 电机+编码器 | PWM 驱动 + 编码器读取，PIT 定时器触发 PID 更新 |
| `PID.c/h` | PID 控制器 | 增量式 PID，双电机速度闭环 + 舵机差速转向 |
| `control.c/h` | 转向控制 | 差速转向（根据偏差调节左右轮目标速度） |
| `isr.c/h` | 中断服务 | GPIO/UART/DMA/Timer 中断向量表，摄像头 VSYNC+DMA 回调 |
| `WcMenu.c/h` | 栈式菜单系统 | 按键导航、逐项滚动、回调执行，基于 WcTFT180 显示 |

### 八路红外循迹 (IRPHOTO)

```c
// 8 路红外传感器读取，返回加权偏差
int calc_error(int s[8]);  // s[i]=1 表示检测到黑线
// 权重: {-7, -5, -3, -1, 1, 3, 5, 7}，偏差=Σ(s[i]*weight[i])
// 正值偏右，负值偏左，0 居中

// 停车标志检测
int is_stop(int s[8]);  // ≥3 个连续传感器检测到黑线 → 返回 1（停车）
                        // 全部未检测到（冲出赛道保护）→ 返回 1
```

### 灰度摄像头 (camera.c)

```c
#define CROP_MAX_ROWS  20   // 裁切行数（ROI 区域）
#define CROP_MAX_COLS  188  // 裁切列数

uint8 camera_copy[120][188];          // DMA 采集的完整帧
uint8 camera_crop[CROP_MAX_ROWS][CROP_MAX_COLS]; // 裁切后 ROI

void camera_copy_image(void);  // 等 mt9v03x_finish_flag 后拷贝到 camera_copy
void camera_crop_image(row_start, rows, col_start, cols); // 裁切 ROI

// OTSU 大津法自适应阈值 → 找最亮点（高光中心）→ 8 方向采样暗环验证
// 返回 1=找到，*x=列坐标，*y=行坐标
uint8 find_ball(uint8 *x, uint8 *y);
```

OTSU 钢球检测原理：钢球在灰度图像中呈现「中心高亮 + 环形暗区」特征：
1. 大津法计算整帧自适应阈值
2. 找最亮点（>150 灰度），8 方向采样（半径 4px）
3. ≥5 个方向低于阈值 → 确认是钢球

### 电机 + PID (Motor.c + PID.c)

```c
// ---- 硬件引脚 ----
// 电机1: DIR=IO_P74, PWM=PWMB_CH2_P75, 编码器=PWMA P60/P62
// 电机2: DIR=IO_P76, PWM=PWMB_CH4_P77, 编码器=PWMC P40/P42

// ---- PID 结构体 ----
typedef struct {
    int16 Target, Actual, Out;
    int16 Kp, Ki, Kd;
    int16 Error0, Error1, Error2;
    int16 OutMax, OutMin;
} PID_t;

void Motor_Init(void);          // PWM 17kHz, GPIO 方向引脚
void encoder_init(void);        // 编码器正交解码初始化
void motor1_control(int16 d);   // 正转/反转 + PWM 占空比
void motor2_control(int16 d);
void PID_Update(PID_t *p);      // 增量式 PID 更新

// PIT 定时器中断回调 (pit_handler):
//   读编码器计数 → 清零 → 作为 PID.Actual → PID_Update → 输出到电机
```

### 转向控制 (control.c)

```c
void steer_init(void);  // 初始化转向 PID (Kp=5, OutMax=±20)
void steer_set(int error, int base_speed);
// error = 红外循迹偏差，base_speed = 基础速度
// 左轮目标 = base_speed - steer_pid.Out
// 右轮目标 = base_speed + steer_pid.Out
```

### main.c 启动流程

```c
void main(void) {
    clock_init(SYSTEM_CLOCK_96M);
    debug_init();

    // 1. WiFi SPI 连接
    wifi_spi_init(WIFI_SSID, WIFI_PASSWORD);
    wifi_spi_socket_connect("TCP", TARGET_IP, TARGET_PORT, LOCAL_PORT);

    // 2. 摄像头初始化
    mt9v03x_init();

    // 3. 逐飞助手协议（WiFi 传输调试画面+数据）
    seekfree_assistant_interface_init(SEEKFREE_ASSISTANT_WIFI_SPI);
    seekfree_assistant_camera_information_config(
        SEEKFREE_ASSISTANT_MT9V03X, camera_crop[0], CROP_MAX_COLS, CROP_MAX_ROWS);
    seekfree_assistant_camera_boundary_config(
        XY_BOUNDARY, 1, &ball_x, NULL, NULL, &ball_y, NULL, NULL);

    while(1) {
        camera_copy_image();                    // 拷贝 DMA 帧
        camera_crop_image(50, 20, 0, 188);      // 裁切 ROI（行50~69, 全列）
        find_ball(&ball_x, &ball_y);            // OTSU 找球
        seekfree_assistant_camera_send();       // 发送到逐飞助手
    }
}
```

### 常用库 API

| 功能 | 函数 | 所属模块 |
|------|------|----------|
| 时钟 | `clock_init(SYSTEM_CLOCK_96M)` / `SYSTEM_CLOCK_120M` | zf_common |
| 延时 | `system_delay_ms(ms)` / `system_delay_us(us)` | zf_driver |
| GPIO | `gpio_init(pin, dir, dat, mode)` / `gpio_set_level(pin, dat)` / `gpio_get_level(pin)` / `gpio_low(pin)` / `gpio_high(pin)` | zf_driver |
| PWM | `pwm_init(ch, freq, duty)` / `pwm_set_duty(ch, duty)` | zf_driver |
| 编码器 | `encoder_dir_init(enc, pulse_pin, dir_pin)` / `encoder_get_count(enc)` | zf_driver |
| 摄像头 | `mt9v03x_init()` / `mt9v03x_finish_flag` / `mt9v03x_image[][]` | zf_device |
| WiFi SPI | `wifi_spi_init(ssid, pwd)` / `wifi_spi_socket_connect(...)` | zf_device |
| 逐飞助手 | `seekfree_assistant_interface_init(...)` / `seekfree_assistant_camera_send()` | zf_components |
| 调试输出 | `debug_init()` / `debug_send_buffer(buff, len)` / `debug_read_buffer(buff, len)` (USB-CDC) | zf_common |

### TFT180 显示屏 (zf_device_tft180)

1.8 寸 RGB_TFT，驱动芯片未知（非 ST7735/ST7789），SPI_2 CH4，RGB565 格式。

**坐标系统（关键！容易出错）**：

`tft180_set_dir()` 根据方向设定 `tft180_x_max` / `tft180_y_max`：

| 方向 | `dir < 2` (PORTAIT/180) | `dir >= 2` (CROSSWISE/180) |
|------|--------------------------|-----------------------------|
| x_max（宽） | **128** | 160 |
| y_max（高） | **160** | 128 |
| 字符/行 | 16 | 20 |
| 行数 | 10 | 8 |

> **注意**：`tft180_init()` 默认调用 `set_dir(TFT180_PORTAIT)` → x_max=128, y_max=160。**所有坐标必须满足 `x < tft180_x_max` 且 `y < tft180_y_max`**，否则触发 `zf_assert`。

**引脚**：

| 信号 | STC32 引脚 | 宏 |
|------|-----------|-----|
| SCL | P83 | `SPI2_CH4_SCLK_P83` |
| SDA | P81 | `SPI2_CH4_MOSI_P81` |
| RES | P70 | `TFT180_RES_PIN (IO_P70)` |
| DC | P71 | `TFT180_DC_PIN (IO_P71)` |
| CS | P35 | `TFT180_CS_PIN (IO_P35)` |
| BL | P82 | `TFT180_BL_PIN (IO_P82)` |

**API**：

```c
void tft180_init(void);
void tft180_clear(uint16 color);
void tft180_set_dir(tft180_dir_enum dir);        // PORTAIT / PORTAIT_180 / CROSSWISE / CROSSWISE_180
void tft180_set_color(uint16 pen, uint16 bgcolor);
void tft180_draw_point(uint16 x, uint16 y, uint16 color);
void tft180_show_char(uint16 x, uint16 y, const char dat);      // 8×16 字符
void tft180_show_string(uint16 x, uint16 y, const char dat[]);  // 横向排列
void tft180_show_int8/uint8/int16/uint16(uint16 x, uint16 y, ...);
void tft180_show_int32(uint16 x, uint16 y, int32 dat, uint8 num);
void tft180_show_float(uint16 x, uint16 y, double dat, uint8 num, uint8 pointnum);
void tft180_show_gray_image(uint16 x, uint16 y, const uint8 *img, uint16 w, uint16 h, uint16 dw, uint16 dh, uint8 threshold);
```

**已知陷阱**：

| 陷阱 | 错误写法 | 正确写法 |
|------|---------|---------|
| **背光控制不能用宏** | `TFT180_BL(0)` / `TFT180_BL(1)` | `gpio_set_level(TFT180_BL_PIN, 0/1)` |
| **串口无 write_string** | `debug_write_string("...")` | `debug_send_buffer((uint8*)"...", len)` |

> `TFT180_BL(x)` 宏展开为 `P82 = x`，但 STC32 上 **P8 端口不是位可寻址 SFR**（仅 P0-P3 是），编译报错 `undefined identifier 'P82'`。必须用 `gpio_set_level` 替代。

### Keil 工程管理

`.uvproj` 是 XML 文件，新增 `.c/.h` 不会自动注册。添加文件有两种方式：

1. **Keil IDE**：右键 `user` 组 → Add Existing Files
2. **手动编辑** `.uvproj` XML，在对应 `<Group>` 内插入：
```xml
<File>
  <FileName>xxx.c</FileName>
  <FileType>1</FileType>         <!-- 1=C源文件, 5=头文件 -->
  <FilePath>..\user\xxx.c</FilePath>
</File>
```

路径从 `project/mdk/` 出发，`..\user\` 解析到 `project/user/`。

### WcTFT180 安全封装（`project/WcLibraries/`）

**为什么需要**：原生 `tft180_show_string` 在字符串超出屏幕右边界时会触发 `zf_assert` 死机（T3 测试已踩坑）。

**核心安全机制**：
- 所有显示函数调用前计算 `(tft180_x_max - x) / 8`，只写入能容纳的字符数
- 越界坐标静默丢弃，不 crash
- 向右超出自动截断，向下超出自动换行
- 数字格式化为字符串后再安全显示（不会因 `num` 参数设错而溢出）

**两种使用模式**：

```c
// 模式 A：无光标（推荐用于调试输出、固定布局）
WcTFT_PrintAt(0, 16, "Sensor OK");          // 指定位置，不跟踪光标
WcTFT_PrintIntAt(80, 16, 42);               // 数字同理
WcTFT_PrintCenter(3, "Title");              // 居中
WcTFT_PrintRow(1, "label", "value");        // "label: value" 格式

// 模式 B：光标模式（推荐用于日志流输出）
WcTFT_GotoRow(0);
WcTFT_Print("Temp = ");
WcTFT_PrintFloat(25.3, 1);
WcTFT_Newline();
WcTFT_Print("Loop: ");
WcTFT_PrintInt(cnt);
```

**完整 API**：见 `WcTFT180.h` 注释，主要包括 `WcTFT_Init / Clear / Backlight / SetColor / Print / PrintAt / PrintCenter / PrintRow / PrintInt / PrintFloat / PrintIntAt / PrintFloatAt / Goto / GotoRow / Newline / Tab / DrawPoint / DrawHLine / DrawVLine / DrawRect / FillRect / GetMaxCols / GetMaxRows`。

## OMV-RT5 编程约定

### WiFi MJPEG 图传（对应 PDF 任务 1）

OMV-RT5 作为**发送模块**稳固装在车上，仅负责实时图传 + 录像，不参与检测：

- 运行 `main.py`：WiFi AP 模式，单页应用 MJPEG 推流
- 热点：`OMVRT5` / `12345678`，地址 `http://192.168.4.1:8000/`
- 分辨率 320×240，JPEG 质量 30
- 不主动 GC，不显示 FPS，不与 STC32 通信
- 接收模块（手机/PC + 显示存储装置）置环形线路外，**实时显示钢球在凹槽中滚动画面** + **完整记录每次测试视频** + **按要求回放**

### OMV-RT5 引脚 (RT1062)

| 功能 | 引脚 | 备注 |
|------|------|------|
| LED | LED1~4 | 板载 |
| UART1 | TX=P1_01, RX=P1_02 | 未使用（不与 STC32 通信） |
| WiFi | 内置 | AP 模式 |

---

## OpenART 编程约定

OpenART Plus 摄像头从车体上方**俯视**拍摄摆杆区域，**只负责识别钢球位置**：

> **摆杆角度无反馈**——伸缩装置无编码器，是开环控制；PID 只能靠摄像头给的钢球位置反馈来调整伸缩指令。

### 钢球检测

钢球（直径 ~1cm）在水管内滚动，不可涂色。两种可选方案：

| 方案 | 方法 | 优点 | 缺点 |
|------|------|------|------|
| **TFLite 模型** | `tf.detect()` 目标检测 | 准确率高，已有训练好的模型 | 速度较慢（~50-100ms） |
| **灰度色块** | `find_blobs` 灰度阈值 | 速度快（~5ms），球在浅色水管上对比度高 | 可能受光照干扰 |

- TFLite 模型: `_合并-2tflite.tflite` (YOLOv3 MobileNetV2, INT8, 112×112, 含 NMS)
- 灰度方案：球在白色/绿色水管背景上为暗色圆形，ROI 限制在摆杆区域后 `find_blobs` 灰度阈值即可检出

```python
# 俯视，钢球在水管背景上呈暗色圆形
# 灰度阈值（球比水管暗，需实地校准）
BALL_GRAY_THRESHOLD = [(0, 60)]   # 灰度 < 60 视为球像素

# ROI 限制在摆杆区域，避免背景干扰
roi = (x, y, w, h)  # 摆杆 ROI 矩形

blobs = img.find_blobs(BALL_GRAY_THRESHOLD, roi=roi,
                       pixels_threshold=50, area_threshold=20)
for b in blobs:
    ball_x = b.cx()
    ball_y = b.cy()  # 在水管上 = 球相对 O 点的位置（cm）
```

### UART 输出格式

OpenART 通过 UART12 发送钢球位置给 STC32（115200bps）：

```
B,cx,cy,score\n           ← 钢球在摆杆上的位置 (像素坐标, 置信度)
N\n                       ← 未检测到
```

> 坐标原点建议以摆杆中心 O 为参考，STC32 接收到后做 PID 控制（PID 输入：球位置 → 输出：伸缩装置指令（开环） → 摆杆倾斜 → 球在重力下向目标位置滚动）。

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
