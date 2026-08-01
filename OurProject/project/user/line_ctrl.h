/*********************************************************************************************************************
* 文件名称          line_ctrl.h
* 说明              巡线环 — 红外偏差 → 差速 PWM（开环）
*
* 数据流（任务层每 10ms 调用）：
*   calc_error(IRPHOTO) --error--> line_ctrl_set(err, base_speed)
*     → 弯道自适应减速 → steer PID → 左右轮 duty → motor1/2_control
********************************************************************************************************************/

#ifndef __LINE_CTRL_H__
#define __LINE_CTRL_H__

#include "zf_common_headfile.h"
#include "PID.h"

extern PID_t steer_pid;            /* 差速 PID（位置式） */
extern int16 line_duty_lr;         /* 最近一次左轮输出 duty（诊断显示） */
extern int16 line_duty_rr;         /* 最近一次右轮输出 duty（诊断显示） */
extern int16 kd_yaw;               /* 陀螺仪阻尼系数（默认 0=不生效） */

void line_ctrl_init(void);                    /* 差速 PID 初始化 + 配置加载 */
void line_ctrl_set(int error, int base_speed); /* 每 10ms：偏差→差速 duty 输出 */

#endif
