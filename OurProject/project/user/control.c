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
    int16 left_target, right_target, rev_limit;

    steer_pid.Actual  = error;
    PID_Update(&steer_pid);

    left_target  = base_speed - steer_pid.Out;
    right_target = base_speed + steer_pid.Out;

    /*
     * 钳位：上限 base_speed，下限 -base_speed/3（允许内侧轮适度反转，
     * 改善 0.5m 半径弯道跟踪）。低速时反转才有意义。
     */
    rev_limit = (int16)(-(base_speed / 3));
    if (left_target  > base_speed) left_target  = (int16)base_speed;
    if (right_target > base_speed) right_target = (int16)base_speed;
    if (left_target  < rev_limit)  left_target  = rev_limit;
    if (right_target < rev_limit)  right_target = rev_limit;

    /* motor1 反向安装（负 Target → 前进），motor2 正常安装 */
    motor1_pid.Target = -left_target;
    motor2_pid.Target = right_target;
}

