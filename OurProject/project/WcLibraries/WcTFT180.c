/*********************************************************************************************************************
* 文件名称          WcTFT180.c
* 说明              TFT180 安全显示接口实现（精简版）
*
* 核心安全机制：
*   1. 所有坐标在调用底层 tft180_* 之前做边界检查
*   2. 字符串显示时计算"还能写几个字符"，超出部分截断
*   3. y 超出屏幕高度时停止绘制（静默丢弃）
*   4. SetColor 时若 pen==bg，自动将 pen 改为 ~bg（避免隐形文字）
*
* 比赛清理（2026-08）：裁剪到只用 7 个 API（光标模式/绘图/查询已删）
********************************************************************************************************************/

#include "WcTFT180.h"

/* ── 内部常量 ── */
#define CH_W  8     /* 字符宽 */
#define CH_H  16    /* 字符高 */

/* ── 颜色状态 ── */
static uint16 cur_pen = RGB565_WHITE;
static uint16 cur_bg  = RGB565_BLACK;

/* ── 当前屏幕方向下的每行字符数 ── */
static uint8 cols_per_line(void) { return tft180_x_max / CH_W; }

/* ═══════════════════════════════════════════════════════════
 * 内部安全原语
 * ════════════════════════════════════════════════════════════ */

/* 安全显示单个字符：越界静默丢弃 */
static void safe_char(uint16 x, uint16 y, char ch)
{
    if (x + CH_W > tft180_x_max || y + CH_H > tft180_y_max) return;
    tft180_show_char(x, y, ch);
}

/* 安全显示字符串的核心：从 (x, y) 开始，最多写 max_chars 个字符，返回实际写入字符数 */
static uint8 safe_str_core(uint16 x, uint16 y, const char *str, uint8 max_chars)
{
    uint8 i;
    if (x >= tft180_x_max || y >= tft180_y_max) return 0;
    if (max_chars > (tft180_x_max - x) / CH_W)
        max_chars = (uint8)((tft180_x_max - x) / CH_W);

    for (i = 0; i < max_chars && str[i] != '\0'; i++)
    {
        safe_char((uint16)(x + i * CH_W), y, str[i]);
    }
    return i;  /* 实际写入字符数 */
}

/* 安全显示字符串：从 (x, y) 开始，自动裁剪到屏幕右边界，返回写入字符数 */
static uint8 safe_str(uint16 x, uint16 y, const char *str)
{
    uint8 maxc;
    if (x >= tft180_x_max || y >= tft180_y_max) return 0;
    maxc = (uint8)((tft180_x_max - x) / CH_W);
    return safe_str_core(x, y, str, maxc);
}

/* ── 内部：int8[] → char[] 转换（zf_sprintf 返回 int8*） ── */
static void copy_int8_str(char *dst, const int8 *src, uint8 maxlen)
{
    uint8 i;
    for (i = 0; i < maxlen && src[i] != '\0'; i++)
        dst[i] = (char)src[i];
    dst[i] = '\0';
}

/* ── 内部格式化辅助 ── */
static void fmt_int(int32 val, char *buf)
{
    int8 tmp[12];
    zf_sprintf(tmp, (const int8 *)"%d", val);
    copy_int8_str(buf, tmp, 12);
}

static void fmt_float(double val, uint8 decimals, char *buf)
{
    int8 tmp[24];
    int8 fmt[8];
    fmt[0] = '%';
    fmt[1] = '.';
    fmt[2] = (int8)('0' + decimals);
    fmt[3] = 'f';
    fmt[4] = '\0';
    zf_sprintf(tmp, fmt, val);
    copy_int8_str(buf, tmp, 24);
}

/* ═══════════════════════════════════════════════════════════
 * 公开 API
 * ════════════════════════════════════════════════════════════ */

void WcTFT_Init(void)
{
    tft180_init();
    cur_pen  = tft180_pencolor;
    cur_bg   = tft180_bgcolor;
}

void WcTFT_SetColor(uint16 pen, uint16 bg)
{
    /* 若字色 == 底色，自动将字色改为底色的反色，避免"隐形文字" */
    if (pen == bg)
    {
        pen = (uint16)(~bg);  /* RGB565 按位取反 = 互补色 */
    }
    cur_pen = pen;
    cur_bg  = bg;
    tft180_set_color(pen, bg);
}

void WcTFT_PrintAt(uint8 x, uint8 y, const char *str)
{
    tft180_set_color(cur_pen, cur_bg);
    safe_str(x, y, str);
}

void WcTFT_PrintCenter(uint8 row, const char *str)
{
    uint8 len, x;
    len = 0;
    while (str[len] != '\0') len++;
    if (len > cols_per_line()) len = cols_per_line();
    x = (uint8)((tft180_x_max - len * CH_W) / 2);
    WcTFT_PrintAt(x, (uint8)(row * CH_H), str);
}

void WcTFT_PrintIntAt(uint8 x, uint8 y, int32 val)
{
    char buf[13];
    fmt_int(val, buf);
    WcTFT_PrintAt(x, y, buf);
}

void WcTFT_PrintFloatAt(uint8 x, uint8 y, double val, uint8 decimals)
{
    char buf[25];
    if (decimals > 6) decimals = 6;
    fmt_float(val, decimals, buf);
    WcTFT_PrintAt(x, y, buf);
}

void WcTFT_Clear(uint16 color)
{
    tft180_clear(color);
}