#include "IRPHOTO.h"
#include "WcTFT180.h"

// 八路红外循迹传感器引脚
static const gpio_pin_enum ir_pins[8] = {
    IO_P87, IO_P85, IO_P36, IO_P34,
    IO_PA1, IO_PA3, IO_PA5, IO_PA7,
};

void IRPHOTO_Init(void)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        gpio_init(ir_pins[i], GPI, 1, GPI_PULL_UP);
    }
}

// 加权偏差计算
// 权重: {-7,-5,-3,-1, 1,3,5,7} — 两侧权重高，中间低
// 正值=偏右，负值=偏左，0=居中
int calc_error(int s[8])
{
    static const int8 weight[8] = { -7, -5, -3, -1, 1, 3, 5, 7 };
    int error = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (s[i] == 1)
            error += weight[i];
    }
    return error;
}

// 在 TFT180 屏幕上显示 8 路红外传感器状态
void IRPHOTO_Display(int s[8])
{
    int i;
    for (i = 0; i < 8; i++)
    {
        WcTFT_PrintIntAt((uint8)(10 + i * 8), 50, s[i]);  // 横向排列 8 个值 (0/1)
    }
    WcTFT_PrintIntAt(10, 70, calc_error(s));               // 下一行显示加权偏差
}

// 停车标志检测：≥4 个连续传感器同时检测到黑线
// 附加保护：全部 8 路未检测到黑线 → 视为冲出赛道，返回 2（区别于正常停车 1）
int is_stop(int s[8])
{
    int i;
    int count = 0;
    int has_line = 0;

    for (i = 0; i < 8; i++)
    {
        if (s[i] == 1)
        {
            count++;
            has_line = 1;
            if (count >= 4)
                return 1;       // 停车标志
        }
        else
        {
            count = 0;
        }
    }

    if (!has_line)
        return 2;               // 冲出赛道保护（区别于停车）

    return 0;                   // 正常行驶
}


void IRPHOTO_Read(int s[8])
{
    int i;
    for (i = 0; i < 8; i++)
        s[i] = !gpio_get_level(ir_pins[i]);  // 黑=1, 白=0
}

/* ═══════════════════════════════════════════════════════════
 * IR 测试页同步 — 4×2 大块可视化
 *
 * 布局 (128×160 PORTRAIT)：
 *   Row 0 (y=0):   标题栏（Menu_Draw 绘制）
 *   Row 1 (y=20):  S0  S1  S2  S3    ← 编号
 *   Row 2 (y=28): [██][  ][██][  ]   ← 28×28 方块（白底=无线，黑块=检测到线）
 *   Row 3 (y=62):  S4  S5  S6  S7
 *   Row 4 (y=70): [  ][██][  ][██]
 *   Row 5 (y=108): Err: -7
 *   Row 6 (y=124): Stop: 0 (run)
 * ════════════════════════════════════════════════════════════ */

#include "WcMenu.h"
#include "menu_defs.h"

#define BLOCK_W   28
#define BLOCK_H   28
#define ROW1_Y    20   /* 第一行编号 y */
#define ROW1_BY   28   /* 第一行方块 y */
#define ROW2_Y    62   /* 第二行编号 y */
#define ROW2_BY   70   /* 第二行方块 y */

/* 4 个方块的 X 起始坐标 (28px 宽 + 3px 间距) */
static const uint8 block_x[4] = { 4, 35, 66, 97 };

void IRPHOTO_TestSync(void)
{
    int s[8];
    int i, err, stop;

    if (!Menu_IsTop(&page_ir_test)) return;

    IRPHOTO_Read(s);
    err  = calc_error(s);
    stop = is_stop(s);

    /* ── 上半行：传感器 0-3 ── */
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    for (i = 0; i < 4; i++)
    {
        /* 编号 */
        WcTFT_PrintIntAt((uint8)(block_x[i] + 10), ROW1_Y, (int32)i);
        /* 方块 */
        if (s[i])
        {
            WcTFT_FillRect(block_x[i], ROW1_BY, BLOCK_W, BLOCK_H, RGB565_WHITE);
            /* 黑块居中显示"1" */
            WcTFT_SetColor(RGB565_BLACK, RGB565_WHITE);
            WcTFT_PrintIntAt((uint8)(block_x[i] + 10), (uint8)(ROW1_BY + 6), 1);
        }
        else
        {
            WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
            WcTFT_DrawRect(block_x[i], ROW1_BY, BLOCK_W, BLOCK_H, RGB565_WHITE);
            WcTFT_PrintIntAt((uint8)(block_x[i] + 10), (uint8)(ROW1_BY + 6), 0);
        }
    }

    /* ── 下半行：传感器 4-7 ── */
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    for (i = 4; i < 8; i++)
    {
        WcTFT_PrintIntAt((uint8)(block_x[i - 4] + 10), ROW2_Y, (int32)i);
        if (s[i])
        {
            WcTFT_FillRect(block_x[i - 4], ROW2_BY, BLOCK_W, BLOCK_H, RGB565_WHITE);
            WcTFT_SetColor(RGB565_BLACK, RGB565_WHITE);
            WcTFT_PrintIntAt((uint8)(block_x[i - 4] + 10), (uint8)(ROW2_BY + 6), 1);
        }
        else
        {
            WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
            WcTFT_DrawRect(block_x[i - 4], ROW2_BY, BLOCK_W, BLOCK_H, RGB565_WHITE);
            WcTFT_PrintIntAt((uint8)(block_x[i - 4] + 10), (uint8)(ROW2_BY + 6), 0);
        }
    }

    /* ── 底部信息 ── */
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintAt(0, 108, "Err:");
    WcTFT_PrintIntAt(32, 108, (int32)err);

    WcTFT_PrintAt(0, 124, "Stop:");
    WcTFT_PrintIntAt(40, 124, (int32)stop);
    {
        const char *desc = (stop == 0) ? "(run)" : (stop == 1) ? "(line)" : "(off)";
        WcTFT_PrintAt(56, 124, desc);
    }
}