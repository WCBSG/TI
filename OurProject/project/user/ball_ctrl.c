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

/* ── 标定参数默认值（现场标定后经 config 持久化） ── */
int16 servo_center_duty = SERVO_DUTY_CENTER;
int16 pixel_zero        = 160;    /* 假设 O 点在图像中心，待标定 */
int16 px_per_cm         = 20;     /* 假设 20px/cm，待标定 */
int16 ball_target_cm_x10 = 0;

PID_t ball_pid;
int16 ball_cm_x10    = 0;
int16 ball_duty_out  = SERVO_DUTY_CENTER;   /* 诊断：最近一次舵机输出 duty */

void ball_ctrl_init(void)
{
    pwm_init(SERVO_PWM, SERVO_FREQ, SERVO_DUTY_CENTER);   /* 舵机置中位 */

    ball_pid.Kp     = 30;
    ball_pid.Ki     = 2;          /* 以 0.1 为单位，有效 0.2 */
    ball_pid.Kd     = 0;
    ball_pid.OutMax = 300;        /* 舵机 duty 偏移上限（750±300 在行程内） */
    ball_pid.OutMin = -300;
    ball_pid.Target = 0;

    if (config_valid())
    {
        ball_pid.Kp         = flash_buff[12];
        ball_pid.Ki         = flash_buff[13];
        ball_pid.Kd         = flash_buff[14];
        ball_pid.OutMax     = flash_buff[15];
        ball_pid.OutMin     = flash_buff[16];
        servo_center_duty   = flash_buff[17];
        pixel_zero          = flash_buff[18];
        px_per_cm           = flash_buff[19];
        ball_target_cm_x10  = flash_buff[20];
    }
}

void ball_ctrl_set_target(int16 cm_x10)
{
    ball_pid.Target = cm_x10;
}

void ball_ctrl_tick(void)
{
    int32 duty;
    int16 ball_cm_x10;

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
