#include "Motor.h"

static int16 encoder_lr, encoder_rr;  /* PIT 中断内采样，供诊断显示 */
volatile uint32 pit_tick = 0;         /* 5ms 硬件时基 */

void Motor_Init(void)
{
    pwm_init(PWM_1, 17000, 0);
    pwm_init(PWM_2, 17000, 0);
    gpio_init(DIR_1, GPO, 1, GPO_PUSH_PULL);
    gpio_init(DIR_2, GPO, 0, GPO_PUSH_PULL);
}

void encoder_init(void)
{
    encoder_dir_init(ENCODER_DIR_LR, ENCODER_DIR_PULSE_LR, ENCODER_DIR_DIR_LR);
    encoder_dir_init(ENCODER_DIR_RR, ENCODER_DIR_PULSE_RR, ENCODER_DIR_DIR_RR);
}

/** 通用电机控制：duty>=0 正转, duty<0 反转 */
static void motor_control(gpio_pin_enum dir_pin, pwm_channel_enum pwm_ch, int16 duty)
{
    if (duty >= 0)
    {
        gpio_set_level(dir_pin, 0);
        pwm_set_duty(pwm_ch, duty);
    }
    else
    {
        gpio_set_level(dir_pin, 1);
        pwm_set_duty(pwm_ch, (uint32)(-duty));
    }
}

void motor1_control(int16 motor_duty) { motor_control(DIR_1, PWM_1, motor_duty); }
void motor2_control(int16 motor_duty) { motor_control(DIR_2, PWM_2, motor_duty); }

int16 motor_get_encoder_lr(void) { return encoder_lr; }
int16 motor_get_encoder_rr(void) { return encoder_rr; }

/*
 * PIT 5ms 中断：硬件计时 + 编码器采样。
 * 已删除双电机速度闭环（改差速开环），编码器仅用于诊断显示。
 */
void pit_handler(void)
{
    pit_tick++;
    encoder_lr = encoder_get_count(ENCODER_DIR_LR);
    encoder_rr = encoder_get_count(ENCODER_DIR_RR);
    encoder_clear_count(ENCODER_DIR_LR);
    encoder_clear_count(ENCODER_DIR_RR);
}
