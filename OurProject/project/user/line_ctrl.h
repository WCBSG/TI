#ifndef __LINE_CTRL_H__
#define __LINE_CTRL_H__

#include "zf_common_headfile.h"

extern int16 line_duty_lr;         /* 最近一次左轮输出 duty（诊断显示） */
extern int16 line_duty_rr;         /* 最近一次右轮输出 duty（诊断显示） */
extern int16 line_turn;            /* 最近一次转向量 turn（诊断显示） */
extern int16 steer_kp;             /* 纯比例增益（官方 Kp=280） */
extern int16 steer_smooth;         /* 转向低通系数 0-100（弯道平滑，菜单可调） */
extern int16 steer_lim;            /* 转向轮加速上限 % 100-200（弯道速度差控制） */
extern int16 curve_decel;          /* 弯道减速 % 0-50（误差大降 base，球稳有利） */

void line_ctrl_init(void);                    /* 纯比例参数初始化 */
void line_ctrl_set(int error, int base_speed); /* 偏差→纯比例→差速 duty */

#endif
