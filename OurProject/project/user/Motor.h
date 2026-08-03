/* Motor.h — 电机驱动 + 5ms 时基
 * 电机1 左后轮：DIR=P76 PWM=P77（正 duty 前进）| 电机2 右后轮：DIR=P74 PWM=P75（负 duty 前进）
 * ⚠️ 符号：motor1_control(正) / motor2_control(负) 才是前进，勿单独改一侧 */
#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "zf_common_headfile.h"

#define PIT_ENCODER  (TIM1_PIT)      /* 5ms 硬件时基（计时） */

#define PWM_1  (PWMB_CH4_P77)        /* 左后轮速度 */
#define DIR_1  (PWMB_CH3_P76)        /* 左后轮转向 */
#define PWM_2  (PWMB_CH2_P75)        /* 右后轮速度 */
#define DIR_2  (PWMB_CH1_P74)        /* 右后轮转向 */

extern volatile uint32 pit_tick;     /* 5ms 时基计数（PIT 中断递增） */

void Motor_Init(void);
void motor1_control(int16 motor_duty);
void motor2_control(int16 motor_duty);
void pit_handler(void);

#endif
