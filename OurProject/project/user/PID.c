/*********************************************************************************************************************
* 文件名称          PID.c
* 说明              增量式 PID 控制器 — int32 安全运算 + anti-windup
*
* Ki 换算：Ki 以 0.1 为单位（Ki=3 → 有效 Ki=0.3），每次更新时 /10，
* 避免小数值被整数除法截断为 0。
********************************************************************************************************************/

#include "PID.h"
#include "config.h"

PID_t motor1_pid;
PID_t motor2_pid;

void PID_Update(PID_t *p)
{
    int32 kp_term, ki_term, kd_term;

    p->Error2 = p->Error1;
    p->Error1 = p->Error0;
    p->Error0 = p->Target - p->Actual;

    /*
     * C251 中 int=16 位，必须全部转为 int32 再乘，否则溢出。
     */
    kp_term = (int32)p->Kp * (p->Error0 - p->Error1);
    ki_term = ((int32)p->Ki * p->Error0) / 10;
    kd_term = (int32)p->Kd * (p->Error0 - 2 * p->Error1 + p->Error2);

    /*
     * Anti-windup 策略：先加 P+D（不含 I），钳位后再决定是否加 I。
     *   - 饱和 → 冻结积分，防止积分持续累积（windup）
     *   - 未饱和 → 正常累加积分
     */
    p->Out = (int16)(p->Out + kp_term + kd_term);

    if (p->Out > p->OutMax)
    {
        p->Out = p->OutMax;
        /* 饱和：跳过积分累加 */
    }
    else if (p->Out < p->OutMin)
    {
        p->Out = p->OutMin;
        /* 饱和：跳过积分累加 */
    }
    else
    {
        /* 未饱和：累加积分，再钳位一次 */
        p->Out = (int16)(p->Out + ki_term);
        if (p->Out > p->OutMax) p->Out = p->OutMax;
        if (p->Out < p->OutMin) p->Out = p->OutMin;
    }
}

static void motor_pid_init_core(PID_t *p)
{
    p->Kp     = 15;
    p->Ki     = 3;
    p->Kd     = 0;
    p->Target = 0;    /* 发车时由 main.c 动态设定 */
    p->OutMax = 1500;
    p->OutMin = -1500;

    if (config_valid())
    {
        p->Kp = flash_buff[0];
        p->Ki = flash_buff[1];
        p->Kd = flash_buff[2];
    }
}

void motor1_pid_init(void) { motor_pid_init_core(&motor1_pid); }
void motor2_pid_init(void) { motor_pid_init_core(&motor2_pid); }
