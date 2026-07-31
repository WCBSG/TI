/*********************************************************************************************************************
* 文件名称          KEY.h
* 说明              5 键按键检测 + 长按自动重复（线性加速）
*
* 使用：
*   button_init();
*   // 主循环每 50ms:
*   button_control(KEY_REPEAT_KEY1 | KEY_REPEAT_KEY2);
*   if (key1_flag) { key1_flag = 0; ... }
*
* 长按自动重复参数（基于 50ms 调用周期）：
*   启动延迟: 400ms (8 ticks)    初始间隔: 250ms (5 ticks)
*   加速速率: -50ms/次 (1 tick)  最小间隔:  50ms (1 tick)
********************************************************************************************************************/

#ifndef __KEY_H__
#define __KEY_H__

#include "zf_common_headfile.h"

/* ── 自动重复掩码 ── */
#define KEY_REPEAT_KEY1   0x01
#define KEY_REPEAT_KEY2   0x02
#define KEY_REPEAT_KEY3   0x04
#define KEY_REPEAT_KEY4   0x08
#define KEY_REPEAT_KEY5   0x10

extern uint8 key1_flag, key2_flag, key3_flag, key4_flag, key5_flag;

void button_init(void);
void button_control(uint8 repeat_mask);  /* repeat_mask: 按位指定哪些键启用长按自动重复 */

#endif