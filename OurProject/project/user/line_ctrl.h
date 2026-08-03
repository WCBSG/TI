/* line_ctrl.h — 巡线环：纯比例 + 转向低通 + 弯道减速 + 加速上限 */
#ifndef __LINE_CTRL_H__
#define __LINE_CTRL_H__

#include "zf_common_headfile.h"

extern int16 line_duty_lr, line_duty_rr, line_turn;   /* 诊断输出 duty/turn */
extern int16 steer_kp, steer_smooth, steer_lim, curve_decel;   /* 激活参数 */
extern int16 kp_t2, smooth_t2, lim_t2, dec_t2;        /* 任务 2 参数 */
extern int16 kp_ot, smooth_ot, lim_ot, dec_ot;        /* 其他任务参数 */

void line_ctrl_init(void);
void line_ctrl_apply_t2(void);       /* 应用任务 2 参数 */
void line_ctrl_apply_other(void);    /* 应用其他任务参数 */
void line_ctrl_set(int error, int base_speed);   /* 偏差→单轮差速 duty */

#endif
