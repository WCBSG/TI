#include "control.h"
#include "PID.h"
#include "Motor.h"
#include "config.h"

PID_t steer_pid;

void steer_init(void)
{
    /*
     * 差速开环：唯一 PID 输出作用于"传感器误差 → 左右轮 PWM duty 差"。
     * base_speed 是基准 duty（0-10000 满占空比），Out 是差速 duty 量。
     */
    steer_pid.Kp     = 40;
    steer_pid.Ki     = 0;
    steer_pid.Kd     = 0;
    steer_pid.OutMax = 300;
    steer_pid.OutMin = -300;

    if (config_valid())
    {
        steer_pid.Kp     = flash_buff[0];
        steer_pid.Ki     = flash_buff[1];
        steer_pid.Kd     = flash_buff[2];
        steer_pid.OutMax = flash_buff[3];
        steer_pid.OutMin = flash_buff[4];
    }
}

void steer_set(int error, int base_speed)
{
    int16 left_duty, right_duty, rev_limit;

    /* 位置式差速 PID：Target 恒 0，Error = 0 - error（正值=偏右） */
    steer_pid.Actual = error;
    PID_Update(&steer_pid);

    left_duty  = (int16)(base_speed - steer_pid.Out);
    right_duty = (int16)(base_speed + steer_pid.Out);

    /*
     * 钳位：上限 base_speed，下限 -base_speed/3（允许内侧轮适度反转，
     * 改善 0.5m 半径弯道跟踪）。
     */
    rev_limit = (int16)(-(base_speed / 3));
    if (left_duty  > base_speed) left_duty  = (int16)base_speed;
    if (right_duty > base_speed) right_duty = (int16)base_speed;
    if (left_duty  < rev_limit)  left_duty  = rev_limit;
    if (right_duty < rev_limit)  right_duty = rev_limit;

    /* motor1 反向安装（负 duty → 前进），motor2 正常安装 */
    motor1_control(-left_duty);
    motor2_control(right_duty);
}
