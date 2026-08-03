/* WcTFT180.h — TFT180 1.8" 屏幕安全显示（无光标，指定位置）
 * 越界自动截断/静默丢弃（不触发原生 zf_assert）；数字格式化自动处理 */
#ifndef _WcTFT180_h_
#define _WcTFT180_h_

#include "zf_common_headfile.h"

void     WcTFT_Init(void);                       /* tft180_init + 读默认颜色 */
void     WcTFT_SetColor(uint16 pen, uint16 bg);  /* pen==bg 自动反色 */
void     WcTFT_PrintAt(uint8 x, uint8 y, const char *str);  /* 越界截断 */
void     WcTFT_PrintCenter(uint8 row, const char *str);     /* 第 row 行居中 */
void     WcTFT_PrintIntAt(uint8 x, uint8 y, int32 val);
void     WcTFT_PrintFloatAt(uint8 x, uint8 y, double val, uint8 decimals);
void     WcTFT_Clear(uint16 color);              /* 清屏 */

#endif