#ifndef __MOTOR_H__
#define __MOTOR_H__

#include "zf_common_headfile.h"


#define PIT_ENCODER             (TIM1_PIT)  // 定时器 PIT 通道，5ms 硬件时基（计时 + 编码器采样）

#define PWM_1               ( PWMB_CH4_P77 )//左后轮速度
#define DIR_1               ( PWMB_CH3_P76 )//左后轮转向

#define PWM_2               ( PWMB_CH2_P75 )//右后轮速度
#define DIR_2               ( PWMB_CH1_P74 )//右后轮转向


#define ENCODER_DIR_LR                  (PWMA_ENCODER)              // 左轮编码器
#define ENCODER_DIR_PULSE_LR            (PWMA_ENCODER_CH1P_P60)     // PULSE 引脚
#define ENCODER_DIR_DIR_LR              (PWMA_ENCODER_CH2P_P62)     // DIR 引脚

#define ENCODER_DIR_RR                  (PWMC_ENCODER)              // 右轮编码器
#define ENCODER_DIR_PULSE_RR            (PWMC_ENCODER_CH1P_P40)     // PULSE 引脚
#define ENCODER_DIR_DIR_RR              (PWMC_ENCODER_CH2P_P42)     // DIR 引脚

extern volatile uint32 pit_tick;     /* 5ms 硬件时基计数（PIT 中断递增） */

/* ── 里程累计（PIT 中断内累加，正负号代表方向） ── */
extern volatile int32 encoder_total_lr;   /* 左轮累计编码器脉冲 */
extern volatile int32 encoder_total_rr;   /* 右轮累计编码器脉冲 */

void Motor_Init(void);
void encoder_init(void);
void motor1_control(int16 motor_duty);
void motor2_control(int16 motor_duty);
int16 motor_get_encoder_lr(void);    /* 诊断：左轮 5ms 编码器计数 */
int16 motor_get_encoder_rr(void);    /* 诊断：右轮 5ms 编码器计数 */
void motor_reset_mileage(void);      /* 清零左右轮里程累计（阶段切换用） */
void pit_handler(void);

#endif
