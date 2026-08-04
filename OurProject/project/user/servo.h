/* servo.h — 舵机摆杆控制：像素域 PID+前馈（真机验证），5ms TIM0 中断驱动
 * 控制律：error=cx-g_servo_target → control=KP*e+KI*Σe+KD*D低通+前馈 → duty=center±control
 * 标定参数变量化（本车实测 4500/175/11，菜单可调）；PID 参数宏定死（真机值） */

#ifndef __SERVO_H_
#define __SERVO_H_

#include "zf_common_headfile.h"

/* ════════════════════════════════════════════════════════════
 * 舵机 PWM 硬件参数
 * ════════════════════════════════════════════════════════════ */
#define SERVO_FREQ      300     /* PWM 频率 Hz（PWME 同组，与灯带同频） */
#define SERVO_MIN       3740    /* 舵机左极限（实测行程） */
#define SERVO_MAX       5740    /* 舵机右极限（实测行程） */
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
#define SERVO_KI        0.01f    /* 积分项：消除稳态误差（纯视觉追踪必须开启） */
#define SERVO_KD        600.0f
#define SERVO_KF        20.0f
#define SERVO_I_MAX     500.0f
#define SERVO_I_DEADZONE 50      /* 积分累积死区：|error|<该值才积分 */
#define SERVO_FF_DEADZONE 8      /* 前馈死区：|error|>该值才加前馈 */

/* ── 速度前馈（Zui1 借鉴，行驶中稳球关键） ──
 * 球速 = 5ms 像素位移（低通 α=1/16）× Kv → 球开始滚就反向推舵机，抵消车运动带动球
 * 换算：Zui1 用 20ms 位移×Kv100 ≈ 本工程 5ms 位移×Kv400
 * 任务 3 静止用 SERVO_T3_KV=0（不需要） */
#define SERVO_KV        400.0f

/* ── 任务 3 特调参数（与归中/任务5/6 分开） ──
 * 基于 E09（KD800 强刹车/KF5 弱前馈/KI0），真机过冲 1.5cm → KD 升到 1100 增强刹车
 * 真机对比调：超调→升 KD；不到位/慢→加 KI 破静摩擦或升 KF */
#define SERVO_T3_KP  10.0f
#define SERVO_T3_KI  0.0f
#define SERVO_T3_KD  1200.0f
#define SERVO_T3_KF  5.0f
#define SERVO_T3_KV  0.0f     /* 任务3 静止不用速度前馈 */

/* ── 任务 5 参数（行驶球稳 O，现场调宏，初值=默认） ── */
#define SERVO_T5_KP  10.0f
#define SERVO_T5_KI  0.01f
#define SERVO_T5_KD  600.0f
#define SERVO_T5_KF  20.0f
#define SERVO_T5_KV  400.0f

/* ── 任务 6 参数（行驶球稳指定位置，现场调宏，初值=默认） ── */
#define SERVO_T6_KP  10.0f
#define SERVO_T6_KI  0.01f
#define SERVO_T6_KD  600.0f
#define SERVO_T6_KF  20.0f
#define SERVO_T6_KV  400.0f

/* ════════════════════════════════════════════════════════════
 * 5ms 周期控制（PIT 中断驱动舵机追踪）
 * ════════════════════════════════════════════════════════════ */
#define SERVO_PIT_CH    TIM0_PIT    /* 独立 PIT 通道（TIM0），与 TM1(pit_tick 计时) 并存 */
#define SERVO_PIT_MS    5

/* 5ms 中断累加的毫秒计数器（供任务3 稳定判定计时） */
extern volatile uint32 g_servo_ms;

/* 球目标像素 X（运行时修改：任务3/5/6 设置） */
extern int16 g_servo_target;

/* 控制使能：任务 3/5/6 + BallCal 页 = 1，其他禁用（Servo_Control_Init 复位为 0） */
extern uint8 servo_enable;

/* ── 函数声明 ── */
void   Servo_Init(void);                       /* 舵机 PWM 初始化 */
void   Servo_PWM_Set(uint16 pwm_value);        /* 直接设置占空比（带钳位） */
void   Servo_Enable(void);                     /* 使能控制 + Protocol_Start（任务3/5/6 + BallCal 用） */
void   Servo_Control_Init(void);               /* 回中 + PID 复位 + 禁用 + Protocol_Stop */
void   Servo_SetDefaultParams(void);           /* 默认参数（归中） */
void   Servo_SetTask3Params(void);             /* 任务 3 特调参数 */
void   Servo_SetTask5Params(void);             /* 任务 5 特调参数 */
void   Servo_SetTask6Params(void);             /* 任务 6 特调参数 */
void   Servo_SetBallTargetCm(int16 cm_x10);    /* 球目标 cm(0.1)→像素（Launch 预览/任务6） */
uint16 Servo_Control_Update(int16 cx);         /* 球像素 X → PID+前馈 → 舵机，返回 duty */
void   Servo_Timer_Init(void);                 /* 初始化 5ms PIT，中断里执行 PID 追踪 */
void   Servo_Timer_Callback(void);             /* 5ms 中断回调（pit 自动调用） */

#endif
