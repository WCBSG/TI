#include "zf_common_headfile.h"
#include "control.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "PID.h"

void main(void)
{
    int s[8];

    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    tft180_init();

    IRPHOTO_Init();
    Motor_Init();
    encoder_init();
    motor1_pid_init();
    motor2_pid_init();
    steer_init();

    pit_ms_init(PIT_ENCODER, 5, pit_handler);

    tft180_set_color(0x0000, 0xFFFF);
    tft180_clear(0xFFFF);

    driving = 1;

    while (1)
    {
        IRPHOTO_Read(s);

        if (is_stop(s))
        {
            motor1_control(0);
            motor2_control(0);
            break;
        }

        steer_set(calc_error(s), motor1_pid.Target);
        system_delay_ms(10);
    }
}
