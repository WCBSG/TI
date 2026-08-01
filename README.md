# 车载平衡滚球运动控制系统

全国大学生电子设计竞赛 H 题 — 竞赛小车项目。

## 硬件架构

| 芯片 | 型号 | 职责 |
|------|------|------|
| 主控 | STC32G144K246 (C251) | 红外循迹、差速开环巡线、舵机球稳、陀螺仪阻尼、任务调度、菜单交互 |
| 视觉 | OpenART Plus (RT117x) | 钢球位置检测（色块/TFLite），UART 输出 |
| 图传 | OMV-RT5 (RT1062) | WiFi AP MJPEG 推流，手机/PC 实时查看（代码在 `OPenMV RT5/`） |

## 目录结构

```
OurProject/           ← STC32 主控工程（Keil C251）
  libraries/            ← 逐飞官方库（zf_common/driver/device/components）
  project/
    WcLibraries/        ← 用户封装库
      WcTFT180.c/h        TFT180 安全屏幕接口（自动边界裁剪、反色、换行）
      WcMenu.c/h          栈式菜单系统（按键导航、逐项滚动）
      protocol.c/h        OpenART UART3 球坐标协议
    user/               ← 用户应用代码
      main.c              主程序入口（三态循环：菜单⇄任务⇄结果）
      IRPHOTO.c/h         八路红外循迹
      Motor.c/h           电机 PWM + 编码器（PIT 5ms 时基）
      PID.c/h             位置式 PID（差速用）
      line_ctrl.c/h       差速开环巡线（弯道减速 + 陀螺仪阻尼）
      ball_ctrl.c/h       舵机球稳闭环
      task_sched.c/h      任务 2/3/5/6 框架 + 串口调试帧
      imu_ctrl.c/h        IMU660RA 陀螺仪（X 轴 yaw，零点标定）
      config.c/h          参数持久化（IAP）
      menu_defs.c/h       菜单页面定义
      isr.c/h             中断服务
    mdk/                ← Keil 工程文件（seekfree.uvproj）

OPenMV RT5/            ← 图传端 MicroPython（OpenMV IDE，芯片型号 OMV-RT5）
OpenART Plus/          ← 识别端 MicroPython（OpenMV IDE）
docs/                  ← 项目文档
```

## 开发环境

| 端 | IDE | 语言 |
|---|---|---|
| STC32 | Keil MDK for C251 V5.60 | C (C251) |
| OMV-RT5 | OpenMV IDE | MicroPython |
| OpenART Plus | OpenMV IDE | MicroPython |

### STC32 下载

- 按住 P32 引脚上电进入 USB 下载模式，使用 AiCube-ISP 烧录
- 推荐 **AiCube-ISP MCP 自动烧录**：`project/AiCube-ISP-v6.96Z.exe` 内置 MCP 服务（`http://localhost:8051/mcp`），AI 可操控 GUI 完成烧录（详见 CLAUDE.md）

## 封装库

### WcTFT180 — 安全屏幕接口

`WcLibraries/WcTFT180.h`

```c
WcTFT_Init();
WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
WcTFT_PrintAt(0, 16, "Hello");        // 指定位置（不影响光标）
WcTFT_PrintCenter(3, "Title");        // 居中
WcTFT_PrintIntAt(80, 16, 42);         // 数字
WcTFT_DrawGlyph16(x, y, bitmap, RGB565_YELLOW);  // 16×16 中文
```

- 自动边界裁剪，越界静默丢弃（不 crash）
- 字色 = 底色时自动反色
- `WcTFT_SetWrap(1)` 开启自动换行

### WcMenu — 栈式菜单

`WcLibraries/WcMenu.h`

```c
Menu_Init();
Menu_Push(&page_main);     // 显示页面
Menu_Inc();                // Key1: 上移/参数+
Menu_Dec();                // Key2: 下移/参数-
Menu_Edit();               // Key3: 确定/进入编辑
Menu_Cancel();             // Key4: 取消/返回
Menu_Home(&page_main);     // Key5: 主菜单→Launch / 子页回主菜单
```

- 栈式导航，支持嵌套子菜单
- 带 `value` 指针的菜单项可进入编辑态（Key3 进入，Key1/Key2 调值，Key3 保存，Key4 取消）
- 选中项反色高亮
