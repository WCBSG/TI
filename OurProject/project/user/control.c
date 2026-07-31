#include "control.h"
#include "PID.h"
#include "Motor.h"
#include "config.h"

PID_t steer_pid;

void steer_init(void)
{
    steer_pid.Kp     = 15;
    steer_pid.Ki     = 0;
    steer_pid.Kd     = 0;
    steer_pid.OutMax = 20;
    steer_pid.OutMin = -20;

    if (config_valid())
    {
        steer_pid.Kp     = flash_buff[8];
        steer_pid.Ki     = flash_buff[9];
        steer_pid.Kd     = flash_buff[10];
        steer_pid.OutMax = flash_buff[11];
        steer_pid.OutMin = flash_buff[12];
    }
}

void steer_set(int error, int base_speed)
{
    int16 left_target, right_target;

    steer_pid.Actual  = error;
    PID_Update(&steer_pid);

    left_target  = base_speed - steer_pid.Out;
    right_target = base_speed + steer_pid.Out;

    /* 钳位：轮子最多停转（0），禁止反转 */
    if (left_target  < 0) left_target  = 0;
    if (right_target < 0) right_target = 0;

    /* 左后轮（motor1）反向安装：负 duty → 前进 */
    motor1_pid.Target = -left_target;
    motor2_pid.Target = right_target;
}

