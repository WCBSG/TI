/*********************************************************************************************************************
* 文件名称          KEY.c
* 说明              5 键按键检测实现 — 释放沿触发 + 长按自动重复（线性加速）
*
* 自动重复参数（ticks ≈ button_control 调用次数 ≈ 50ms）：
*   启动延迟 8 ticks (400ms) → 初始间隔 5 ticks (250ms)
*   → 每次重复减 1 tick (50ms) → 最小间隔 1 tick (50ms)
********************************************************************************************************************/

#include "KEY.h"

#define KEY1_PIN        IO_P92
#define KEY2_PIN        IO_P93
#define KEY3_PIN        IO_P94
#define KEY4_PIN        IO_P95
#define KEY5_PIN        IO_P96

#define SWITCH1_PIN     IO_P90
#define SWITCH2_PIN     IO_P91

/* ── 自动重复参数（ticks，每 tick ≈ 50ms） ── */
#define HOLD_STARTUP_TICKS   8    /* 400ms — 区分短按/长按 */
#define REPEAT_INIT_TICKS    5    /* 250ms — 首次重复间隔 */
#define REPEAT_ACCEL_TICKS   1    /* 每次重复加速 1 tick (50ms) */
#define REPEAT_MIN_TICKS     1    /* 50ms — 最快每帧触发 */

/* ── 全局标志 ── */
uint8 key1_flag, key2_flag, key3_flag, key4_flag, key5_flag;

/* ── 按键引脚表 ── */
static const gpio_pin_enum key_pins[5] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN, KEY5_PIN
};

/* ── 每键状态 ── */
static uint8 key_last[5]   = {1, 1, 1, 1, 1};
static uint8 hold_ticks[5] = {0, 0, 0, 0, 0};   /* 连续按住 tick 数 */
static uint8 repeat_cnt[5] = {0, 0, 0, 0, 0};   /* 本次按住已触发多少次重复 */
static uint8 repeat_cd[5]  = {0, 0, 0, 0, 0};   /* 距离下次重复的倒计时 */


void button_init(void)
{
    gpio_init(KEY1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY2_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY3_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY4_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY5_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(SWITCH1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(SWITCH2_PIN, GPI, 1, GPI_PULL_UP);
}


/** 每 50ms 调用一次。repeat_mask 按位指定哪些键启用长按自动重复。 */
void button_control(uint8 repeat_mask)
{
    uint8 i;
    uint8 st;
    uint8 *flags[5] = { &key1_flag, &key2_flag, &key3_flag, &key4_flag, &key5_flag };

    for (i = 0; i < 5; i++)
    {
        st = gpio_get_level(key_pins[i]);   /* 0=按下(上拉开路), 1=释放 */

        /* ── 释放沿（上升沿）→ 始终触发（短按单步 + 长按最后一步） ── */
        if (st && !key_last[i])
        {
            *flags[i] = 1;
        }

        /* ── 长按自动重复 ── */
        if ((repeat_mask & (1 << i)) && !st)
        {
            hold_ticks[i]++;

            if (hold_ticks[i] >= HOLD_STARTUP_TICKS)
            {
                if (repeat_cd[i] > 0)
                {
                    repeat_cd[i]--;
                }

                if (repeat_cd[i] == 0)
                {
                    *flags[i] = 1;                    /* 触发一次调整 */
                    repeat_cnt[i]++;

                    /* 线性加速：间隔 = max(MIN, INIT - ACCEL * 重复次数) */
                    {
                        uint8 interval;
                        if (REPEAT_INIT_TICKS > repeat_cnt[i] * REPEAT_ACCEL_TICKS)
                            interval = REPEAT_INIT_TICKS - repeat_cnt[i] * REPEAT_ACCEL_TICKS;
                        else
                            interval = REPEAT_MIN_TICKS;
                        repeat_cd[i] = interval;
                    }
                }
            }
        }
        else
        {
            /* 释放或未启用 → 复位 */
            hold_ticks[i] = 0;
            repeat_cnt[i] = 0;
            repeat_cd[i]  = 0;
        }

        key_last[i] = st;
    }
}