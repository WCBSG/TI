/*********************************************************************************************************************
* 文件名称          KEY.h
* 说明              4 键按键检测 + 长按自动重复（线性加速）
*                   Motherboard 主板按键：b2=PB2 b3=PB3 b4=PB4 P3.2=P32
*
* 使用：
*   button_init();
*   // 主循环每 10ms:
*   button_control(KEY_REPEAT_KEY1 | KEY_REPEAT_KEY2);
*   if (key1_flag) { key1_flag = 0; ... }
*
* 去抖：电平变化需连续 2 次采样（20ms）稳定才确认，防止机械弹跳误触发
* 长按自动重复参数（基于 10ms 调用周期）：
*   启动延迟: 200ms (20 ticks)   初始间隔: 250ms (25 ticks)
*   加速速率: -50ms/次 (5 ticks)  最小间隔:  10ms (1 tick)
* 长按重复中松开不额外触发释放沿（长按已由重复覆盖）
********************************************************************************************************************/

#ifndef __KEY_H__
#define __KEY_H__

#include "zf_common_headfile.h"

/* ── 自动重复掩码 ── */
#define KEY_REPEAT_KEY1   0x01
#define KEY_REPEAT_KEY2   0x02
#define KEY_REPEAT_KEY3   0x04
#define KEY_REPEAT_KEY4   0x08

extern uint8 key1_flag, key2_flag, key3_flag, key4_flag;

void button_init(void);
void button_control(uint8 repeat_mask);  /* repeat_mask: 按位指定哪些键启用长按自动重复 */

#endif