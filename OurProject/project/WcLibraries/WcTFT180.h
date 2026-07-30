/*********************************************************************************************************************
* 文件名称          WcTFT180.h
* 说明              TFT180 1.8寸屏幕安全接口 —— 自动边界裁剪、可选换行、自动反色、光标跟踪
*
* 设计原则：
*   - 所有显示函数自动检查屏幕边界（x < tft180_x_max, y < tft180_y_max），越界静默丢弃（不 crash）
*   - 字符串超出右边界自动截断，不会触发 zf_assert
*   - 提供光标模式（WcTFT_Print）和无光标模式（WcTFT_PrintAt）两种用法
*   - 数字格式化自动处理，用户无需关心 num/pointnum 细节
*   - 超出右边界：默认截断，可通过 WcTFT_SetWrap(1) 切换为自动换行
*   - 字色 == 底色时自动将字色改为底色的反色（避免"隐形文字"）
*
* 使用示例：
*   WcTFT_Init();
*   WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
*   WcTFT_PrintAt(0, 0, "Hello");              // 在 (0,0) 显示，不移动光标
*   WcTFT_GotoRow(1);                            // 光标移到第 1 行行首
*   WcTFT_Print("Sensor = ");                    // 光标处显示，光标自动后移
*   WcTFT_PrintInt(42);                          // 显示数字，光标后移
*   WcTFT_PrintCenter(3, "Centered Title");      // 第 3 行居中显示
********************************************************************************************************************/

#ifndef _WcTFT180_h_
#define _WcTFT180_h_

#include "zf_common_headfile.h"

/* ── 初始化 ── */
void WcTFT_Init(void);                          /* 调用 tft180_init()，初始化光标到 (0,0)，默认截断模式 */

/* ── 行为设置 ── */
void WcTFT_SetWrap(uint8 enable);               /* 超出右边界行为：0=截断(默认), 1=自动换行 */

/* ── 光标控制 ── */
void WcTFT_Goto(uint8 x, uint8 y);              /* 移动光标到 (x, y) 像素坐标 */
void WcTFT_GotoRow(uint8 row);                  /* 移动光标到第 row 行行首 (0, row*16) */
void WcTFT_Newline(void);                       /* 光标换行：x=0, y+=16 */
void WcTFT_Tab(uint8 cols);                     /* 光标右移 cols 个字符宽度 (cols*8px) */

/* ── 颜色设置 ── */
void WcTFT_SetColor(uint16 pen, uint16 bg);     /* 设置前景色/背景色；若 pen==bg 自动将 pen 改为 bg 反色 */

/* ── 字符串显示 ── */
void WcTFT_Print(const char *str);              /* 光标处显示，超出右边界按 SetWrap 设置处理（默认截断） */
void WcTFT_PrintAt(uint8 x, uint8 y, const char *str); /* 指定位置显示，始终截断，不影响光标 */
void WcTFT_PrintCenter(uint8 row, const char *str);    /* 在第 row 行居中显示 */
void WcTFT_PrintRow(uint8 row, const char *label, const char *value); /* "label: value" 格式，一行内 */

/* ── 数字显示（光标模式，受 WcTFT_SetWrap 影响） ── */
void WcTFT_PrintInt(int32 val);
void WcTFT_PrintUInt(uint32 val);
void WcTFT_PrintFloat(double val, uint8 decimals);
void WcTFT_PrintHex(uint32 val);

/* ── 数字显示（指定位置，不受光标/换行影响） ── */
void WcTFT_PrintIntAt(uint8 x, uint8 y, int32 val);
void WcTFT_PrintFloatAt(uint8 x, uint8 y, double val, uint8 decimals);

/* ── 屏幕操作 ── */
void WcTFT_Clear(uint16 color);                 /* 清屏并重置光标到 (0,0) */
void WcTFT_Backlight(uint8 on);                 /* 背光开关：1=ON, 0=OFF */
void WcTFT_DrawPoint(uint16 x, uint16 y, uint16 color);          /* 安全画点（越界静默丢弃） */
void WcTFT_DrawHLine(uint8 x, uint8 y, uint8 w, uint16 color);  /* 水平线 */
void WcTFT_DrawVLine(uint8 x, uint8 y, uint8 h, uint16 color);  /* 垂直线 */
void WcTFT_DrawRect(uint8 x, uint8 y, uint8 w, uint8 h, uint16 color); /* 矩形边框 */
void WcTFT_FillRect(uint8 x, uint8 y, uint8 w, uint8 h, uint16 color); /* 实心矩形 */
void WcTFT_DrawGlyph16(uint8 x, uint8 y, const uint8 bitmap[32], uint16 color); /* 16×16 点阵字符（中文） */

/* ── 信息查询 ── */
uint8 WcTFT_GetCursorX(void);
uint8 WcTFT_GetCursorY(void);
uint8 WcTFT_GetMaxCols(void);                   /* 当前方向每行最大字符数 */
uint8 WcTFT_GetMaxRows(void);                   /* 当前方向最大行数 */

#endif
