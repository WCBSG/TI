#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "zf_common_headfile.h"
#include "PID.h"

extern int16 encoder_data_dir_1, encoder_data_dir_2;
extern uint8 driving;


#define PIT_ENCODER             (TIM1_PIT)  // 编码器 PIT 定时器 用于更新 PID

#define DIR_1               ( IO_P74 )//暂定左轮方向
#define PWM_1               ( PWMB_CH2_P75 )//暂定左轮PWM
                              
#define DIR_2               ( IO_P76 )//暂定右轮方向
#define PWM_2               ( PWMB_CH4_P77 )//暂定右轮PWM


#define ENCODER_DIR_1                 	(PWMA_ENCODER)              
#define ENCODER_DIR_PULSE_1            	(PWMA_ENCODER_CH1P_P60)     
#define ENCODER_DIR_DIR_1              	(PWMA_ENCODER_CH2P_P62)     

#define ENCODER_DIR_2                 	(PWMC_ENCODER)              
#define ENCODER_DIR_PULSE_2       		(PWMC_ENCODER_CH1P_P40)     
#define ENCODER_DIR_DIR_2           	(PWMC_ENCODER_CH2P_P42)     

void Motor_Init(void);
void encoder_init(void);
void motor1_control(int16 motor_duty);
void motor2_control(int16 motor_duty);
void pit_handler(void);

#endif