/*********************************************************************************************************************
* 文件名称          WcTFT180.h
* 说明              TFT180 1.8寸屏幕安全接口（精简版）—— 无光标，指定位置输出
*
* 设计原则：
*   - 所有显示函数自动检查屏幕边界（x < tft180_x_max, y < tft180_y_max），越界静默丢弃（不 crash）
*   - 字符串超出右边界自动截断，不会触发 zf_assert
*   - 数字格式化自动处理，用户无需关心 num/pointnum 细节
*   - 字色 == 底色时自动将字色改为底色的反色（避免"隐形文字"）
*
* 使用示例：
*   WcTFT_Init();
*   WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
*   WcTFT_PrintCenter(0, "Title");
*   WcTFT_PrintAt(0, 16, "T:");
*   WcTFT_PrintIntAt(24, 16, 42);
*   WcTFT_PrintFloatAt(40, 32, 25.3, 1);
*
* 比赛清理（2026-08）：裁剪到只用 7 个 API（光标模式/绘图/查询 API 已删）
********************************************************************************************************************/

#ifndef _WcTFT180_h_
#define _WcTFT180_h_

#include "zf_common_headfile.h"

/* ── 初始化 ── */
void     WcTFT_Init(void);                       /* 调用 tft180_init() + 读取默认颜色 */

/* ── 颜色设置 ── */
void     WcTFT_SetColor(uint16 pen, uint16 bg);  /* 若 pen==bg 自动将 pen 改为 bg 反色 */

/* ── 显示（指定位置，无光标） ── */
void     WcTFT_PrintAt(uint8 x, uint8 y, const char *str);  /* 越界自动截断 */
void     WcTFT_PrintCenter(uint8 row, const char *str);     /* 第 row 行居中 */
void     WcTFT_PrintIntAt(uint8 x, uint8 y, int32 val);
void     WcTFT_PrintFloatAt(uint8 x, uint8 y, double val, uint8 decimals);

/* ── 屏幕操作 ── */
void     WcTFT_Clear(uint16 color);              /* 清屏 */

#endif