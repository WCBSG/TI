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

> **设计报告（任务 8）由其他成员负责，本项目不处理**——本仓库只交付硬件方案与代码。

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

> **设计报告（任务 8）由其他成员负责，本项目不处理。**

> **钢球不可涂色** → 需模型或灰度检测。**摄像头俯视**，色块识别/`find_blobs` 在摆杆 ROI 内识别钢球（球在水管背景上为暗色圆形，对比度高），比 TFLite 快一个数量级。**摆杆角度没有反馈**——伸缩装置无编码器，是开环控制（给多少就是多少），STC32 通过 PID 输出伸缩指令控制钢球位置。

## 目录结构

```
OPenMV RT5/           ← 图传端 MicroPython 代码（OpenMV IDE 开发；芯片型号 OMV-RT5）
  main.py               ← WiFi AP MJPEG 图传（精简版，对应任务 1）
  omv-rt-visual_module/ ← 厂商资料（例程/固件/手册，不纳入版本控制）

OpenART Plus/          ← 识别端 MicroPython 代码（OpenMV IDE 开发）
  main.py              ← 主程序：TFLite 模型钢球检测 + UART 输出
  cmm_cfg.csv          ← 引脚映射配置（LED/UART/I2C/SPI/ADC/PWM）
  cmm_load.py          ← 引脚映射加载器
  stubs/               ← Python 类型存根（sensor/image/machine/time/cmm）
  [例程]OpenART Plus例程/  ← 官方例程（AI模型/apriltag/基础外设/外置外设）

OurProject/         ← 当前 STC32 工程（在此开发，why-456 维护）
  libraries/           ← 逐飞官方库（zf_common/driver/device/components）
  project/
    WcLibraries/       ← 用户封装库（WcTFT180/WcMenu/protocol 等）
    mdk/               ← MDK 工程文件（seekfree.uvproj）
    user/              ← 应用代码：main/IRPHOTO/Motor/PID/line_ctrl/ball_ctrl/task_sched/imu_ctrl/config/menu_defs

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
                           │  line_ctrl.c: 差速开环巡线     │
                           │  ball_ctrl.c: 舵机球稳闭环     │
                           │  imu_ctrl.c: 陀螺仪阻尼        │
                           │  task_sched.c: 任务2/3/5/6框架 │
                           │  USB-CDC: 串口调试帧           │
                           └─────────────────────────────────┘
```

- **OMV-RT5**：WiFi AP MJPEG 图传（不参与检测，仅供操作手查看）
- **OpenART Plus**：负责识别——色块识别/TFLite 检测钢球位置（俯视）
- **IRPHOTO**：8 路红外光电管，加权求和计算巡线偏差 + 停车标志检测
- **STC32**：通过 UART 接收 OpenART Plus 检测结果 → 差速开环/球稳闭环 → 输出，USB-CDC 串口调试帧

> **注意**：OMV-RT5 仅图传，不参与识别；OpenART Plus 负责所有视觉识别。

## 开发环境

| 端 | IDE | 语言 | 编译器/运行时 |
|---|---|---|---|
| OMV-RT5 | OpenMV IDE | MicroPython | MicroPython on RT1062 |
| OpenART Plus | OpenMV IDE | MicroPython | MicroPython on RT117x |
| STC32 | MDK for C251 V5.60 | C (C251) | Keil C251 |

- **VSCode 设置**：在 `.vscode/settings.json` 项目级修改，不要改用户级
- **VSCode 导航**：`#include` 行上的文件名用 Ctrl+单击跳转；函数/宏/类型用 F12
- **STC32 下载**：按住 P32 上电进入 USB 下载模式（无需专用下载器）；更推荐 **AiCube-ISP MCP 自动烧录**（见下）
- **STC32 头文件路径**：限定在 `OurProject/libraries` 与 `OurProject/project` 内，避免多副本同名文件导致跳转歧义
- **源码编码：全库 UTF-8 统一**（libraries/user/WcLibraries 已从 GB2312 转 UTF-8，编译不受影响，仅注释编码）。**坑**：Keil（MDK）编辑器默认按 GBK 解析，UTF-8 注释在 Keil 里显示乱码——需 Edit → Configuration → Editor → Encoding 改 UTF-8（VSCode 无此问题）。新增/修改源码保持 UTF-8 无 BOM，勿混入 GB2312
- **⚠️ 调试输出：一定不要吝啬 debug 消息**（用户强调）。任何测试/标定功能必须**过程中持续回显**关键状态（每 100ms 一帧：YAW/里程/差速/阶段等），不能只结束时回显一次——否则上位机无法判断"是否偏了/闭环是否生效"，只能盲调。曾因 CALM 标定只结束回显一次、且过程死循环无回显，导致多次"试了好久没反应"。**串口调试帧、回显消息永远多发、发全**

> **STC32 维护者：why-456**。

#### AiCube-ISP MCP 自动烧录（推荐，已打通）

`project/AiCube-ISP-v6.96Z.exe` 内置 **STC-MCP 服务**（`http://localhost:8051/mcp`，JSON-RPC 2.0 over HTTP POST），AI 可操控其 GUI 完成烧录：

1. 启动工具 → MCP 服务自动拉起（`curl http://localhost:8051/` 返回帮助页即成功）
2. `tools/call(list_windows)` 发现主窗口 hwnd → `list_controls` 拿各控件句柄
3. 点"打开程序文件" → 文件对话框弹出 → **文件名注入用 Win32 `WM_SETTEXT`**（MCP 工具集无文本输入）→ 点"打开"
4. 点"下载/编程" → 端口框选 `(HID1) USB-Writer`（芯片自动进入下载模式）→ 日志确认 `操作成功`

**坑**：
- **hwnd 每次重启/重开都会变**，必须重新 `list_windows` 发现，不可复用旧句柄
- MCP 只操控宿主程序自己的窗口；**无键盘/文本输入工具**，文件对话框填路径靠 `WM_SETTEXT`
- 芯片处于下载模式时 USB 枚举为 `USB-Writer`（HID），不是 COM 口；COM 口是运行时 USB-CDC 的

## 当前架构状态（2026-08）

| 模块 | 状态 |
|------|------|
| 任务框架 task_sched（2/3/5/6） | ✅ 就绪（5/6 复用巡线+球稳引擎，`DEBUG_LINE_FOREVER` 调试中） |
| 差速开环巡线 line_ctrl | ✅ 就绪（弯道自适应减速 + KdYaw 陀螺仪阻尼，待真机调参） |
| 球稳环 ball_ctrl（舵机摆杆） | ✅ 就绪（300Hz 舵机，标定参数 servo_center_duty/pixel_zero/px_per_cm 待现场标） |
| IMU660RA 陀螺仪 imu_ctrl | ✅ 就绪（硬件 SPI3，X 轴 yaw，开机静止 1s 标定，转一圈验证 354°） |
| 4 键按键 KEY.c | ✅ 就绪（b2/b3/b4/P3.2，20ms 去抖 + 长按重复，修复按一次识别两次） |
| 菜单 menu_defs | ✅ 已简化（删失效 Send 命令；Spd/Led 直接编辑；Protocol→Ball 调试页） |
| 红外循迹 IRPHOTO | ✅ 引脚改为 P94/P54/P93/P92/P14/P11/P07/P06（与 IMU SPI3 错开） |
| 图传 OMV-RT5 | ✅ MJPEG 推流重写（帧半截不丢/多客户端/排空 accept，浏览器画面已修复） |
| 源码编码 | ✅ 全库（libraries/user/WcLibraries）UTF-8 统一，编译 0 Error 0 Warning |
| 串口命令 cmd_ctrl | ✅ 上位机发 T2/IR 等命令启动测试（USB-CDC 双向，用户只负责烧录放车） |
| 调试 | ✅ USB-CDC 一行一帧调试数据 + tools/serial_monitor.py 监视器 |
| 差速转向 | ✅ 实测修正：`Actual=+error`（曾两次误判 -error，勿再改） |
| 真机调参 | ⚠️ 进行中（Kp=18/Kd=25 待实测，KdYaw/球稳标定/停车判定待开） |

## STC32 编程约定

### 模块总览

| 文件 | 功能 | 说明 |
|------|------|------|
| `main.c` | 主程序 | 三态循环：菜单 ⇄ 任务 ⇄ 结果（按键导航 + Launch 启动任务） |
| `KEY.c/h` | 4 键按键 | b2/b3/b4/P3.2，释放沿触发 + 长按自动重复 + 20ms 去抖 |
| `IRPHOTO.c/h` | 八路红外循迹 | 8 路 GPIO 输入，加权偏差，停车标志检测 |
| `Motor.c/h` | 电机+编码器 | PWM 驱动 + 编码器采样，PIT 5ms 时基（`pit_tick` 计数） |
| `PID.c/h` | 位置式 PID | **唯一 PID 用于差速**（Target 恒 0）；int32 积分余数消除死区 |
| `line_ctrl.c/h` | 差速开环巡线 | 偏差→左右轮 PWM duty 差；弯道自适应减速；KdYaw 陀螺仪阻尼 |
| `ball_ctrl.c/h` | 球稳环 | 舵机摆杆 + 球位置 PID（OpenART 反馈） |
| `task_sched.c/h` | 任务框架 | 任务 2/3/5/6 调度 + 结果页 + USB-CDC 串口调试帧 |
| `cmd_ctrl.c/h` | 串口命令 | USB-CDC 收命令启动测试（T2/T3/T5/T6/IR/STOP/HELP），ISR 只存字节 |
| `imu_ctrl.c/h` | 陀螺仪 | IMU660RA 硬件 SPI3，X 轴 yaw 积分，开机静止标定 |
| `config.c/h` | 配置持久化 | 32 槽 IAP，XOR 校验 + 边界验证 |
| `protocol.c/h` | OpenART 协议 | UART3 DMA 逐字节接收，解析 `B,<cx>\n` / `N\n` |
| `menu_defs.c/h` | 菜单页面 | 主菜单 4 项：Steer PID(子页)/Spd/Led(直接调)/Ball(调试) + Launch 任务列表 |
| `isr.c/h` | 中断服务 | GPIO/UART/DMA/Timer 中断向量表 |
| `WcMenu.c/h` | 栈式菜单 | 按键导航、逐项滚动、回调执行 |

### 八路红外循迹 (IRPHOTO)

```c
// 引脚（IRPHOTO.c 内静态表，s[i]=1 表示检测到黑线）
// 已与 IMU SPI3 (P87/P85/P86/P34) 完全错开，无引脚冲突
static const gpio_pin_enum ir_pins[8] = {
    IO_P94, IO_P54, IO_P93, IO_P92,
    IO_P14, IO_P11, IO_P07, IO_P06,
};

// 8 路红外传感器读取，返回加权偏差
int calc_error(int s[8]);  // 权重: {-7, -5, -3, -1, 1, 3, 5, 7}，偏差=Σ(s[i]*weight[i])
                          // 正值偏右，负值偏左，0 居中

// 停车标志检测
int is_stop(int s[8]);  // ≥4 个连续传感器检测到黑线 → 返回 1（停车）
                        // 全部未检测到（冲出赛道保护）→ 返回 2（区别于停车）
                        // 正常行驶 → 返回 0
```

### 位置式 PID (PID.c)

```c
typedef struct {
    int16 Target, Actual, Out;
    int16 Kp, Ki, Kd;
    int16 Error0, Error1, Error2;
    int16 OutMax, OutMin;
    int32 Integral;      // int32 余数积分，消除整数除法死区
} PID_t;

void PID_Update(PID_t *p);      // 位置式 PID 更新（见下方逻辑）
```

位置式核心：`Out = Kp*e + Integral/10 + Kd*Δe`，int32 域计算后截断 int16；**饱和时冻结积分（anti-windup）**。当前工程**唯一 PID 实例是差速 `steer_pid`**（Target 恒 0，Error = 0 - 红外偏差），无电机速度闭环。

### 电机 (Motor.c)

```c
// ---- 硬件引脚（2026-08 主板，以用户指定为准） ----
// 电机1 左后轮: DIR=IO_P76, PWM=PWMB_CH4_P77（正向安装，正 duty 前进）
// 电机2 右后轮: DIR=IO_P74, PWM=PWMB_CH2_P75（反向安装，负 duty 前进）
// 编码器=PWMA P60/P62（左）、PWMC P40/P42（右）
// 符号关键：motor1_control(正 left_duty) / motor2_control(-right_duty) 两个都前进
//   （曾因引脚对调+安装方向反转导致倒车+差速反，已修）

void Motor_Init(void);          // PWM 17kHz, GPIO 方向引脚
void encoder_init(void);        // 编码器正交解码初始化
void motor1_control(int16 d);   // 正转/反转 + PWM 占空比
void motor2_control(int16 d);
int16 motor_get_encoder_lr(void);   // 诊断：左轮 5ms 编码器计数
int16 motor_get_encoder_rr(void);   // 诊断：右轮 5ms 编码器计数

// PIT 定时器中断回调 (pit_handler): 仅 pit_tick++ + 编码器采样，不做 PID
```

> **编码器不参与闭环**：差速是开环 PWM，编码器只作诊断显示（串口 E1/E2）。

> **⚠️ 电机符号（已实测校正）**：motor1_control 给**正 duty**（左后轮 P77 正向安装）、motor2_control 给**负 duty**（右后轮 P75 反向安装）才是前进。勿单独改一侧符号。

### 差速开环巡线 (line_ctrl.c)

```c
void line_ctrl_init(void);                       // 差速 PID + 配置加载
void line_ctrl_set(int error, int base_speed);   // 每 10ms：偏差 → 左右轮 duty
```

- `base_speed` 是基准 duty（0-10000 满量程），`steer_pid.Out` 是差速量；`左 = base - Out`，`右 = base + Out`
- **差速转向符号（实测确认，勿改）**：`steer_pid.Actual = error`。线偏左(error<0)→需左转→eff_out<0→左轮快右轮慢→左转 ✓。**曾两次误判为 `-error`，实测方向反（车平稳偏出），已改回 +error**
- **弯道自适应减速**：|error|≤2→100%，≤4→85%，否则 70%
- **差速钳位**：上限 base，下限 `-base/3`（允许内侧轮反转，改善 0.5m 半径弯道）
- **陀螺仪阻尼**：`Out -= kd_yaw * 角速度`，`kd_yaw` 在 Steer PID 菜单第 6 项（默认 0 不生效；同时削弱正常转弯，需现场调小）
- **调参初值**：Kp=18 / Kd=25 / BASE=1500（Kp=40 实测振荡甩尾，降 + 加 Kd 抑制）

### 球稳环 (ball_ctrl.c)

```c
void ball_ctrl_init(void);           // 舵机 PWM + 球 PID + 标定加载
void ball_ctrl_set_target(int16 cm_x10);  // 球目标位置（0.1cm，相对 O）
void ball_ctrl_tick(void);           // 10ms：读球位→cm→PID→舵机
void ball_ctrl_stop(void);           // 舵机回中位
```

- 硬件：舵机 `PWME_CH1P_PA0 @ 300Hz`（与灯带同 PWME 组同频），duty 1500(0.5ms)~7500(2.5ms)，中位 4500（300Hz 仅数字舵机适用）
- 反馈：OpenART 球像素 X → `cm_x10 = (pixel - pixel_zero) * 10 / px_per_cm`
- **丢球保护**：`proto_ball_valid==0` 保持上次输出
- 标定参数：`servo_center_duty / pixel_zero / px_per_cm / ball_target_cm_x10`（config 持久化）

### 任务框架 (task_sched.c)

```c
void task_sched_set(int task_id);   // TASK_2/3/5/6
void task_sched_run(void);          // 阻塞运行所选任务 + 结果页
```

- `line_drive_run(enable_ball, ball_target)`：任务 2/5/6 共用巡线引擎（5/6 并行球稳）
- `task3_run()`：球 O→+5→-5 往返，到位 ±1cm 持续 100ms
- `DEBUG_LINE_FOREVER` 宏：1=关闭脱轨/停车结束判定一直巡线（调试）；调好后改 0
- **任务 2/5/6 结束判定（全开）**：脱轨连续 3 次全灭→FAIL；停车线起步 1s 忽略+连续 3 次确认→OK；30s 超时→TIMEOUT
- 任务运行中每 100ms 发 USB-CDC 调试帧（**一行一帧，空格分隔**）：
  `T=0.1 E=3 IR=00011000 D1=1440 D2=-1500 SO=60 E1=1 E2=-1 BASE=1500`
- **结果页也响应串口命令**：`task_sched_show_result` 循环内调 `cmd_poll()`，收到 T2/T3/... 可立即重跑（不再卡死等按键）

### IMU 陀螺仪 (imu_ctrl.c)

```c
void imu_ctrl_init(void);               // 初始化重试 + 静止 1s 零点标定（失败不阻塞）
void imu_ctrl_tick(uint16 dt_ms);       // 周期调用：读角速度 → yaw 积分
void imu_ctrl_reset_yaw(void);          // 航向清零
extern int16 imu_gyro_dps_x10;          // 0.1°/s（阻尼数据源）
extern int32 imu_yaw_x100;              // 0.01°
```

- 硬件：**IMU660RA 六轴，硬件 SPI3**（SCK=P87 / MOSI=P85 / MISO=P86 / CS=P34，VCC=3V3 GND=地），库默认配置零改动
- **yaw 轴 = X 轴**（模块竖插实测）：`imu_ctrl.c` 里 `IMU_YAW_RAW()` / `IMU_YAW_SIGN` 两个宏，方向反了把 SIGN 改 -1
- 静态零点标定：开机静止采样 200 次求平均偏置，**消除积分漂移**（航向可用性关键）
- 采样放主循环（非中断）：10ms 节拍够航向反馈；避免软硬件 SPI 被高优先级中断打断
- 用途：`kd_yaw` 阻尼（用瞬时角速度，不依赖 yaw 积分精度）

### 4 键按键 (KEY.c)

```c
void button_init(void);
void button_control(uint8 repeat_mask);  // 主循环每 10ms 调用；repeat_mask 按位启用长按重复
// 全局标志：key1_flag/key2_flag/key3_flag/key4_flag（消费后须清零）
```

- 引脚：b2=PB2(上/+)、b3=PB3(下/-)、b4=PB4(确定/编辑)、P3.2=P32(返回)
- **20ms 软件去抖**（`DEBOUNCE_TICKS=2`）：电平变化需连续 2 次采样稳定才确认，**防止机械弹跳跨过采样点产生多个释放沿**（曾导致"按一次识别两次"）
- **长按去重**：进入长按重复后（`repeat_active`）松开不再补触发释放沿——长按一次只出一串重复值
- 自动重复参数：启动 200ms → 初始间隔 250ms → 每次 -50ms → 最小 10ms（线性加速）
- 关键实现细节：释放沿（0→1）触发 flag；**确认按下才累计 hold_ticks**，去抖未确认前跳过重复逻辑

### main.c 三态循环

```c
void main(void) {
    clock_init(SYSTEM_CLOCK_96M);  debug_init();  WcTFT_Init();
    IRPHOTO_Init();  button_init();  Motor_Init();  encoder_init();
    Protocol_Init();
    config_load();  line_ctrl_init();  ball_ctrl_init();
    if (config_valid()) base_speed = flash_buff[5];
    imu_ctrl_init();
    pit_ms_init(PIT_ENCODER, 5, pit_handler);
    Menu_Init();  Menu_Push(&page_main);

    while (1) {
        while (!launch_triggered) {   // [菜单] 4 键导航 + Protocol_ReadBall + 灯带 PWM
            button_control(KEY_REPEAT_KEY1 | KEY_REPEAT_KEY2);
            if (key1_flag) { key1_flag = 0; Menu_Inc(); }      // b2: 上/+
            if (key2_flag) { key2_flag = 0; Menu_Dec(); }      // b3: 下/-
            if (key3_flag) { key3_flag = 0; Menu_Edit(); }     // b4: 确定/编辑
            if (key4_flag) { key4_flag = 0;
                if (Menu_IsTop(&page_main)) { config_save(); Menu_Push(&page_launch); }
                else Menu_Cancel(); }                          // P3.2: 返回
            Protocol_ReadBall(&proto_ball_x);
            pwm_set_duty(LED_PWM_PIN, (uint32)led_duty);
            system_delay_ms(10);
        }
        task_sched_run();             // [任务] 阻塞运行 + 结果页
        launch_triggered = 0;
        Menu_Push(&page_main);        // 回菜单
    }
}
```

### 常用库 API

| 功能 | 函数 | 所属模块 |
|------|------|----------|
| 时钟 | `clock_init(SYSTEM_CLOCK_96M)` | zf_common |
| 延时 | `system_delay_ms(ms)` / `system_delay_us(us)` | zf_driver |
| GPIO | `gpio_init(pin, dir, dat, mode)` / `gpio_set_level(pin, dat)` / `gpio_get_level(pin)` / `gpio_low(pin)` / `gpio_high(pin)` | zf_driver |
| PWM | `pwm_init(ch, freq, duty)` / `pwm_set_duty(ch, duty)` | zf_driver |
| 编码器 | `encoder_dir_init(enc, pulse_pin, dir_pin)` / `encoder_get_count(enc)` | zf_driver |
| PIT 定时 | `pit_ms_init(PIT, ms, callback)` | zf_driver |
| 舵机 | `pwm_init(PWME_CH1P_PA0, 300, 4500)` / `pwm_set_duty(...)` | zf_driver |
| UART | `uart_init(port, baud, tx, rx)` / `uart_write_string(port, s)` | zf_driver |
| IMU660RA | `imu660ra_init()` / `imu660ra_get_gyro()` / `imu660ra_gyro_x/y/z` | zf_device |
| 格式化 | `zf_sprintf(int8*, fmt, ...)` → uint32 长度 | zf_common |
| 调试输出 | `debug_init()` / `usb_cdc_write_buffer(buff, len)` / `usb_cdc_write_string(s)` (USB-CDC) | zf_common |

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
| **zf_sprintf %d 不能直接传 int16** | `zf_sprintf(b, "%d", int16_val)` | `zf_sprintf(b, "%d", (int32)int16_val)` |

> `TFT180_BL(x)` 宏展开为 `P82 = x`，但 STC32 上 **P8 端口不是位可寻址 SFR**（仅 P0-P3 是），编译报错 `undefined identifier 'P82'`。必须用 `gpio_set_level` 替代。

> **`zf_sprintf` 的 `%d` 按 int32 读变参**，而 C251 的 int=16 位。传 int16 时栈上只有 2 字节，`va_arg(int32)` 读到 `0x0000FFFF`——**int16 负数 -1 会被打印成 65535**。所有 `%d` 参数必须显式 `(int32)` 转换（已在 task_sched.c/main.c 串口帧中统一处理；`tft180_show_int16` 等库函数内部也有此问题）。

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

> 芯片型号 OMV-RT5，代码在仓库目录 **`OPenMV RT5/`**。

### WiFi MJPEG 图传（对应 PDF 任务 1）

OMV-RT5 作为**发送模块**稳固装在车上，仅负责实时图传 + 录像，不参与检测：

- 运行 `main.py`：WiFi AP 模式，单页应用 MJPEG 推流
- 热点：`OMVRT-WC` / `12345678`，地址 `http://192.168.4.1:8000/`
- 分辨率 320×240，JPEG 质量 45
- 不主动 GC，不显示 FPS，不与 STC32 通信
- **MJPEG 推流健壮性（已修复，勿回退）**：
  - **帧绝不半截丢弃**：每个客户端持 `{pending, off}` 缓冲，缓冲满（`EWOULDBLOCK`）保留未发完帧下次续发——若中途 return 丢弃，`--frame` 边界与 `Content-Length` 失配会让浏览器 multipart 永久失步 → 画面黑屏
  - **多客户端列表广播**：`clients` 字典存所有 `/stream` 连接，不互杀（原版新连接 `close(stream)` 会杀掉活跃流，首帧未发即断）
  - **排空 accept 队列 + `listen(8)`**：每循环处理所有待连连接，避免 backlog 溢出 RST `/stream`（浏览器开页面会同时连 `/`、`/favicon.ico`、`/stream`）
  - **循环读请求行**：`recv` 直到 `\r\n`，防 TCP 分片；`settimeout(0.5)` 只用于请求读/前导发送，前导发送成功后才 `setblocking(False)` 挂流表
  - 无观众时 `sleep_ms(20)` 降速不抓帧
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

- TFLite 模型: `yolo3_iou_smartcar_final_with_post_processing.tflite`（SD 卡根目录，YOLOv3 MobileNetV2，INT8，**模型输入 [1,32,320,3] RGB**，含 NMS）
- **ROI 必须与模型输入等比匹配**：`main.py` 用 `ROI_X,ROI_Y,ROI_W,ROI_H = 0, H//2-16, 320, 32`（320×32），**不转灰度**（新模型是 RGB），否则缩放失真导致 X 偏移
- 灰度方案：球在白色/绿色水管背景上为暗色圆形，ROI 限制在摆杆区域后 `find_blobs` 灰度阈值即可检出（备选，未启用）

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
B,<cx>\n                 ← 检测到球，cx = 球像素 X 坐标（0-319，全帧）
N\n                      ← 未检测到
```

> **上电即持续发送**，无 START/STOP 门控（STC32 无需发开始指令）。STC32 `protocol.c` 只解析 `B,<cx>` / `N`，其余忽略。坐标相对 O 点的换算在 STC32 `ball_ctrl` 完成：`cm_x10 = (pixel - pixel_zero) * 10 / px_per_cm`。

### OpenART 引脚映射 (cmm_cfg.csv)

| 功能 | 引脚 | 备注 |
|------|------|------|
| LED1~4 | DISP_B2_03/B1_07/B1_05/B2_08 | 板载 LED |
| UART12 | TX=LPSR_06, RX=LPSR_07 | 与 STC32 通信（`main.py` 用 `UART(12, 115200)`） |
| UART5 | TX=AD_28, RX=AD_29 | 备用串口 |
| UART11 | TX=LPSR_04, RX=LPSR_05 | 未用 |
| I2C5 | SDA=LPSR_04, SCL=LPSR_05 | 未用 |
| I2C6 | SDA=LPSR_06, SCL=LPSR_07 | 共用 UART12 引脚 |
| SPI6 | SCK=LPSR_10, SDO=LPSR_11, SDI=LPSR_12, CS=LPSR_09 | |
| ADC A0~A3 | AD_26~AD_29 | |
| PWM CH1~4 | AD_26~AD_29 (FlexPWM2) | |

> **接线（对应 STC32 `protocol.c`）**：OpenART UART12 TX (LPSR_06) → STC32 P5.0 (UART3 RX)；OpenART UART12 RX (LPSR_07) → STC32 P5.1 (UART3 TX)。
> **注意**：UART12（LPSR_06/07）与 I2C6 共用引脚，默认用于通信的 UART12，不可同时使用。LPSR_04/05 归 UART11/I2C5，未参与通信。

## 比赛题目

H题 — 车载平衡滚球运动控制系统（见 `H题_车载平衡滚球运动控制系统.pdf`）
