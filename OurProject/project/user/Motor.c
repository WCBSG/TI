#include "Motor.h"

static int16 encoder_lr, encoder_rr;  /* PIT 中断内使用，外部不可见 */
volatile uint8 driving = 0;

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

void pit_handler(void)
{
    encoder_lr = encoder_get_count(ENCODER_DIR_LR);
    encoder_rr = encoder_get_count(ENCODER_DIR_RR);
    encoder_clear_count(ENCODER_DIR_LR);
    encoder_clear_count(ENCODER_DIR_RR);

    if (driving == 1)
    {
        /* motor1 反向安装（负 Target → 前进），Actual 取负匹配 Target 符号 */
        motor1_pid.Actual = -encoder_lr;
        PID_Update(&motor1_pid);
        motor1_control(motor1_pid.Out);

        /* motor2 正常安装（正 Target → 前进） */
        motor2_pid.Actual = encoder_rr;
        PID_Update(&motor2_pid);
        motor2_control(motor2_pid.Out);
    }
}