/*********************************************************************************************************************
* 文件名称          servo.h
* 说明              舵机摆杆控制 — 像素域 PID + 前馈（真机验证），5ms PIT 中断驱动
*
* 来源：舵机项目 servo.c（移植自 E09 servo_control），真机验证过
* 控制律（像素域，直接 cx 像素）：
*   error = cx - g_servo_target
*   control = KP*e + KI*Σe + KD*D低通 + 前馈(破静摩擦)
*   duty = servo_center_duty ± control
*
* 合并适配（2026-08）：SERVO_CENTER/IMAGE_CENTER 宏 → 标定变量
*   servo_center_duty（本车实测 4500）/ pixel_zero（本车实测 175）
*   PID 参数保留舵机项目真机值（宏定死）
********************************************************************************************************************/

#ifndef __SERVO_H_
#define __SERVO_H_

#include "zf_common_headfile.h"

/* ════════════════════════════════════════════════════════════
 * 舵机 PWM 硬件参数
 * ════════════════════════════════════════════════════════════ */
#define SERVO_FREQ      300     /* PWM 频率 Hz（PWME 同组，与灯带同频） */
#define SERVO_MIN       3500    /* 舵机左极限（实测行程） */
#define SERVO_MAX       5500    /* 舵机右极限（实测行程） */
#define SERVO_INVERT    1       /* 舵机方向反了改为 0/1（合并后现场确认！） */

/* ════════════════════════════════════════════════════════════
 * 标定参数（变量，菜单可调）
 * ════════════════════════════════════════════════════════════ */
extern int16 servo_center_duty;  /* 舵机中位 duty（摆杆水平，本车实测 4500） */
extern int16 pixel_zero;         /* O 点像素 X（本车实测 175） */
extern int16 px_per_cm;          /* 每 cm 像素数（本车实测 11，任务6 目标换算） */
extern int16 ball_target_cm_x10; /* 任务6 球目标位置（0.1cm，相对 O） */

/* ════════════════════════════════════════════════════════════
 * PID + 前馈参数（真机验证，宏定死；改参数需重编译）
 * KP: 主比例 | KI: 积分(仅 |err|<50 累积, 破静摩擦) | KD: 微分刹车(低通)
 * KF: 前馈(误差>8 推第一把) | I_MAX: 积分限幅
 * ════════════════════════════════════════════════════════════ */
#define SERVO_KP        10.0f
#define SERVO_KI        0.1f    /* 积分项：消除稳态误差（纯视觉追踪必须开启） */
#define SERVO_KD        600.0f
#define SERVO_KF        20.0f
#define SERVO_I_MAX     500.0f
#define SERVO_I_DEADZONE 50      /* 积分累积死区：|error|<该值才积分 */
#define SERVO_FF_DEADZONE 8      /* 前馈死区：|error|>该值才加前馈 */

/* ════════════════════════════════════════════════════════════
 * 5ms 周期控制（PIT 中断驱动舵机追踪）
 * ════════════════════════════════════════════════════════════ */
#define SERVO_PIT_CH    TIM0_PIT    /* 独立 PIT 通道（TIM0），与 TM1(pit_tick 计时) 并存 */
#define SERVO_PIT_MS    5

/* 5ms 中断累加的毫秒计数器（供任务3 稳定判定计时） */
extern volatile uint32 g_servo_ms;

/* 球目标像素 X（运行时修改：任务3/5/6 设置） */
extern int16 g_servo_target;

/* ── 函数声明 ── */
void   Servo_Init(void);                       /* 舵机 PWM 初始化 */
void   Servo_PWM_Set(uint16 pwm_value);        /* 直接设置占空比（带钳位） */
void   Servo_Control_Init(void);               /* PID 状态复位 + 回中 */
uint16 Servo_Control_Update(int16 cx);         /* 球像素 X → PID+前馈 → 舵机，返回 duty */
void   Servo_Timer_Init(void);                 /* 初始化 5ms PIT，中断里执行 PID 追踪 */
void   Servo_Timer_Callback(void);             /* 5ms 中断回调（pit 自动调用） */

#endif
