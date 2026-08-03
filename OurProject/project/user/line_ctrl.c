/*********************************************************************************************************************
* 文件名称          line_ctrl.c
* 说明              巡线环实现 — 官方纯比例 P 控制
*
*   turn = error * Kp(260)     （偏差→转向量，低通平滑）
*   left = base + turn         （单轮差速：左轮调，右轮恒定 base）
*   right = base               （error>0 偏右→左轮快→左快右慢→右转追线）
*
* 单轮差速：直线更顺（右轮恒定），弯道转向力度弱于双轮（左轮不反转）
********************************************************************************************************************/

#include "line_ctrl.h"
#include "Motor.h"
#include "config.h"

int16 line_duty_lr = 0;   /* 诊断：最近一次左轮输出 duty */
int16 line_duty_rr = 0;   /* 诊断：最近一次右轮输出 duty */
int16 line_turn = 0;      /* 诊断：最近一次转向量 turn */

/* ═══ 巡线参数：激活值（line_ctrl_set 使用）+ 两套任务默认 ═══ */
int16 steer_kp = 260;     /* 激活 Kp（任务启动时由 apply 设置） */
int16 steer_smooth = 3;   /* 激活转向低通 0-100 */
int16 steer_lim = 150;    /* 激活加速上限 % 100-200 */
int16 curve_decel = 15;   /* 激活弯道减速 % 0-50 */

int16 kp_t2 = 290, smooth_t2 = 4, lim_t2 = 200, dec_t2 = 25;  /* 任务 2：更快 */
int16 kp_ot = 260, smooth_ot = 4, lim_ot = 180, dec_ot = 20;  /* 其他（5/6 球稳）：更稳 */

static int32 turn_filt = 0;   /* 转向滤波值 */

void line_ctrl_init(void)
{
    line_ctrl_apply_other();   /* 默认其他任务参数 */
    turn_filt = 0;
}

void line_ctrl_apply_t2(void)
{
    steer_kp = kp_t2; steer_smooth = smooth_t2; steer_lim = lim_t2; curve_decel = dec_t2;
}

void line_ctrl_apply_other(void)
{
    steer_kp = kp_ot; steer_smooth = smooth_ot; steer_lim = lim_ot; curve_decel = dec_ot;
}

void line_ctrl_set(int error, int base_speed)
{
    int32 turn, left_duty, right_duty, spd;
    int   abs_err = (error < 0) ? -error : error;

    /* 弯道自适应减速：误差大时 base 降 curve_decel%（球稳有利，避免弯道加速车速过快） */
    spd = base_speed;
    if (curve_decel > 0 && abs_err > 2)
        spd = (int32)base_speed * (100 - curve_decel) / 100;

    /* 转向低通滤波：误差离散跳变时 turn 渐变，消除弯道多边形折线
     * 误差权重间隔 3 → turn 阶跃 3*Kp，滤波让转向平滑划弧
     * 高速巡线循环 ~1kHz，steer_smooth 需小值（1~10）才有平滑效果 */
    turn_filt += ((int32)error * steer_kp - turn_filt) * steer_smooth / 100;
    turn = turn_filt;
    line_turn = (int16)turn;

    /* ═══ 前进单轮差速：左轮随 turn 调，右轮恒 spd
     *   error>0 偏右 → turn>0 → left 增 → 左轮快 → 左快右慢 → 右转 ✓
     *   error<0 偏左 → turn<0 → left 减 → 左轮慢 → 左慢右快 → 左转 ✓ */
    left_duty  = spd + turn;     /* 左轮调 */
    right_duty = spd;            /* 右轮恒定 */

    /* 钳位：左轮 [spd/3, spd*steer_lim/100]（限制加速上限，弯道速度差小） */
    if (left_duty > spd * steer_lim / 100) left_duty = spd * steer_lim / 100;
    if (left_duty < spd / 3) left_duty = spd / 3;

    line_duty_lr = (int16)left_duty;
    line_duty_rr = -(int16)right_duty;
    motor1_control((int16)left_duty);
    motor2_control(-(int16)right_duty);
}
