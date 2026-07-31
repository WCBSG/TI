/*********************************************************************************************************************
* 文件名称          WcTFT180.c
* 说明              TFT180 安全显示接口实现
*
* 核心安全机制：
*   1. 所有坐标在调用底层 tft180_* 之前做边界检查
*   2. 字符串显示时计算"还能写几个字符"，超出部分按 wrap_mode 处理（默认截断）
*   3. wrap_mode=0(默认)：超出截断，静默丢弃；wrap_mode=1：自动换行递归输出
*   4. y 超出屏幕高度时停止绘制（静默丢弃）
*   5. SetColor 时若 pen==bg，自动将 pen 改为 ~bg（避免隐形文字）
********************************************************************************************************************/

#include "WcTFT180.h"

/* ── 内部常量 ── */
#define CH_W  8     /* 字符宽 */
#define CH_H  16    /* 字符高 */

/* ── 光标状态 ── */
static uint8 cursor_x = 0;
static uint8 cursor_y = 0;
static uint16 cur_pen   = RGB565_WHITE;
static uint16 cur_bg    = RGB565_BLACK;
static uint8  wrap_mode = 0;   /* 0=截断(默认), 1=自动换行 */

/* ── 获取当前屏幕方向下的字符容量 ── */
static uint8 cols_per_line(void) { return tft180_x_max / CH_W; }
static uint8 rows_max(void)      { return tft180_y_max / CH_H; }

/* ═══════════════════════════════════════════════════════════
 * 内部安全原语
 * ════════════════════════════════════════════════════════════ */

/* 安全画点：越界静默丢弃 */
static void safe_point(uint16 x, uint16 y, uint16 color)
{
    if (x >= tft180_x_max || y >= tft180_y_max) return;
    tft180_draw_point(x, y, color);
}

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

static void fmt_uint(uint32 val, char *buf)
{
    int8 tmp[12];
    zf_sprintf(tmp, (const int8 *)"%u", val);
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

static void fmt_hex(uint32 val, char *buf)
{
    int8 tmp[12];
    zf_sprintf(tmp, (const int8 *)"0x%X", val);
    copy_int8_str(buf, tmp, 12);
}

/* ═══════════════════════════════════════════════════════════
 * 公开 API
 * ════════════════════════════════════════════════════════════ */

/* ── 初始化 ── */
void WcTFT_Init(void)
{
    tft180_init();
    cursor_x = 0;
    cursor_y = 0;
    cur_pen  = tft180_pencolor;
    cur_bg   = tft180_bgcolor;
}

/* ── 光标控制 ── */
void WcTFT_Goto(uint8 x, uint8 y)
{
    cursor_x = x;
    cursor_y = y;
}

void WcTFT_GotoRow(uint8 row)
{
    cursor_x = 0;
    cursor_y = (uint8)(row * CH_H);
}

void WcTFT_Newline(void)
{
    cursor_x = 0;
    cursor_y += CH_H;
}

void WcTFT_Tab(uint8 cols)
{
    cursor_x += (uint8)(cols * CH_W);
}

/* ── 行为设置 ── */
void WcTFT_SetWrap(uint8 enable)
{
    wrap_mode = (enable != 0) ? 1 : 0;
}

/* ── 颜色 ── */
void WcTFT_SetColor(uint16 pen, uint16 bg)
{
    /* 若字色 == 底色，自动将字色改为底色的反色，避免"隐形文字" */
    if (pen == bg)
    {
        pen = (uint16)(~bg);  /* RGB565 按位取反 = 互补色 */
        /* 极端情况：bg=0x7BEF 时 ~bg=0x8410（接近中灰），仍可区分 */
    }
    cur_pen = pen;
    cur_bg  = bg;
    tft180_set_color(pen, bg);
}

/* ── 字符串显示 ── */
void WcTFT_Print(const char *str)
{
    uint8 written;
    const char *p = str;

    tft180_set_color(cur_pen, cur_bg);

    /* 循环写每一段——wrap_mode=1 时换行继续，wrap_mode=0 时仅写一行 */
    while (*p != '\0' && cursor_y < tft180_y_max)
    {
        written = safe_str(cursor_x, cursor_y, p);
        cursor_x = (uint8)(cursor_x + written * CH_W);

        /* 跳过已写字符 */
        p += written;

        /* 还有剩余字符 */
        if (*p != '\0')
        {
            if (wrap_mode && (cursor_y + CH_H) < tft180_y_max)
            {
                WcTFT_Newline();  /* 换行继续 */
            }
            else
            {
                break;  /* 截断模式：丢弃剩余 */
            }
        }
    }
}

void WcTFT_PrintAt(uint8 x, uint8 y, const char *str)
{
    tft180_set_color(cur_pen, cur_bg);
    safe_str(x, y, str);  /* 不影响光标 */
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

void WcTFT_PrintRow(uint8 row, const char *label, const char *value)
{
    /* 在一行内左对齐显示 "label: value" */
    uint8 lx, vx;
    uint8 lab_len = 0;
    while (label[lab_len] != '\0') lab_len++;

    lx = 0;
    vx = (uint8)((lab_len + 2) * CH_W);  /* label + ": " */

    WcTFT_PrintAt(lx, (uint8)(row * CH_H), label);
    WcTFT_PrintAt((uint8)(lab_len * CH_W), (uint8)(row * CH_H), ": ");
    WcTFT_PrintAt(vx, (uint8)(row * CH_H), value);
}

/* ── 数字显示（光标模式） ── */
void WcTFT_PrintInt(int32 val)
{
    char buf[13];
    fmt_int(val, buf);
    WcTFT_Print(buf);
}

void WcTFT_PrintUInt(uint32 val)
{
    char buf[12];
    fmt_uint(val, buf);
    WcTFT_Print(buf);
}

void WcTFT_PrintFloat(double val, uint8 decimals)
{
    char buf[25];
    if (decimals > 6) decimals = 6;
    fmt_float(val, decimals, buf);
    WcTFT_Print(buf);
}

void WcTFT_PrintHex(uint32 val)
{
    char buf[12];
    fmt_hex(val, buf);
    WcTFT_Print(buf);
}

/* ── 数字显示（指定位置） ── */
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

/* ── 屏幕操作 ── */
void WcTFT_Clear(uint16 color)
{
    tft180_clear(color);
    cursor_x = 0;
    cursor_y = 0;
}

void WcTFT_Backlight(uint8 on)
{
    gpio_set_level(TFT180_BL_PIN, on ? 1 : 0);
}

void WcTFT_DrawPoint(uint16 x, uint16 y, uint16 color)
{
    safe_point(x, y, color);
}

void WcTFT_DrawHLine(uint8 x, uint8 y, uint8 w, uint16 color)
{
    uint8 i;
    for (i = 0; i < w; i++)
        safe_point((uint16)(x + i), y, color);
}

void WcTFT_DrawVLine(uint8 x, uint8 y, uint8 h, uint16 color)
{
    uint8 i;
    for (i = 0; i < h; i++)
        safe_point(x, (uint16)(y + i), color);
}

void WcTFT_DrawRect(uint8 x, uint8 y, uint8 w, uint8 h, uint16 color)
{
    WcTFT_DrawHLine(x, y, w, color);                      /* 上 */
    WcTFT_DrawHLine(x, (uint8)(y + h - 1), w, color);      /* 下 */
    WcTFT_DrawVLine(x, y, h, color);                       /* 左 */
    WcTFT_DrawVLine((uint8)(x + w - 1), y, h, color);      /* 右 */
}

void WcTFT_FillRect(uint8 x, uint8 y, uint8 w, uint8 h, uint16 color)
{
    uint8 row;
    for (row = 0; row < h; row++)
        WcTFT_DrawHLine(x, (uint8)(y + row), w, color);
}

void WcTFT_DrawGlyph16(uint8 x, uint8 y, const uint8 bitmap[32], uint16 color)
{
    /* 16×16 点阵汉字绘制
     * bitmap 格式：32 字节，每行 2 字节（左 8 位 + 右 8 位），共 16 行
     * bit=1 画 color 点，bit=0 不画（保持原有背景）
     * MSB 是该行最左像素
     */
    uint8 row, col;
    for (row = 0; row < 16; row++)
    {
        uint8 left  = bitmap[row * 2];      /* 该行左 8 像素 */
        uint8 right = bitmap[row * 2 + 1];  /* 该行右 8 像素 */
        for (col = 0; col < 8; col++)
        {
            if (left & (0x80 >> col))
                safe_point((uint16)(x + col),      (uint16)(y + row), color);
            if (right & (0x80 >> col))
                safe_point((uint16)(x + col + 8),  (uint16)(y + row), color);
        }
    }
}

/* ── 信息查询 ── */
uint8 WcTFT_GetCursorX(void) { return cursor_x; }
uint8 WcTFT_GetCursorY(void) { return cursor_y; }
uint8 WcTFT_GetMaxCols(void) { return cols_per_line(); }
uint8 WcTFT_GetMaxRows(void) { return rows_max(); }
