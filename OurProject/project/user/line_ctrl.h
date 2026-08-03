#ifndef __LINE_CTRL_H__
#define __LINE_CTRL_H__

#include "zf_common_headfile.h"

extern int16 line_duty_lr;         /* 最近一次左轮输出 duty（诊断显示） */
extern int16 line_duty_rr;         /* 最近一次右轮输出 duty（诊断显示） */
extern int16 line_turn;            /* 最近一次转向量 turn（诊断显示） */
extern int16 steer_kp;             /* 纯比例增益（官方 Kp=280） */

void line_ctrl_init(void);                    /* 纯比例参数初始化 */
void line_ctrl_set(int error, int base_speed); /* 偏差→纯比例→差速 duty */

#endif
