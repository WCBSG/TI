/* servo.c — 舵机摆杆控制：像素域 PID+前馈，5ms TIM0 中断驱动
 * error = cx - g_servo_target → control = KP*e + KI*Σe + KD*D低通 + 前馈 → duty = center ± control
 * 无球时保持上次位置（防球乱滚）；标定参数变量化（本车实测），PID 宏定死 */

#include "servo.h"
#include "protocol.h"   /* 读取 proto_ball_x / proto_ball_valid */

/* ── 标定参数（本车实测，菜单可调） ── */
int16 servo_center_duty = 4400;   /* 摆杆水平 */
int16 pixel_zero        = 175;    /* O 点像素 X */
int16 px_per_cm         = 11;     /* 每 cm 像素数 */
int16 ball_target_cm_x10 = 0;

/* 球目标像素 X（任务3/5/6 设置） */
int16 g_servo_target = 175;   /* 默认 O 点（pixel_zero 初始值） */

/* 控制使能：任务 3/5/6 + BallCal 页 = 1，其他禁用（中断里检查，防止回中后又被 PID 覆盖） */
uint8 servo_enable = 0;

/* 5ms 中断累加的毫秒计数器（Servo_Timer_Callback 每 5ms 加一次，任务3 稳定判定用） */
volatile uint32 g_servo_ms = 0;

/* PID 内部状态 */
static int16  last_error    = 0;
static float  error_sum     = 0.0f;
static float  filtered_diff = 0.0f;
static int16  last_target   = 175;
static uint16 last_duty     = 4500;
static int16  last_cx       = -1;     /* 上次球像素（速度前馈） */
static float  filtered_speed = 0.0f;  /* 球速低通（速度前馈） */

/* 当前生效 PID 参数（默认/任务3 可切换） */
static float  cur_kp = SERVO_KP;
static float  cur_ki = SERVO_KI;
static float  cur_kd = SERVO_KD;
static float  cur_kf = SERVO_KF;
static float  cur_kv = SERVO_KV;

void Servo_SetDefaultParams(void)
{
    cur_kp = SERVO_KP; cur_ki = SERVO_KI; cur_kd = SERVO_KD; cur_kf = SERVO_KF; cur_kv = SERVO_KV;
}

void Servo_SetTask3Params(void)
{
    cur_kp = SERVO_T3_KP; cur_ki = SERVO_T3_KI; cur_kd = SERVO_T3_KD; cur_kf = SERVO_T3_KF; cur_kv = SERVO_T3_KV;
}

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
/* 使能控制 + 开 UART3 接收（任务3/5/6 + BallCal 页调用；servo 需要球位置才接收）
 * ⚠️ 幂等：main 菜单循环每 10ms 调用本函数，已使能必须跳过 Protocol_Start，
 *    否则每 10ms 重启 DMA 接收（DMA_URXR_CR=0xA1 清 FIFO）→ 打断接收读不到球 */
void Servo_Enable(void)
{
    if (servo_enable) return;
    servo_enable = 1;
    Protocol_Start();
}

void Servo_Control_Init(void)   /* 回中 + PID 复位 + 禁用控制 + 关 UART3 接收（省算力） */
{
    servo_enable    = 0;
    g_servo_target  = pixel_zero;
    last_error      = 0;
    error_sum       = 0.0f;
    filtered_diff   = 0.0f;
    last_target     = pixel_zero;
    last_duty       = (uint16)servo_center_duty;
    last_cx         = -1;
    filtered_speed  = 0.0f;
    Servo_SetDefaultParams();   /* 恢复默认参数（任务3 特调结束后回归） */
    Servo_PWM_Set((uint16)servo_center_duty);
    Protocol_Stop();
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

    control = cur_kp * error
            + cur_ki * error_sum
            + cur_kd * filtered_diff;

    /* 速度前馈：球速（5ms 像素位移）低通 × Kv，球开始滚就反向推，抵消车运动带动球 */
    if (last_cx >= 0)
    {
        float speed = (float)(cx - last_cx);
        filtered_speed = (filtered_speed * 15.0f + speed) / 16.0f;
        control += cur_kv * filtered_speed;
    }
    last_cx = cx;

    /* 前馈：静摩擦补偿，误差大时额外推一把 */
    if (abs_error > SERVO_FF_DEADZONE)
    {
        control += (error > 0) ? cur_kf : -cur_kf;
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

    if (servo_enable && proto_ball_valid)
    {
        Servo_Control_Update(proto_ball_x);
    }
}

/* ── 初始化 5ms 周期中断（TIM0，与 TM1 pit_tick 并存） ── */
void Servo_Timer_Init(void)
{
    pit_ms_init(SERVO_PIT_CH, SERVO_PIT_MS, Servo_Timer_Callback);
}
