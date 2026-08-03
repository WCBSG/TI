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

/* 积分条件阈值（0.1cm 单位，20=2cm）：误差 >2cm 不积分（防猛推振荡）
 * 曾设 10=1cm：稳态误差 1.6cm 时误差 16>10 积分不工作 → 稳态误差消不掉 */
#define PID_I_THRESH   20

void PID_Reset(PID_t *p)
{
    p->Actual   = 0;
    p->Out      = 0;
    p->Error0   = 0;
    p->Error1   = 0;
    p->Error2   = 0;
    p->Integral = 0;
    p->DFilter  = 0;
}

void PID_Update(PID_t *p)
{
    int32 p_term, d_term, i_term, out, ki_inc, next_integral;
    int16 abs_err;

    p->Error2 = p->Error1;
    p->Error1 = p->Error0;
    p->Error0 = (int16)((int32)p->Target - (int32)p->Actual);

    /*
     * C251 中 int=16 位，Error 差分先提升 int32 再运算，避免溢出。
     */
    p_term = (int32)p->Kp * (int32)p->Error0;

    /* D 项低通滤波，状态放大 16 倍以保留小误差差分。 */
    p->DFilter += ((((int32)p->Error0 - (int32)p->Error1) * 16) - p->DFilter) / 16;
    d_term = ((int32)p->Kd * p->DFilter) / 16;

    out = p_term + d_term;

    abs_err = (p->Error0 < 0) ? (int16)(-p->Error0) : p->Error0;

    if (abs_err < PID_I_THRESH)
    {
        /* 误差 < 1cm 才积分：真累积消除稳态误差（学长：误差小才破静摩擦） */
        ki_inc = (int32)p->Ki * (int32)p->Error0;
        next_integral = p->Integral + ki_inc;
        if (next_integral >  2000) next_integral =  2000;
        if (next_integral < -2000) next_integral = -2000;
        i_term = next_integral / 10;

        /* 最终输出未饱和，或积分有助于退出饱和时才提交。 */
        if ((out + i_term <= p->OutMax && out + i_term >= p->OutMin) ||
            (out + i_term > p->OutMax && ki_inc < 0) ||
            (out + i_term < p->OutMin && ki_inc > 0))
        {
            p->Integral = next_integral;
        }
        out += p->Integral / 10;
    }
    else
    {
        /* 大误差：积分清零（防积分猛推振荡） */
        p->Integral = 0;
    }

    if (out > p->OutMax) out = p->OutMax;
    if (out < p->OutMin) out = p->OutMin;

    /* int32 域钳位完成后才截断为 int16 输出 */
    p->Out = (int16)out;
}
