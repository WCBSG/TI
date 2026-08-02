/*********************************************************************************************************************
* 文件名称          line_ctrl.c
* 说明              巡线环实现 — 红外偏差 → 差速 PWM（开环）
*
* 差速开环：唯一 PID 输出作用于"传感器误差 → 左右轮 PWM duty 差"。
* base_speed 是基准 duty（0-10000 满占空比），steer_pid.Out 是差速 duty 量。
* 弯道自适应减速内置于此模块（|error| 分档），任务层无需关心。
********************************************************************************************************************/

#include "line_ctrl.h"
#include "PID.h"
#include "Motor.h"
#include "config.h"
#include "imu_ctrl.h"    /* imu_ok, imu_gyro_dps_x10（陀螺仪阻尼） */

PID_t steer_pid;
int16 line_duty_lr = 0;   /* 诊断：最近一次左轮输出 duty */
int16 line_duty_rr = 0;   /* 诊断：最近一次右轮输出 duty */
int16 kd_yaw = 0;            /* 陀螺仪阻尼系数（默认 0=不生效） */

/* 差速渐变率：每 10ms 输出最多变化量（小=更平滑，大=响应快）
 * 200 → 满差速约 100ms 到位；调小更顺但弯道响应慢 */
#define STEER_RAMP   200
static int32 steer_last_out = 0;   /* 上次 eff_out（渐变用） */

void line_ctrl_init(void)
{
    steer_pid.Kp     = 180;   /* 弯道转弯角速度需要(实测>20°/s 才过弯)，rate limit 削弱已回退 */
    steer_pid.Ki     = 0;
    steer_pid.Kd     = 50;    /* 用 Kd 抑制差速跳变卡顿（替代 rate limit，不砍弯道响应） */
    steer_pid.OutMax = 2000;  /* 差速上限 66%，给足转向力度 */
    steer_pid.OutMin = -2000;
    steer_last_out   = 0;

    if (config_valid())
    {
        steer_pid.Kp     = flash_buff[0];
        steer_pid.Ki     = flash_buff[1];
        steer_pid.Kd     = flash_buff[2];
        steer_pid.OutMax = flash_buff[3];
        steer_pid.OutMin = flash_buff[4];
        kd_yaw           = flash_buff[21];
    }
}

void line_ctrl_set(int error, int base_speed)
{
    int16 left_duty, right_duty, spd;
    int32 eff_out;
    int   abs_err = (error < 0) ? -error : error;

    /* 弯道自适应减速：|error|>6→85%, >2→90%, else 100%（实测 70% 过弯太慢，减轻） */
    if      (abs_err <= 2) spd = (int16)base_speed;
    else if (abs_err <= 6) spd = (int16)((int32)base_speed * 90 / 100);
    else                   spd = (int16)((int32)base_speed * 85 / 100);

    /* 位置式差速 PID：Target 恒 0，Actual = error（方向实测确认）
     * 差速规则：左慢右快=左转。线偏左(error<0)需左转 → 需 right 快 left 慢 → eff_out<0
     *   → Error0<0 → Actual>0 → Actual = error ✓（-error 实测会反，勿改）
     * 振荡则降 Kp / 加 Kd。 */
    steer_pid.Actual = error;
    PID_Update(&steer_pid);

    /* 陀螺仪阻尼：Out -= kd_yaw * 角速度(°/s)
     * 作用：抑制出弯振荡；代价：同时削弱正常转弯（弯道上 gyro 恒大），故 KdYaw 需现场调小 */
    eff_out = steer_pid.Out;
    if (imu_ok && kd_yaw != 0)
    {
        eff_out -= (int32)kd_yaw * (int32)imu_gyro_dps_x10 / 10;
        if (eff_out > 8000)  eff_out = 8000;   /* 防 int16 溢出（差速钳位后续收敛） */
        if (eff_out < -8000) eff_out = -8000;
    }

    /* 差速缓慢渐变：每 10ms 最多变化 STEER_RAMP，消除 PID 瞬间大幅跳变
     * （PID 对 err 突变一次性输出大差速 → 转向猛；渐变后转向渐进出） */
    if (eff_out - steer_last_out >  STEER_RAMP) eff_out = steer_last_out + STEER_RAMP;
    if (eff_out - steer_last_out < -STEER_RAMP) eff_out = steer_last_out - STEER_RAMP;
    steer_last_out = eff_out;

    /* 单轮差速：左轮固定 spd，右轮随 eff_out 变化
     *   方向验证：eff_out>0 → right 增 → 右轮快 → 左慢右快 → 左转 ✓（与双轮一致）
     *   eff_out<0 → right 减 → 右轮慢 → 左快右慢 → 右转 ✓
     *   直线更顺：eff_out 小幅波动时只有右轮响应，左轮恒定 */
    left_duty  = (int16)spd;
    right_duty = (int16)(spd + eff_out);

    /* 钳位：右轮上限 spd*2（允许外轮加速转向），下限 spd/3 */
    if (right_duty > (int32)spd * 2) right_duty = (int16)((int32)spd * 2);
    if (right_duty < spd / 3) right_duty = (int16)(spd / 3);

    /* 记录实际输出 duty（诊断显示）。
     * ⚠️ 电机左右（SPIN 实测重新推导）：
     *   motor1_control(2000) 车右转 → motor1=左轮（左快右慢=右转）
     *   motor2_control(-2000) 车左转 → motor2=右轮
     * → 左右未标反！motor1=左轮、motor2=右轮。
     * 分配：左轮(motor1) 给 left_duty、右轮(motor2) 给 -right_duty。
     * 符号：motor1 正 duty 前进、motor2 负 duty 前进（已实测）。
     * 差速验证：线偏右(eff_out<0)→left_duty大→左轮快/right_duty小→右轮慢→左快右慢=右转追线 ✓ */
    line_duty_lr = left_duty;       /* 诊断：实际左轮 = motor1 收到值 */
    line_duty_rr = -right_duty;     /* 诊断：实际右轮 = motor2 收到值 */
    motor1_control(left_duty);      /* 实际左轮 */
    motor2_control(-right_duty);    /* 实际右轮 */
}
