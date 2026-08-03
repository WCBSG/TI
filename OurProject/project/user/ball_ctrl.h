/*********************************************************************************************************************
* 文件名称          ball_ctrl.h
* 说明              球稳控制环 — 舵机摆杆 + 球位置 PID
*
* 硬件：摆杆角度控制机构 = 舵机（PWME_CH1P_PA0 @ 50Hz）
*   duty 0.5ms(250) ~ 2.5ms(1250)，中位 1.5ms(750) 对应摆杆水平
*
* 反馈：OpenART 球像素 X（protocol.h）→ 0.1cm（相对摆杆中心 O）
*   cm_x10 = (pixel - pixel_zero) * 10 / px_per_cm
*
* 控制：目标 cm - 球 cm → 位置式 PID（PID_Update）→ 舵机 duty 偏移
********************************************************************************************************************/

#ifndef __BALL_CTRL_H__
#define __BALL_CTRL_H__

#include "zf_common_headfile.h"
#include "PID.h"
#include "protocol.h"      /* proto_ball_x, proto_ball_valid */

/* ── 舵机硬件 ── */
#define SERVO_PWM           (PWME_CH1P_PA0)   /* 主板舵机接口 PWM1 = A0 */
#define SERVO_FREQ          300               /* 舵机+灯带共 300Hz（PWME 同组同频，仅数字舵机适用） */
/* 300Hz 周期=3.33ms，脉宽→duty（PWM_DUTY_MAX=10000）：0.5ms=1500 / 1.5ms=4500 / 2.5ms=7500
 * 实测有效行程 duty 3500~5500（中位 4500），超出范围打齿/无效，故钳位在此区间 */
#define SERVO_DUTY_MIN      3500              /* 实测行程下限（摆杆一端） */
#define SERVO_DUTY_MAX      5500              /* 实测行程上限（摆杆另一端） */
#define SERVO_DUTY_CENTER   4500              /* 中位（摆杆水平） */

/* ── 标定参数（config 持久化，菜单可调） ── */
extern int16 servo_center_duty;   /* 舵机中位 duty（摆杆水平时的脉宽） */
extern int16 pixel_zero;          /* O 点对应的球像素 X */
extern int16 px_per_cm;           /* 每厘米对应像素数（现场标定） */
extern int16 ball_target_cm_x10;  /* 球目标位置（0.1cm，相对 O；任务6 用） */

extern PID_t ball_pid;            /* 球位置 PID（位置式） */
extern int16 ball_cm_x10;         /* 最近一次球位置（0.1cm，相对 O；tick 后更新） */
extern int16 ball_duty_out;       /* 最近一次舵机输出 duty（诊断显示） */

/* ── 接口 ── */
void ball_ctrl_init(void);                    /* 舵机 PWM + 球 PID + 标定加载 */
void ball_ctrl_set_target(int16 cm_x10);      /* 设定球目标位置（0.1cm，相对 O） */
void ball_ctrl_tick(void);                    /* 10ms 周期：读球位→cm→PID→舵机 */
void ball_ctrl_stop(void);                    /* 停止：舵机回中位 */

#endif
