/*********************************************************************************************************************
* 文件名称          servo.c
* 说明              舵机摆杆控制 — 像素域 PID + 前馈，5ms PIT 中断驱动
*
* 数据流（5ms 中断）：
*   proto_ball_x(像素) --PID+前馈--> 舵机 duty（目标 = g_servo_target 像素）
*
* 无球时（proto_ball_valid==0）保持上次舵机位置，不归零（防球乱滚）
*
* 合并适配（2026-08）：SERVO_CENTER/IMAGE_CENTER 宏 → 标定变量，
* 其余逻辑照搬舵机项目（真机验证）
********************************************************************************************************************/

#include "servo.h"
#include "protocol.h"   /* 读取 proto_ball_x / proto_ball_valid */

/* ── 标定参数（本车实测，菜单可调） ── */
int16 servo_center_duty = 4500;   /* 摆杆水平 */
int16 pixel_zero        = 175;    /* O 点像素 X */
int16 px_per_cm         = 11;     /* 每 cm 像素数 */
int16 ball_target_cm_x10 = 0;

/* 球目标像素 X（任务3/5/6 设置） */
int16 g_servo_target = 175;   /* 默认 O 点（pixel_zero 初始值） */

/* 5ms 中断累加的毫秒计数器（Servo_Timer_Callback 每 5ms 加一次，任务3 稳定判定用） */
volatile uint32 g_servo_ms = 0;

/* PID 内部状态 */
static int16  last_error    = 0;
static float  error_sum     = 0.0f;
static float  filtered_diff = 0.0f;
static int16  last_target   = 175;
static uint16 last_duty     = 4500;

/* ── 舵机 PWM 初始化 ── */
void Servo_Init(void)
{
    pwm_init(PWME_CH1P_PA0, SERVO_FREQ, (uint32)servo_center_duty);
}

/* ── 直接设置占空比（带钳位） ── */
void Servo_PWM_Set(uint16 pwm_value)
{
    if (pwm_value > SERVO_MAX)
    {
        pwm_value = SERVO_MAX;
    }
    else if (pwm_value < SERVO_MIN)
    {
        pwm_value = SERVO_MIN;
    }
    pwm_set_duty(PWME_CH1P_PA0, pwm_value);
}

/* ── PID 控制状态复位 + 舵机回中 ── */
void Servo_Control_Init(void)
{
    g_servo_target = pixel_zero;
    last_error      = 0;
    error_sum       = 0.0f;
    filtered_diff   = 0.0f;
    last_target     = pixel_zero;
    last_duty       = (uint16)servo_center_duty;
    Servo_PWM_Set((uint16)servo_center_duty);
}

/* ── 小球追踪 PID + 前馈（移植自 E09 servo_control，真机验证）
 * cx: OpenART 小球像素 X 坐标 (0~319)
 * 返回实际写入的舵机占空比 */
uint16 Servo_Control_Update(int16 cx)
{
    int16 target = g_servo_target;
    int16 error;
    int16 abs_error;
    int16 servo_duty;
    float error_diff;
    float control;

    /* 目标切换时清空历史/滤波状态，避免旧数据污染新目标 */
    if (target != last_target)
    {
        filtered_diff = 0.0f;
        error_sum     = 0.0f;
    }

    error     = (int16)(cx - target);
    abs_error = (error < 0) ? (int16)(-error) : error;
    error_diff = (float)(error - last_error);

    /* D 项低通滤波：去噪不减刹车力 */
    filtered_diff = (filtered_diff * 15.0f + error_diff) / 16.0f;

    /* 积分：误差小于死区才累积（突破静摩擦），超限幅清零 */
    if (abs_error < SERVO_I_DEADZONE)
    {
        error_sum += error;
        if (error_sum >  SERVO_I_MAX) error_sum =  SERVO_I_MAX;
        if (error_sum < -SERVO_I_MAX) error_sum = -SERVO_I_MAX;
    }
    else
    {
        error_sum = 0.0f;
    }

    control = SERVO_KP * error
            + SERVO_KI * error_sum
            + SERVO_KD * filtered_diff;

    /* 前馈：静摩擦补偿，误差大时额外推一把 */
    if (abs_error > SERVO_FF_DEADZONE)
    {
        control += (error > 0) ? SERVO_KF : -SERVO_KF;
    }

#if SERVO_INVERT
    control = -control;
#endif

    servo_duty = (int16)(servo_center_duty + control);
    if (servo_duty < SERVO_MIN) servo_duty = SERVO_MIN;
    if (servo_duty > SERVO_MAX) servo_duty = SERVO_MAX;

    Servo_PWM_Set((uint16)servo_duty);

    last_error  = error;
    last_target = target;
    last_duty   = (uint16)servo_duty;

    return (uint16)servo_duty;
}

/* ── 5ms PIT 中断回调：舵机追踪控制 ──
 * 由 pit_ms_init 注册（TIM0），每 5ms 读一次最新球位置做 PID；无球时保持上次位置 */
void Servo_Timer_Callback(void)
{
    g_servo_ms += SERVO_PIT_MS;     /* 5ms 计时 */

    if (proto_ball_valid)
    {
        Servo_Control_Update(proto_ball_x);
    }
}

/* ── 初始化 5ms 周期中断（TIM0，与 TM1 pit_tick 并存） ── */
void Servo_Timer_Init(void)
{
    pit_ms_init(SERVO_PIT_CH, SERVO_PIT_MS, Servo_Timer_Callback);
}
