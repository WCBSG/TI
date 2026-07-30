#include "control.h"
#include "PID.h"
#include "Motor.h"

PID_t steer_pid;

void steer_init(void)
{
    steer_pid.Kp     = 5;
    steer_pid.Ki     = 0;
    steer_pid.Kd     = 0;
    steer_pid.OutMax = 20;
    steer_pid.OutMin = -20;
}

void steer_set(int error, int base_speed)
{
    steer_pid.Actual  = error;
    PID_Update(&steer_pid);

    motor1_pid.Target = base_speed - steer_pid.Out;
    motor2_pid.Target = base_speed + steer_pid.Out;
}

