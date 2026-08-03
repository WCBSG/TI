/*********************************************************************************************************************
* 文件名称          ball_ctrl.c
* 说明              球稳控制环实现 — 舵机摆杆 + 球位置 PID
*
* 数据流（每 10ms tick）：
*   proto_ball_x(像素) --标定--> ball_cm_x10(0.1cm) --球PID--> 舵机duty偏移
*
* 丢球保护：proto_ball_valid==0 时保持上次输出（不归零，防球乱滚）
********************************************************************************************************************/

#include "ball_ctrl.h"
#include "config.h"

/* ── 标定参数默认值（实测：2026-08） ── */
int16 servo_center_duty = SERVO_DUTY_CENTER;   /* 4500 摆杆水平 */
int16 pixel_zero        = 175;    /* 实测：O 点像素 X */
int16 px_per_cm         = 11;     /* 实测：每 cm 像素数（±5cm 远端线性拟合，10.6~11.4） */
int16 ball_target_cm_x10 = 0;

PID_t ball_pid;
int16 ball_cm_x10    = 0;
int16 ball_duty_out  = SERVO_DUTY_CENTER;   /* 诊断：最近一次舵机输出 duty */

void ball_ctrl_init(void)
{
    pwm_init(SERVO_PWM, SERVO_FREQ, SERVO_DUTY_CENTER);   /* 舵机置中位 */

    /* 球 PID：积分真累积后 Ki=2 太强 → 球 ±14cm 大幅振荡（积分猛推过冲）
     * 调参：Ki 2→1（温和积分只消除稳态误差），积分限幅收紧（PID.c ±2000） */
    ball_pid.Kp     = 8;
    ball_pid.Ki     = 1;          /* 积分：真累积，只补偿稳态误差 */
    ball_pid.Kd     = 15;         /* 阻尼：抑制过冲振荡 */
    ball_pid.OutMax = 300;        /* 舵机 duty 偏移上限（4500±300 在行程内） */
    ball_pid.OutMin = -300;
    ball_pid.Target = 0;

    if (config_valid())
    {
        ball_pid.Kp         = flash_buff[2];
        ball_pid.Ki         = flash_buff[3];
        ball_pid.Kd         = flash_buff[4];
        ball_pid.OutMax     = flash_buff[5];
        ball_pid.OutMin     = flash_buff[6];
        servo_center_duty   = flash_buff[7];
        pixel_zero          = flash_buff[8];
        px_per_cm           = flash_buff[9];
        ball_target_cm_x10  = flash_buff[10];
    }
}

void ball_ctrl_set_target(int16 cm_x10)
{
    ball_pid.Target = cm_x10;
}

void ball_ctrl_tick(void)
{
    int32 duty;
    /* ⚠️ 不能用局部 ball_cm_x10：会 shadow 全局（任务 3 判定/显示用的全局值） */

    /* 丢球：保持上次输出 */
    if (!proto_ball_valid) return;
    if (px_per_cm <= 0) return;

    /* 像素 → 0.1cm（相对 O 点） */
    ball_cm_x10 = (int16)(((int32)(proto_ball_x - pixel_zero) * 10) / px_per_cm);

    ball_pid.Actual = ball_cm_x10;    PID_Update(&ball_pid);

    /* 舵机 duty = 中位 + PID 输出，钳位在行程内 */
    duty = (int32)servo_center_duty + ball_pid.Out;
    if (duty < SERVO_DUTY_MIN) duty = SERVO_DUTY_MIN;
    if (duty > SERVO_DUTY_MAX) duty = SERVO_DUTY_MAX;

    ball_duty_out = (int16)duty;
    pwm_set_duty(SERVO_PWM, (uint32)duty);
}

void ball_ctrl_stop(void)
{
    pwm_set_duty(SERVO_PWM, SERVO_DUTY_CENTER);
}
