/* line_ctrl.c — 巡线环：纯比例 P + 转向低通 + 弯道减速 + 加速上限
 * turn = error*Kp（低通）；单轮差速 left=spd+turn（左轮调），right=spd（右轮恒定）
 * error>0 偏右 → 左轮快 → 右转（方向已实测） */
#include "line_ctrl.h"
#include "Motor.h"

int16 line_duty_lr = 0;   /* 诊断：最近左轮输出 duty */
int16 line_duty_rr = 0;   /* 诊断：最近右轮输出 duty */
int16 line_turn = 0;      /* 诊断：最近转向量 turn */

int16 steer_kp = 260;     /* 激活 Kp */
int16 steer_smooth = 3;   /* 转向低通 0-100 */
int16 steer_lim = 150;    /* 加速上限 % 100-200 */
int16 curve_decel = 15;   /* 弯道减速 % 0-50 */

int16 kp_t2 = 290, smooth_t2 = 4, lim_t2 = 200, dec_t2 = 25;  /* 任务 2：更快 */
int16 kp_ot = 260, smooth_ot = 4, lim_ot = 180, dec_ot = 20;  /* 其他任务：更稳 */

static int32 turn_filt = 0;

void line_ctrl_init(void)         { line_ctrl_apply_other(); turn_filt = 0; }
void line_ctrl_apply_t2(void)     { steer_kp = kp_t2; steer_smooth = smooth_t2; steer_lim = lim_t2; curve_decel = dec_t2; }
void line_ctrl_apply_other(void)  { steer_kp = kp_ot; steer_smooth = smooth_ot; steer_lim = lim_ot; curve_decel = dec_ot; }

void line_ctrl_set(int error, int base_speed)
{
    int32 turn, left_duty, right_duty, spd;
    int   abs_err = (error < 0) ? -error : error;

    /* 弯道减速：|error|>2 时 base 降 curve_decel%，避免弯道加速球不稳 */
    spd = base_speed;
    if (curve_decel > 0 && abs_err > 2) spd = (int32)base_speed * (100 - curve_decel) / 100;

    /* 转向低通：误差离散跳变时 turn 渐变，弯道划弧（高速巡线需小值 1~10） */
    turn_filt += ((int32)error * steer_kp - turn_filt) * steer_smooth / 100;
    turn = turn_filt;
    line_turn = (int16)turn;

    left_duty  = spd + turn;    /* 左轮调 */
    right_duty = spd;           /* 右轮恒定 */

    /* 加速上限：左轮 [spd/3, spd*Lim/100]，弯道速度差小 */
    if (left_duty > spd * steer_lim / 100) left_duty = spd * steer_lim / 100;
    if (left_duty < spd / 3) left_duty = spd / 3;

    line_duty_lr = (int16)left_duty;
    line_duty_rr = -(int16)right_duty;
    motor1_control((int16)left_duty);
    motor2_control(-(int16)right_duty);
}
