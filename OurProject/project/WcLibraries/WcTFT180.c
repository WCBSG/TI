/* WcTFT180.c — TFT180 安全显示（无光标，指定位置输出）
 * 所有坐标边界检查，越界静默丢弃（不触发 zf_assert）；pen==bg 自动反色 */
#include "WcTFT180.h"

#define CH_W  8
#define CH_H  16

static uint16 cur_pen = RGB565_WHITE;
static uint16 cur_bg  = RGB565_BLACK;

static uint8 cols_per_line(void) { return tft180_x_max / CH_W; }

static void safe_char(uint16 x, uint16 y, char ch)
{ if (x + CH_W > tft180_x_max || y + CH_H > tft180_y_max) return; tft180_show_char(x, y, ch); }

static uint8 safe_str_core(uint16 x, uint16 y, const char *str, uint8 max_chars)
{
    uint8 i;
    if (x >= tft180_x_max || y >= tft180_y_max) return 0;
    if (max_chars > (tft180_x_max - x) / CH_W) max_chars = (uint8)((tft180_x_max - x) / CH_W);
    for (i = 0; i < max_chars && str[i] != '\0'; i++) safe_char((uint16)(x + i * CH_W), y, str[i]);
    return i;
}

static uint8 safe_str(uint16 x, uint16 y, const char *str)
{ return (x >= tft180_x_max || y >= tft180_y_max) ? 0 : safe_str_core(x, y, str, (uint8)((tft180_x_max - x) / CH_W)); }

static void copy_int8_str(char *dst, const int8 *src, uint8 maxlen)
{ uint8 i; for (i = 0; i < maxlen && src[i]; i++) dst[i] = (char)src[i]; dst[i] = '\0'; }

static void fmt_int(int32 val, char *buf)
{ int8 tmp[12]; zf_sprintf(tmp, (const int8 *)"%d", val); copy_int8_str(buf, tmp, 12); }

static void fmt_float(double val, uint8 decimals, char *buf)
{
    int8 tmp[24], fmt[8];
    fmt[0] = '%'; fmt[1] = '.'; fmt[2] = (int8)('0' + decimals); fmt[3] = 'f'; fmt[4] = '\0';
    zf_sprintf(tmp, fmt, val); copy_int8_str(buf, tmp, 24);
}

void WcTFT_Init(void)
{ tft180_init(); cur_pen = tft180_pencolor; cur_bg = tft180_bgcolor; }

void WcTFT_SetColor(uint16 pen, uint16 bg)
{
    if (pen == bg) pen = (uint16)(~bg);   /* 避免隐形文字 */
    cur_pen = pen; cur_bg = bg;
    tft180_set_color(pen, bg);
}

void WcTFT_PrintAt(uint8 x, uint8 y, const char *str)
{ tft180_set_color(cur_pen, cur_bg); safe_str(x, y, str); }

void WcTFT_PrintCenter(uint8 row, const char *str)
{
    uint8 len = 0;
    while (str[len]) len++;
    if (len > cols_per_line()) len = cols_per_line();
    WcTFT_PrintAt((uint8)((tft180_x_max - len * CH_W) / 2), (uint8)(row * CH_H), str);
}

void WcTFT_PrintIntAt(uint8 x, uint8 y, int32 val)
{ char buf[13]; fmt_int(val, buf); WcTFT_PrintAt(x, y, buf); }

void WcTFT_PrintFloatAt(uint8 x, uint8 y, double val, uint8 decimals)
{ char buf[25]; if (decimals > 6) decimals = 6; fmt_float(val, decimals, buf); WcTFT_PrintAt(x, y, buf); }

void WcTFT_Clear(uint16 color) { tft180_clear(color); }