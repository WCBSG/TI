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

void PID_Update(PID_t *p)
{
    int32 p_term, d_term, i_term, out, ki_inc;

    p->Error2 = p->Error1;
    p->Error1 = p->Error0;
    p->Error0 = (int16)((int32)p->Target - (int32)p->Actual);

    /*
     * C251 中 int=16 位，Error 差分先提升 int32 再运算，避免溢出。
     */
    p_term = (int32)p->Kp * (int32)p->Error0;
    d_term = (int32)p->Kd * ((int32)p->Error0 - (int32)p->Error1);

    out = p_term + d_term;

    if (out > p->OutMax || out < p->OutMin)
    {
        /* 饱和：冻结积分（不累加），先钳位 */
        out = (out > p->OutMax) ? (int32)p->OutMax : (int32)p->OutMin;
    }
    else
    {
        /* 未饱和：累加积分（Ki 以 0.1 为单位），余数保留消除死区 */
        ki_inc = (int32)p->Ki * (int32)p->Error0;
        p->Integral += ki_inc;
        i_term = p->Integral / 10;
        p->Integral -= i_term * 10;

        out += i_term;
        if (out > p->OutMax) out = p->OutMax;
        if (out < p->OutMin) out = p->OutMin;
    }

    /* int32 域钳位完成后才截断为 int16 输出 */
    p->Out = (int16)out;
}
