#ifndef __LINE_CTRL_H__
#define __LINE_CTRL_H__

#include "zf_common_headfile.h"

extern int16 line_duty_lr;         /* 最近一次左轮输出 duty（诊断显示） */
extern int16 line_duty_rr;         /* 最近一次右轮输出 duty（诊断显示） */
extern int16 line_turn;            /* 最近一次转向量 turn（诊断显示） */
extern int16 steer_kp;             /* 激活 Kp（纯比例增益） */
extern int16 steer_smooth;         /* 激活转向低通 0-100 */
extern int16 steer_lim;            /* 激活加速上限 % 100-200 */
extern int16 curve_decel;          /* 激活弯道减速 % 0-50 */

/* 两套任务默认参数（菜单分别编辑） */
extern int16 kp_t2, smooth_t2, lim_t2, dec_t2;    /* 任务 2 */
extern int16 kp_ot, smooth_ot, lim_ot, dec_ot;    /* 其他任务 */

void line_ctrl_init(void);                    /* 纯比例参数初始化（apply_other） */
void line_ctrl_apply_t2(void);                /* 应用任务 2 参数 */
void line_ctrl_apply_other(void);             /* 应用其他任务参数 */
void line_ctrl_set(int error, int base_speed); /* 偏差→单轮差速 duty */

#endif
