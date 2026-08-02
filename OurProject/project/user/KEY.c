/*********************************************************************************************************************
* 文件名称          KEY.c
* 说明              4 键按键检测实现 — 释放沿触发 + 长按自动重复（线性加速）
*                   Motherboard 主板：b2=PB2(上) b3=PB3(下) b4=PB4(确定) P3.2=P32(返回)
*
* 自动重复参数（ticks ≈ button_control 调用次数 ≈ 10ms）：
*   启动延迟 20 ticks (200ms) → 初始间隔 25 ticks (250ms)
*   → 每次重复减 5 ticks (50ms) → 最小间隔 1 tick (10ms)
********************************************************************************************************************/

#include "KEY.h"

#define KEY1_PIN        IO_PB2
#define KEY2_PIN        IO_PB3
#define KEY3_PIN        IO_PB4
#define KEY4_PIN        IO_P32

/* ── 自动重复参数（ticks，每 tick ≈ 10ms） ── */
#define HOLD_STARTUP_TICKS   20   /* 200ms — 区分短按/长按 */
#define REPEAT_INIT_TICKS    25   /* 250ms — 首次重复间隔 */
#define REPEAT_ACCEL_TICKS   5    /* 每次重复加速 5 ticks (50ms) */
#define REPEAT_MIN_TICKS     1    /* 10ms — 最快重复间隔 */

/* ── 去抖参数：电平变化需连续 DEBOUNCE_TICKS 次采样（20ms）保持才确认状态
 *    机械按键按下/释放的弹跳若跨过采样点，无去抖会产生多个释放沿误触发 */
#define DEBOUNCE_TICKS       2

/* ── 全局标志 ── */
uint8 key1_flag, key2_flag, key3_flag, key4_flag;

/* ── 按键引脚表 ── */
static const gpio_pin_enum key_pins[4] = {
    KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN
};

/* ── 每键状态 ── */
static uint8 key_last[4]      = {1, 1, 1, 1};   /* 去抖确认后的电平（0=按下, 1=释放） */
static uint8 debounce_cnt[4]  = {0, 0, 0, 0};   /* 电平变化后稳定采样计数 */
static uint8 hold_ticks[4]    = {0, 0, 0, 0};   /* 连续按住 tick 数 */
static uint8 repeat_cnt[4]    = {0, 0, 0, 0};   /* 本次按住已触发多少次重复 */
static uint8 repeat_cd[4]     = {0, 0, 0, 0};   /* 距离下次重复的倒计时 */
static uint8 repeat_active[4] = {0, 0, 0, 0};   /* 本次按住是否已进入长按重复 */


void button_init(void)
{
    gpio_init(KEY1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY2_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY3_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY4_PIN, GPI, 1, GPI_PULL_UP);
}


/** 每 10ms 调用一次。repeat_mask 按位指定哪些键启用长按自动重复。 */
void button_control(uint8 repeat_mask)
{
    uint8 i;
    uint8 st;
    uint8 *flags[4] = { &key1_flag, &key2_flag, &key3_flag, &key4_flag };

    for (i = 0; i < 4; i++)
    {
        st = gpio_get_level(key_pins[i]);   /* 0=按下(上拉开路), 1=释放 */

        /* ── 去抖：电平变化需连续 DEBOUNCE_TICKS 次稳定采样才确认。
         *    机械弹跳若跨过采样点，无去抖会产生多个释放沿误触发。 ── */
        if (st != key_last[i])
        {
            if (++debounce_cnt[i] >= DEBOUNCE_TICKS)
            {
                debounce_cnt[i] = 0;
                key_last[i] = st;                /* 状态确认变化 */

                /* 释放沿（0→1）→ 触发一次；长按重复中松开不重复触发 */
                if (st)
                {
                    if (!repeat_active[i])
                        *flags[i] = 1;
                    repeat_active[i] = 0;        /* 已释放，清长按标记 */
                }
            }
            continue;                            /* 抖动未确认，跳过重复逻辑 */
        }
        debounce_cnt[i] = 0;

        /* ── 长按自动重复（仅确认按下时计数） ── */
        if ((repeat_mask & (1 << i)) && !key_last[i])
        {
            hold_ticks[i]++;

            if (hold_ticks[i] >= HOLD_STARTUP_TICKS)
            {
                repeat_active[i] = 1;            /* 已进入长按重复 */

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
    }
}
