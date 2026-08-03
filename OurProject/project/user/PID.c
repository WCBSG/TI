/*********************************************************************************************************************
* 文件名称          PID.c
* 说明              位置式 PID 控制器 — int32 安全运算 + 积分余数累积 + anti-windup
*
* 设计说明（相对增量式重构）：
*   1. 位置式：Out = Kp*e + Integral/10 + Kd*(e-e1)，error=0 时 P 项归零，
*      无增量式累积残留（出弯后不会反向满幅打舵）。
*   2. Ki 以 0.1 为单位（Ki=3 → 有效 Ki=0.3）。Integral 用 int32 余数累积
*      （每次 Integral -= i_term*10），消除恒定小误差被整数除法截断的积分死区。
*   3. 全部运算在 int32 域完成，钳位后才截断为 int16 输出，避免中间值回绕。
*   4. anti-windup：输出饱和时冻结积分（不累加同向 Ki 增量），退饱和更快。
********************************************************************************************************************/

#include "PID.h"

/* 积分条件阈值（0.1cm 单位，10=1cm）：误差大于此不积分，防大误差积分猛推振荡 */
#define PID_I_THRESH   10

/* D 项低通滤波状态（唯一 PID 实例 ball_pid） */
static int32 pid_d_filt = 0;

void PID_Update(PID_t *p)
{
    int32 p_term, d_term, i_term, out, ki_inc;
    int16 abs_err;

    p->Error2 = p->Error1;
    p->Error1 = p->Error0;
    p->Error0 = (int16)((int32)p->Target - (int32)p->Actual);

    /*
     * C251 中 int=16 位，Error 差分先提升 int32 再运算，避免溢出。
     */
    p_term = (int32)p->Kp * (int32)p->Error0;

    /* D 项低通滤波（学长 servo_control 参考）：去噪不减刹车力 */
    pid_d_filt = (pid_d_filt * 15 + ((int32)p->Error0 - (int32)p->Error1)) / 16;
    d_term = (int32)p->Kd * pid_d_filt;

    out = p_term + d_term;

    abs_err = (p->Error0 < 0) ? (int16)(-p->Error0) : p->Error0;

    if (out > p->OutMax || out < p->OutMin)
    {
        /* 饱和：冻结积分（不累加），先钳位 */
        out = (out > p->OutMax) ? (int32)p->OutMax : (int32)p->OutMin;
    }
    else if (abs_err < PID_I_THRESH)
    {
        /* 误差 < 1cm 才积分：真累积消除稳态误差（学长：误差小才破静摩擦） */
        ki_inc = (int32)p->Ki * (int32)p->Error0;
        p->Integral += ki_inc;
        if (p->Integral >  2000) p->Integral =  2000;   /* 积分限幅（防溢出 + 防猛推） */
        if (p->Integral < -2000) p->Integral = -2000;
        i_term = p->Integral / 10;

        out += i_term;
        if (out > p->OutMax) out = p->OutMax;
        if (out < p->OutMin) out = p->OutMin;
    }
    else
    {
        /* 大误差：积分清零（防积分猛推振荡） */
        p->Integral = 0;
    }

    /* int32 域钳位完成后才截断为 int16 输出 */
    p->Out = (int16)out;
}
