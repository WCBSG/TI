/* KEY.c — 4 键按键：释放沿触发 + 长按自动重复（线性加速）
 * b2=PB2(上) b3=PB3(下) b4=PB4(确定) P3.2(返回)
 * 去抖 20ms（连续 2 次采样稳定）；长按 200ms 启动 → 250ms 初间隔 → 每次-50ms → 最小 10ms */
#include "KEY.h"

#define KEY1_PIN        IO_PB2
#define KEY2_PIN        IO_PB3
#define KEY3_PIN        IO_PB4
#define KEY4_PIN        IO_P32

#define HOLD_STARTUP_TICKS   20   /* 200ms — 区分短按/长按 */
#define REPEAT_INIT_TICKS    25   /* 250ms — 首次重复间隔 */
#define REPEAT_ACCEL_TICKS   5    /* 每次重复加速 5 ticks (50ms) */
#define REPEAT_MIN_TICKS     1    /* 10ms — 最快重复间隔 */
#define DEBOUNCE_TICKS       2    /* 20ms 去抖 */

uint8 key1_flag, key2_flag, key3_flag, key4_flag;

static const gpio_pin_enum key_pins[4] = { KEY1_PIN, KEY2_PIN, KEY3_PIN, KEY4_PIN };
static uint8 key_last[4]      = {1,1,1,1};   /* 去抖确认电平（0=按下, 1=释放） */
static uint8 debounce_cnt[4]  = {0,0,0,0};
static uint8 hold_ticks[4]    = {0,0,0,0};
static uint8 repeat_cnt[4]    = {0,0,0,0};
static uint8 repeat_cd[4]     = {0,0,0,0};
static uint8 repeat_active[4] = {0,0,0,0};

void button_init(void)
{
    gpio_init(KEY1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY2_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY3_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY4_PIN, GPI, 1, GPI_PULL_UP);
}

void button_control(uint8 repeat_mask)   /* 每 10ms 调用一次 */
{
    uint8 i, st;
    uint8 *flags[4] = { &key1_flag, &key2_flag, &key3_flag, &key4_flag };

    for (i = 0; i < 4; i++)
    {
        st = gpio_get_level(key_pins[i]);   /* 0=按下(上拉开路), 1=释放 */

        /* 去抖：电平变化需连续 DEBOUNCE_TICKS 次稳定采样才确认（防机械弹跳跨采样点） */
        if (st != key_last[i])
        {
            if (++debounce_cnt[i] >= DEBOUNCE_TICKS)
            {
                debounce_cnt[i] = 0;
                key_last[i] = st;
                if (st)                     /* 释放沿（0→1）触发一次；长按重复中松开不补触发 */
                {
                    if (!repeat_active[i]) *flags[i] = 1;
                    repeat_active[i] = 0;
                }
            }
            continue;                       /* 抖动未确认，跳过重复逻辑 */
        }
        debounce_cnt[i] = 0;

        /* 长按自动重复（仅确认按下时计数） */
        if ((repeat_mask & (1 << i)) && !key_last[i])
        {
            if (++hold_ticks[i] >= HOLD_STARTUP_TICKS)
            {
                repeat_active[i] = 1;
                if (repeat_cd[i] == 0)
                {
                    *flags[i] = 1;
                    /* 线性加速：间隔 = max(MIN, INIT - ACCEL*次数) */
                    repeat_cd[i] = (REPEAT_INIT_TICKS > repeat_cnt[i] * REPEAT_ACCEL_TICKS)
                                 ? (uint8)(REPEAT_INIT_TICKS - repeat_cnt[i] * REPEAT_ACCEL_TICKS)
                                 : REPEAT_MIN_TICKS;
                    repeat_cnt[i]++;
                }
                else repeat_cd[i]--;
            }
        }
        else { hold_ticks[i] = 0; repeat_cnt[i] = 0; repeat_cd[i] = 0; }
    }
}
