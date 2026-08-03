/* Motor.c — 电机 PWM 驱动 + 5ms 时基（差速开环，编码器已删） */
#include "Motor.h"

volatile uint32 pit_tick = 0;         /* 5ms 硬件时基 */

void Motor_Init(void)
{
    pwm_init(PWM_1, 17000, 0);
    pwm_init(PWM_2, 17000, 0);
    gpio_init(DIR_1, GPO, 1, GPO_PUSH_PULL);
    gpio_init(DIR_2, GPO, 0, GPO_PUSH_PULL);
}

static void motor_control(gpio_pin_enum dir_pin, pwm_channel_enum pwm_ch, int16 duty)
{   /* duty>=0 正转, duty<0 反转 */
    if (duty >= 0) { gpio_set_level(dir_pin, 0); pwm_set_duty(pwm_ch, duty); }
    else           { gpio_set_level(dir_pin, 1); pwm_set_duty(pwm_ch, (uint32)(-duty)); }
}

void motor1_control(int16 motor_duty) { motor_control(DIR_1, PWM_1, motor_duty); }
void motor2_control(int16 motor_duty) { motor_control(DIR_2, PWM_2, motor_duty); }

void pit_handler(void) { pit_tick++; }   /* 5ms 计时 */
