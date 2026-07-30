/*********************************************************************************************************************
* 文件名称          tft_test.c
* 说明              TFT180 屏幕功能测试（使用 WcTFT180 安全接口）
*
* 测试项目（循环执行 6 项，每项目视检查）：
*   [1] 颜色填充 — 8 色全屏，查坏点/偏色/竖线横线
*   [2] 文字显示 — ASCII 全覆盖 + 多色组合
*   [3] 数字显示 — int32 极值 + 正负浮点数（安全接口自动裁剪，不会溢出死机）
*   [4] 画点绘图 — 边框/对角线/网格/棋盘格/色条
*   [5] 方向切换 — 4 方向旋转
*   [6] 背光闪烁 — BL 引脚 5 次开关
*
* 使用方式：
*   模块模式：main.c 中 #include "tft_test.h"，调用 tft_test_run();
*   独立模式：编译选项中 #define TFT_TEST_STANDALONE
********************************************************************************************************************/

#include "WcTFT180.h"
#include "tft_test.h"

/* ── 测试延时 (ms) ── */
#define DELAY_LONG      1200
#define DELAY_MID       800
#define DELAY_SHORT     400

/* ═══════════════════════════════════════════════════════════
 * 辅助函数
 * ════════════════════════════════════════════════════════════ */

/* 串口输出 */
static void debug_str(const char *str)
{
    uint16 len = 0;
    while (str[len] != '\0') len++;
    if (len > 0)
        debug_send_buffer((const uint8 *)str, len);
}

/* 显示测试标题（蓝底白字，占第 0 行） */
static void test_title(const char *str)
{
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLUE);
    WcTFT_FillRect(0, 0, tft180_x_max, 16, RGB565_BLUE);
    WcTFT_PrintAt(0, 0, str);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 1：颜色填充 — 8 色全屏
 * ════════════════════════════════════════════════════════════ */
static void test_color_fill(void)
{
    static const uint16 colors[] = {
        RGB565_RED, RGB565_GREEN, RGB565_BLUE,
        RGB565_WHITE, RGB565_BLACK,
        RGB565_YELLOW, RGB565_CYAN, RGB565_MAGENTA,
    };
    static const char *names[] = {
        "RED", "GREEN", "BLUE", "WHITE", "BLACK", "YELLOW", "CYAN", "MAGENTA"
    };
    uint8 i;

    WcTFT_Clear(RGB565_BLACK);
    test_title("T1: Color Fill");

    for (i = 0; i < 8; i++)
    {
        /* 先在上方显示颜色名 */
        WcTFT_SetColor(RGB565_WHITE, colors[i]);
        WcTFT_PrintAt(0, 16, names[i]);
        WcTFT_PrintAt(0, 32, "no dead pixel / line");
        system_delay_ms(DELAY_MID);

        /* 全屏填充 */
        WcTFT_Clear(colors[i]);
        system_delay_ms(DELAY_MID);
    }

    WcTFT_Clear(RGB565_WHITE);
    system_delay_ms(DELAY_SHORT);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 2：文字显示 — ASCII + 多色
 * ════════════════════════════════════════════════════════════ */
static void test_text_display(void)
{
    uint8 row, col;
    char ch;

    /* 2a. ASCII 32-79 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T2: ASCII 32-79");
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);

    row = 1; col = 0;
    for (ch = 32; ch < 80; ch++)
    {
        WcTFT_PrintAt((uint8)(col * 8), (uint8)(row * 16), &ch);  /* 单字符 */
        /* 注意：WcTFT_PrintAt 期望字符串，这里传单字符地址 */
        col++;
        if (col >= WcTFT_GetMaxCols()) { col = 0; row++; }
    }
    system_delay_ms(DELAY_LONG);

    /* 2b. ASCII 80-126 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T2: ASCII 80-126");
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);

    row = 1; col = 0;
    for (ch = 80; ch < 127; ch++)
    {
        char s[2];
        s[0] = ch; s[1] = '\0';
        WcTFT_PrintAt((uint8)(col * 8), (uint8)(row * 16), s);
        col++;
        if (col >= WcTFT_GetMaxCols()) { col = 0; row++; }
    }
    system_delay_ms(DELAY_LONG);

    /* 2c. 多色文字 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T2: Color Text");

    WcTFT_SetColor(RGB565_RED,     RGB565_BLACK); WcTFT_PrintAt(0, 16, "RED on BLACK");
    WcTFT_SetColor(RGB565_GREEN,   RGB565_BLACK); WcTFT_PrintAt(0, 32, "GREEN on BLACK");
    WcTFT_SetColor(RGB565_BLUE,    RGB565_BLACK); WcTFT_PrintAt(0, 48, "BLUE on BLACK");
    WcTFT_SetColor(RGB565_YELLOW,  RGB565_BLACK); WcTFT_PrintAt(0, 64, "YELLOW on BLACK");
    WcTFT_SetColor(RGB565_CYAN,    RGB565_BLACK); WcTFT_PrintAt(0, 80, "CYAN on BLACK");
    WcTFT_SetColor(RGB565_BLACK,   RGB565_WHITE); WcTFT_PrintAt(0, 96, "BLACK on WHITE");
    WcTFT_SetColor(RGB565_WHITE,   RGB565_BLUE);  WcTFT_PrintAt(0, 112,"WHITE on BLUE");
    WcTFT_SetColor(RGB565_MAGENTA, RGB565_BLACK); WcTFT_PrintAt(0, 128,"MAGENTA on BLACK");
    system_delay_ms(DELAY_LONG);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 3：数字显示 — 用安全接口，绝不溢出
 * ════════════════════════════════════════════════════════════ */
static void test_number_display(void)
{
    WcTFT_Clear(RGB565_BLACK);
    test_title("T3: Numbers");

    /* int8 极值 */
    WcTFT_SetColor(RGB565_YELLOW, RGB565_BLACK);
    WcTFT_PrintRow(1, "int8", "-128, 0, 127");
    WcTFT_PrintIntAt(0, 32, -128);
    WcTFT_PrintAt(48, 32, ",");
    WcTFT_PrintIntAt(56, 32, 0);
    WcTFT_PrintAt(80, 32, ",");
    WcTFT_PrintIntAt(88, 32, 127);

    /* uint8 */
    WcTFT_PrintRow(2, "uint8", "0, 128, 255");
    WcTFT_PrintIntAt(0, 48, 0);
    WcTFT_PrintAt(40, 48, ",");
    WcTFT_PrintIntAt(48, 48, 128);
    WcTFT_PrintAt(80, 48, ",");
    WcTFT_PrintIntAt(88, 48, 255);

    /* int16 / uint16 */
    WcTFT_PrintRow(3, "int16", "-32768");
    WcTFT_PrintIntAt(0, 64, -32768);

    WcTFT_PrintRow(4, "uint16", "65535");
    WcTFT_PrintIntAt(0, 80, 65535);

    /* int32 */
    WcTFT_PrintRow(5, "int32", "-2.14B");
    WcTFT_PrintIntAt(0, 96, -2147483648L);

    system_delay_ms(DELAY_LONG);

    /* 浮点数 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T3: Float");

    WcTFT_SetColor(RGB565_CYAN, RGB565_BLACK);
    WcTFT_PrintRow(1, "float", "3.14");
    WcTFT_PrintFloatAt(72, 16, 3.14, 2);

    WcTFT_PrintRow(2, "float", "-99.5");
    WcTFT_PrintFloatAt(72, 32, -99.5, 1);

    WcTFT_PrintRow(3, "float", "0.001");
    WcTFT_PrintFloatAt(72, 48, 0.001, 3);

    WcTFT_PrintRow(4, "float", "12345.68");
    WcTFT_PrintFloatAt(72, 64, 12345.678, 2);

    WcTFT_PrintRow(5, "float", "-0.5");
    WcTFT_PrintFloatAt(72, 80, -0.5, 1);

    system_delay_ms(DELAY_LONG);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 4：画点绘图
 * ════════════════════════════════════════════════════════════ */
static void test_draw_point(void)
{
    uint8 i, x, y;

    /* 4a. 边框 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T4a: Border");
    for (i = 0; i < tft180_x_max; i++)
    {
        WcTFT_DrawPoint(i, 16, RGB565_WHITE);
        WcTFT_DrawPoint(i, (uint16)(tft180_y_max - 1), RGB565_WHITE);
    }
    for (i = 16; i < tft180_y_max; i++)
    {
        WcTFT_DrawPoint(0, i, RGB565_WHITE);
        WcTFT_DrawPoint((uint16)(tft180_x_max - 1), i, RGB565_WHITE);
    }
    WcTFT_PrintCenter(3, "Border Check");
    system_delay_ms(DELAY_MID);

    /* 4b. 对角线 + 十字 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T4b: Diag + Cross");
    for (i = 0; (1 + i) < (tft180_x_max - 1) && (17 + i) < (tft180_y_max - 1); i++)
    {
        WcTFT_DrawPoint((uint16)(1 + i), (uint16)(17 + i), RGB565_RED);
    }
    for (i = 0; (1 + i) < (tft180_x_max - 1) && (tft180_y_max - 2 - i) > 16; i++)
    {
        WcTFT_DrawPoint((uint16)(1 + i), (uint16)(tft180_y_max - 2 - i), RGB565_BLUE);
    }
    for (i = 1; i < tft180_x_max - 1; i++)
        WcTFT_DrawPoint(i, (uint16)(tft180_y_max / 2), RGB565_GREEN);
    for (i = 17; i < tft180_y_max - 1; i++)
        WcTFT_DrawPoint((uint16)(tft180_x_max / 2), i, RGB565_GREEN);
    system_delay_ms(DELAY_MID);

    /* 4c. 网格 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T4c: Grid 16px");
    for (x = 0; x < tft180_x_max; x += 16)
        for (y = 16; y < tft180_y_max; y++)
            WcTFT_DrawPoint(x, y, RGB565_GRAY);
    for (y = 16; y < tft180_y_max; y += 16)
        for (x = 0; x < tft180_x_max; x++)
            WcTFT_DrawPoint(x, y, RGB565_GRAY);
    system_delay_ms(DELAY_MID);

    /* 4d. 棋盘格 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T4d: Checker");
    {
        uint8 bx, by;
        for (by = 0; by < (tft180_y_max - 16) / 8; by++)
        {
            for (bx = 0; bx < tft180_x_max / 8; bx++)
            {
                uint16 color = ((bx + by) & 1) ? RGB565_WHITE : RGB565_BLACK;
                uint8 px, py;
                for (py = 0; py < 8; py++)
                    for (px = 0; px < 8; px++)
                        WcTFT_DrawPoint((uint16)(bx * 8 + px),
                                   (uint16)(16 + by * 8 + py), color);
            }
        }
    }
    system_delay_ms(DELAY_MID);

    /* 4e. 彩色条 */
    WcTFT_Clear(RGB565_BLACK);
    test_title("T4e: Color Bars");
    {
        uint16 bar_colors[8] = {
            RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_YELLOW,
            RGB565_CYAN, RGB565_MAGENTA, RGB565_WHITE, 0x7BEF
        };
        uint8 bw = tft180_x_max / 8;  /* 每条约 16px */
        uint8 bar, bx;
        for (bar = 0; bar < 8; bar++)
        {
            for (y = 17; y < tft180_y_max - 1; y++)
                for (bx = 0; bx < bw; bx++)
                    WcTFT_DrawPoint((uint16)(bar * bw + bx), y, bar_colors[bar]);
        }
    }
    system_delay_ms(DELAY_MID);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 5：方向切换
 * ════════════════════════════════════════════════════════════ */
static void test_orientation(void)
{
    static const char *dir_names[] = {
        "PORTRAIT", "PORTRAIT_180", "CROSSWISE", "CROSSWISE_180"
    };
    tft180_dir_enum dirs[] = {
        TFT180_PORTAIT, TFT180_PORTAIT_180,
        TFT180_CROSSWISE, TFT180_CROSSWISE_180
    };
    uint8 d;

    for (d = 0; d < 4; d++)
    {
        tft180_set_dir(dirs[d]);
        WcTFT_Clear(RGB565_BLACK);
        WcTFT_SetColor(RGB565_WHITE, RGB565_BLUE);
        WcTFT_PrintAt(0, 0, "T5: Orientation");
        WcTFT_SetColor(RGB565_YELLOW, RGB565_BLACK);
        WcTFT_PrintAt(0, 32, dir_names[d]);

        WcTFT_SetColor(RGB565_GREEN, RGB565_BLACK);
        WcTFT_PrintAt(0, 64, "ABCDEFGHIJKLMNOP");
        WcTFT_PrintAt(0, 80, "0123456789+-*/=~");

        /* 四角标记 */
        tft180_draw_point(0, 0, RGB565_RED);
        tft180_draw_point((uint16)(tft180_x_max - 1), 0, RGB565_GREEN);
        tft180_draw_point(0, (uint16)(tft180_y_max - 1), RGB565_BLUE);
        tft180_draw_point((uint16)(tft180_x_max - 1),
                          (uint16)(tft180_y_max - 1), RGB565_YELLOW);

        system_delay_ms(DELAY_LONG);
    }

    tft180_set_dir(TFT180_DEFAULT_DISPLAY_DIR);
    WcTFT_Clear(RGB565_BLACK);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 6：背光
 * ════════════════════════════════════════════════════════════ */
static void test_backlight(void)
{
    uint8 i;

    WcTFT_Clear(RGB565_BLACK);
    test_title("T6: Backlight");
    WcTFT_SetColor(RGB565_YELLOW, RGB565_BLACK);
    WcTFT_PrintCenter(3, "BL Blink x5");
    WcTFT_PrintCenter(4, "Watch ON/OFF");

    for (i = 0; i < 5; i++)
    {
        system_delay_ms(DELAY_SHORT);
        WcTFT_Backlight(0);
        WcTFT_SetColor(RGB565_RED, RGB565_BLACK);
        WcTFT_PrintAt(32, 96, "BL = OFF");
        system_delay_ms(DELAY_SHORT);
        WcTFT_Backlight(1);
        WcTFT_SetColor(RGB565_GREEN, RGB565_BLACK);
        WcTFT_PrintAt(32, 96, "BL = ON ");
    }

    WcTFT_Backlight(1);
}

/* ═══════════════════════════════════════════════════════════
 * 测试 7：中文字符（16×16 点阵）
 *   用 WcTFT_DrawGlyph16 逐点绘制，证明屏幕可以显示中文
 * ════════════════════════════════════════════════════════════ */
static void test_chinese(void)
{
    /* HZK16 标准 16×16 点阵数据（每字 32 字节，逐行左8+右8） */
    static const uint8 cn_zhong[32] = {  /* "中" */
        0x01,0x00, 0x01,0x00, 0x01,0x00, 0x01,0x00,
        0x01,0x00, 0x7F,0xFE, 0x41,0x04, 0x41,0x04,
        0x41,0x04, 0x41,0x04, 0x41,0x04, 0x7F,0xFC,
        0x01,0x00, 0x01,0x00, 0x01,0x00, 0x01,0x00
    };
    static const uint8 cn_wen[32] = {    /* "文" */
        0x00,0x00, 0x20,0x80, 0x20,0x80, 0x20,0x80,
        0x20,0x80, 0x20,0x80, 0x2F,0xF8, 0x20,0x80,
        0x20,0x80, 0x20,0x80, 0x20,0x80, 0x40,0x80,
        0x40,0x80, 0x80,0x80, 0x81,0x00, 0x06,0x00
    };
    static const uint8 cn_ce[32] = {     /* "测" */
        0x10,0x00, 0x10,0xFC, 0x10,0x04, 0x10,0x08,
        0xFC,0x10, 0x10,0x20, 0x10,0xFE, 0x10,0x42,
        0x10,0x82, 0x14,0x82, 0x18,0x7E, 0x30,0x42,
        0x50,0x42, 0x90,0x7E, 0x10,0x42, 0x10,0x00
    };
    static const uint8 cn_shi[32] = {    /* "试" */
        0x00,0x20, 0x40,0x20, 0x40,0x24, 0x40,0x24,
        0x40,0x28, 0x48,0x30, 0x48,0xE4, 0x7C,0x44,
        0x48,0x48, 0x48,0x48, 0x48,0x50, 0x78,0x60,
        0x48,0x58, 0x00,0x46, 0x00,0x42, 0x00,0x00
    };
    static const uint8 cn_ok[32] = {     /* "✓" 样式的 "好" */
        0x10,0x00, 0x10,0x00, 0x90,0xFC, 0x50,0x04,
        0x50,0x08, 0x10,0x10, 0xFE,0x20, 0x12,0xFE,
        0x12,0x20, 0x12,0x20, 0x12,0x20, 0x22,0x20,
        0x22,0x20, 0x42,0x20, 0x82,0xA0, 0x04,0x40
    };

    WcTFT_Clear(RGB565_BLACK);
    test_title("T7: Chinese 16x16");

    /* 第一行："中文" */
    WcTFT_SetColor(RGB565_YELLOW, RGB565_BLACK);
    WcTFT_DrawGlyph16(0,  20, cn_zhong, RGB565_YELLOW);
    WcTFT_DrawGlyph16(20, 20, cn_wen,   RGB565_YELLOW);

    /* 第二行："测试" */
    WcTFT_SetColor(RGB565_CYAN, RGB565_BLACK);
    WcTFT_DrawGlyph16(0,  48, cn_ce,  RGB565_CYAN);
    WcTFT_DrawGlyph16(20, 48, cn_shi, RGB565_CYAN);

    /* 下方用 ASCII 标注 */
    WcTFT_SetColor(RGB565_GREEN, RGB565_BLACK);
    WcTFT_PrintCenter(5, "Chinese OK!");
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintAt(0, 96, "16x16 dot matrix");
    WcTFT_PrintAt(0, 112,"via DrawGlyph16()");

    system_delay_ms(3000);
}

/* ═══════════════════════════════════════════════════════════
 * 测试主入口
 * ════════════════════════════════════════════════════════════ */
void tft_test_run(void)
{
    uint8 round;
    char rbuf[3];

    debug_str("\r\n=== TFT180 Test (WcTFT180 Safe API) ===\r\n");

    round = 0;
    while (1)
    {
        round++;
        rbuf[0] = (char)((round / 10) % 10) + '0';
        rbuf[1] = (char)(round % 10) + '0';
        rbuf[2] = '\0';
        debug_str("Round: ");
        debug_str(rbuf);
        debug_str("\r\n");

        /* 轮次标题 */
        WcTFT_Clear(RGB565_BLACK);
        WcTFT_SetColor(RGB565_WHITE, RGB565_BLUE);
        WcTFT_PrintCenter(0, "TFT180 TEST");
        WcTFT_SetColor(RGB565_YELLOW, RGB565_BLACK);
        WcTFT_PrintAt(48, 16, "Round:");
        WcTFT_PrintIntAt(96, 16, round);
        system_delay_ms(DELAY_SHORT);

        test_color_fill();
        test_text_display();
        test_number_display();
        test_draw_point();
        test_orientation();
        test_backlight();
        test_chinese();

        WcTFT_Clear(RGB565_BLACK);
        WcTFT_SetColor(RGB565_GREEN, RGB565_BLACK);
        WcTFT_PrintCenter(3, "ALL TESTS DONE");
        WcTFT_SetColor(RGB565_YELLOW, RGB565_BLACK);
        WcTFT_PrintAt(56, 64, "Round:");
        WcTFT_PrintIntAt(112, 64, round);
        WcTFT_SetColor(RGB565_CYAN, RGB565_BLACK);
        WcTFT_PrintCenter(5, "Looping...");
        system_delay_ms(DELAY_LONG);
    }
}

#ifdef TFT_TEST_STANDALONE
void main(void)
{
    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    WcTFT_Init();  /* 用 WcTFT 替代 tft180_init */
    tft_test_run();
}
#endif
