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
    steer_pid.Actual  = error;
    PID_Update(&steer_pid);

    motor1_pid.Target = base_speed - steer_pid.Out;
    motor2_pid.Target = base_speed + steer_pid.Out;
}

