# 车载平衡滚球运动控制系统

全国大学生电子设计竞赛 H 题 — 竞赛小车项目。

## 硬件架构

| 芯片 | 型号 | 职责 |
|------|------|------|
| 主控 | STC32G144K246 (C251) | 红外循迹、电机 PID 控制、菜单交互 |
| 视觉 | OpenART Plus (RT117x) | 钢球位置检测（色块/TFLite），UART 输出 |
| 图传 | OMV-RT5 (RT1062) | WiFi AP MJPEG 推流，手机/PC 实时查看 |

## 目录结构

```
OurProject/           ← STC32 主控工程（Keil C251）
  libraries/            ← 逐飞官方库（zf_common/driver/device/components）
  project/
    WcLibraries/        ← 用户封装库
      WcTFT180.c/h        TFT180 安全屏幕接口（自动边界裁剪、反色、换行）
      WcMenu.c/h          栈式菜单系统（按键导航、逐项滚动）
    user/               ← 用户应用代码
      main.c              主程序入口
      IRPHOTO.c/h         八路红外循迹
      Motor.c/h           电机 PWM + 编码器
      PID.c/h             PID 控制器
      control.c/h         差速转向
      camera.c/h          MT9V03x 灰度摄像头 + OTSU 钢球检测
      isr.c/h             中断服务
    mdk/                ← Keil 工程文件（seekfree.uvproj）

OPenMV RT5/            ← 图传端 MicroPython（OpenMV IDE）
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

按住 P32 引脚上电进入 USB 下载模式，使用 AiCube-ISP 烧录。

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
Menu_Push(&page_main);   // 显示页面
Menu_Up();   // 上   (在按键 ISR 中调用)
Menu_Down(); // 下
Menu_Ok();   // 确认
Menu_Back(); // 返回
```

- 栈深度 8 层，支持嵌套子菜单
- 超过 9 项自动逐项滚动
- 选中项白底黑字反色高亮
