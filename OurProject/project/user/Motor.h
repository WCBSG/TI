#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "zf_common_headfile.h"


#define PIT_ENCODER             (TIM1_PIT)  // 定时器 PIT 通道，5ms 硬件时基（计时 + 编码器采样）

#define PWM_1               ( PWMB_CH4_P77 )//左后轮速度
#define DIR_1               ( PWMB_CH3_P76 )//左后轮转向

#define PWM_2               ( PWMB_CH2_P75 )//右后轮速度
#define DIR_2               ( PWMB_CH1_P74 )//右后轮转向


extern volatile uint32 pit_tick;     /* 5ms 硬件时基计数（PIT 中断递增） */

void Motor_Init(void);
void motor1_control(int16 motor_duty);
void motor2_control(int16 motor_duty);
void pit_handler(void);

#endif
