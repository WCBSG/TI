/*********************************************************************************************************************
* 文件名称          line_ctrl.c
* 说明              巡线环实现 — 官方纯比例 P 控制
*
*   turn = error * Kp(280)     （偏差→转向量）
*   left = base + turn         （双轮一加一减，error>0 偏右→左快右慢→右转追线）
*   right = base - turn
*
* 官方例程方案，实测最丝滑（无 PID/渐变/钳位复杂度）
********************************************************************************************************************/

#include "line_ctrl.h"
#include "Motor.h"
#include "config.h"

int16 line_duty_lr = 0;   /* 诊断：最近一次左轮输出 duty */
int16 line_duty_rr = 0;   /* 诊断：最近一次右轮输出 duty */
int16 line_turn = 0;      /* 诊断：最近一次转向量 turn */
int16 steer_kp = 260;     /* 纯比例增益（官方 Kp=280） */

void line_ctrl_init(void)
{
    steer_kp = 260;   /* 硬编码官方值，不读 flash */
}

void line_ctrl_set(int error, int base_speed)
{
    int32 turn, left_duty, right_duty;

    turn = (int32)error * steer_kp;
    line_turn = (int16)turn;

    left_duty  = (int32)base_speed + turn;
    right_duty = (int32)base_speed - turn;

    /* 钳位 ±3000（官方值） */
    if (left_duty  >  3000) left_duty  =  3000;
    if (left_duty  < -3000) left_duty  = -3000;
    if (right_duty >  3000) right_duty =  3000;
    if (right_duty < -3000) right_duty = -3000;

    line_duty_lr = (int16)left_duty;
    line_duty_rr = -(int16)right_duty;
    motor1_control((int16)left_duty);
    motor2_control(-(int16)right_duty);
}
