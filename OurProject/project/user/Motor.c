#include "Motor.h"
#include "menu_defs.h"        /* page_motor_debug, Menu_IsTop */

int16 encoder_data_dir_LR, encoder_data_dir_RR;
volatile uint8 driving = 0;

int16 motor1_duty = 0;         /* 调试：电机1 手动占空比 */
int16 motor2_duty = 0;         /* 调试：电机2 手动占空比 */

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

void motor1_control(int16 motor_duty)
{
    if (motor_duty >= 0)
    {
        gpio_set_level(DIR_1, 0);
        pwm_set_duty(PWM_1, motor_duty);
    }
    else
    {
        gpio_set_level(DIR_1, 1);
        pwm_set_duty(PWM_1, -motor_duty);
    }
}

void motor2_control(int16 motor_duty)
{
    if (motor_duty >= 0)
    {
        gpio_set_level(DIR_2, 0);
        pwm_set_duty(PWM_2, motor_duty);
    }
    else
    {
        gpio_set_level(DIR_2, 1);
        pwm_set_duty(PWM_2, -motor_duty);
    }
}

void pit_handler(void)
{
    /* 编码器原始值：物理正转 → 计正值 */
    encoder_data_dir_LR = encoder_get_count(ENCODER_DIR_LR);
    encoder_data_dir_RR = encoder_get_count(ENCODER_DIR_RR);

    encoder_clear_count(ENCODER_DIR_LR);
    encoder_clear_count(ENCODER_DIR_RR);

    if (driving == 1)
    {
        /* motor1 反向安装（负 Target → 前进），Actual 需取负以匹配 Target 符号 */
        motor1_pid.Actual = -encoder_data_dir_LR;
        PID_Update(&motor1_pid);
        motor1_control(motor1_pid.Out);

        /* motor2 正常安装（正 Target → 前进） */
        motor2_pid.Actual = encoder_data_dir_RR;
        PID_Update(&motor2_pid);
        motor2_control(motor2_pid.Out);
    }
}

/** 调试页同步：在调试页时持续将 motor_duty 输出到电机，离开时自动停止 */
void Motor_DebugSync(void)
{
    static uint8 was_debug = 0;
    uint8 is_debug = Menu_IsTop(&page_motor_debug);

    if (is_debug)
    {
        motor1_control(motor1_duty);
        motor2_control(motor2_duty);
    }
    else if (was_debug)
    {
        /* 刚离开调试页 → 停止电机 */
        motor1_control(0);
        motor2_control(0);
    }
    was_debug = is_debug;
}
