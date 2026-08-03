/* KEY.h — 4 键按键：释放沿触发 + 长按自动重复（线性加速）
 * b2=PB2 b3=PB3 b4=PB4 P3.2=P32；button_control(KEY_REPEAT_KEY1|...) 每 10ms 调用 */
#ifndef __KEY_H__
#define __KEY_H__

#include "zf_common_headfile.h"

#define KEY_REPEAT_KEY1   0x01
#define KEY_REPEAT_KEY2   0x02
#define KEY_REPEAT_KEY3   0x04
#define KEY_REPEAT_KEY4   0x08

extern uint8 key1_flag, key2_flag, key3_flag, key4_flag;   /* 消费后须清零 */

void button_init(void);
void button_control(uint8 repeat_mask);   /* repeat_mask 按位启用长按自动重复 */

#endif
