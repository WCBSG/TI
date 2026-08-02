/*********************************************************************************************************************
* 文件名称          cmd_ctrl.c
* 说明              USB-CDC 命令触发实现 — 上位机发命令启动测试
*
* 接收链路：
*   usb_cdc_rx_interrupt(cmd_rx_irq) 注册回调
*   → USB 中断（usb_out_ep1）收到 OUT 包 → cmd_rx_irq(buff, len)
*   → 只存字节凑行（cmd_ready=1），不在 ISR 做事
*   → 主循环 cmd_poll() 解析执行
*
* 命令（大小写不敏感）：T2/T3/T5/T6 启动任务，IR 光电测试，STOP 停 IR，HELP 帮助
********************************************************************************************************************/

#include "cmd_ctrl.h"
#include "task_sched.h"    /* task_sched_set, TASK_* */
#include "menu_defs.h"     /* launch_triggered, base_speed */
#include "KEY.h"           /* key 标志（STOP 复用按键逻辑可留空） */
#include "Motor.h"         /* SPIN 转向测试：motor1/motor2_control */
#include "mileage.h"       /* CALM 标定：pulses_per_cm */
#include "config.h"        /* CALM 标定后保存 */
#include "imu_ctrl.h"      /* CALM 陀螺仪直线：imu_yaw_x100, imu_ctrl_tick, imu_ctrl_reset_yaw */

/* ── 命令缓冲（ISR 写入，主循环读取） ── */
#define CMD_BUF_MAX   16    /* 够 "STOP"/"T2"，超长截断 */

static char  cmd_buf[CMD_BUF_MAX];
static uint8 cmd_idx;
static volatile uint8 cmd_ready;

volatile uint8 ir_test_cmd = 0;

/* ── 大小写不敏感比较（C251 C89 无 stricmp，手写；A-Z 转小写） ── */
static uint8 cmd_eq(const char *a, const char *b)
{
    uint8 i;
    for (i = 0; a[i] && b[i]; i++)
    {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return (a[i] == '\0' && b[i] == '\0');
}

/* ── 前缀匹配：a 以 b 开头（大小写不敏感），如 "SETCM 3" 匹配 "SETCM" ── */
static uint8 cmd_prefix_eq(const char *a, const char *b)
{
    uint8 i;
    for (i = 0; b[i]; i++)
    {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca += 'a' - 'A';
        if (cb >= 'A' && cb <= 'Z') cb += 'a' - 'A';
        if (ca != cb) return 0;
    }
    return 1;
}

/* ── USB 接收回调（ISR 上下文！只存字节，不做事） ── */
static void cmd_rx_irq(uint8 *buff, uint16 len)
{
    uint16 i;
    for (i = 0; i < len; i++)
    {
        if (buff[i] == '\n' || buff[i] == '\r')
        {
            if (cmd_idx > 0)
            {
                cmd_buf[cmd_idx] = '\0';
                cmd_ready = 1;
            }
            cmd_idx = 0;
        }
        else if (cmd_idx < CMD_BUF_MAX - 1)
        {
            cmd_buf[cmd_idx++] = buff[i];
        }
    }
}

/* ── 初始化：注册接收回调 ── */
void cmd_ctrl_init(void)
{
    cmd_idx = 0;
    cmd_ready = 0;
    ir_test_cmd = 0;
    usb_cdc_rx_interrupt(cmd_rx_irq);
}

/* ── 原地转向测试：SPIN[ L|R]，2 秒固定差速（验证电机左右是否标反）
 *   SPIN  或 SPIN L → 左轮正 duty / 右轮停（预期左转）
 *   SPIN R         → 左轮停 / 右轮正 duty（预期右转）
 *   若实测方向与预期相反 → 电机左右标反，需交换 motor1/motor2 输出 */
static void cmd_spin(uint8 dir)
{
    uint32 t0;
    EA = 0; t0 = pit_tick; EA = 1;

    if (dir == 'R')       /* 右轮转，左轮停 */
    {
        motor1_control(0);
        motor2_control(-2000);
        usb_cdc_write_string("[SPIN R] 右轮转左轮停 2s\n");
    }
    else                  /* 左轮转，右轮停 */
    {
        motor1_control(2000);
        motor2_control(0);
        usb_cdc_write_string("[SPIN L] 左轮转右轮停 2s\n");
    }

    while (1)
    {
        EA = 0; if ((pit_tick - t0) * 5 > 2000) { EA = 1; break; } EA = 1;
        system_delay_ms(10);
    }
    motor1_control(0);
    motor2_control(0);
}

/* ── 里程标定：陀螺仪保持直线，跑到目标右轮脉冲停。
 *   车放赛道直线段，固定走 CALM_TARGET 脉冲（约等距离），
 *   上位机量实际 cm 后用 SETCM <p/cm> 写入。
 *
 * 陀螺仪直线：yaw 偏差 → 差速微调。
 *   yaw>0(偏右) → 需右转回正 → 左快右慢 → 左轮 spd、右轮 spd-diff。
 *   比例 KGYRO_CAL 小，防振荡（标定只要方向大致直，不需精确闭环）。 */
#define CALM_TARGET      500      /* 目标右轮脉冲 */
#define CALM_YAW_K       3        /* yaw 0.1° → 差速 duty（小值防振荡） */

static void cmd_calibrate(void)
{
    int16 spd = (base_speed > 0) ? base_speed : 2000;
    int32 err_yaw_x10, diff, ld, rd;

    motor_reset_mileage();
    imu_ctrl_reset_yaw();            /* 当前朝向=目标，保持直线 */
    system_delay_ms(50);             /* 让 yaw 稳定 */

    while (1)
    {
        /* 到目标脉冲停 */
        EA = 0; if (encoder_total_rr >= CALM_TARGET) { EA = 1; break; } EA = 1;

        imu_ctrl_tick(10);
        err_yaw_x10 = imu_yaw_x100 / 10;       /* 0.1° */

        /* 直线差速：右偏(yaw>0)→右转回正→左快右慢 */
        diff = (int32)CALM_YAW_K * err_yaw_x10;
        ld = spd + diff;                        /* 左轮 */
        rd = spd - diff;                        /* 右轮 */
        if (ld > 4000) ld = 4000;
        if (rd < 0)    rd = 0;

        motor1_control((int16)ld);             /* 左轮正 duty 前进 */
        motor2_control(-(int16)rd);            /* 右轮负 duty 前进 */
        system_delay_ms(10);
    }
    motor1_control(0);
    motor2_control(0);

    {
        int32 pulses;
        char buf[64];
        uint32 n = 0;
        EA = 0; pulses = encoder_total_rr; EA = 1;
        if (pulses < 0) pulses = -pulses;
        n += zf_sprintf((int8 *)(buf + n),
                        "[CALM] pulses=%d, 量实际cm后发 SETCM <p/cm>\n", (int32)pulses);
        usb_cdc_write_buffer((const uint8 *)buf, (uint16)n);
    }
}

/* ── 里程标定第 2 步：SETCM <p/cm> 手动写入每 cm 脉冲数并保存。
 *   命令格式: SETCM 3   （p/cm = 脉冲 / 实际cm） */
static void cmd_setcm(void)
{
    int32 val = 0;
    uint8 i = 5;                     /* 跳过 "SETCM" 前缀 */
    while (cmd_buf[i] == ' ') i++;   /* 跳过空格 */
    while (cmd_buf[i] >= '0' && cmd_buf[i] <= '9')
    {
        val = val * 10 + (cmd_buf[i] - '0');
        i++;
    }
    if (val <= 0) val = 1;
    pulses_per_cm_rr = (int16)val;
    pulses_per_cm_lr = (int16)val;   /* 先假设左右一致 */
    config_save();
    {
        char buf[48];
        uint32 n = 0;
        n += zf_sprintf((int8 *)(buf + n), "[SETCM] p/cm=%d saved\n", (int32)val);
        usb_cdc_write_buffer((const uint8 *)buf, (uint16)n);
    }
}

/* ── 主循环调用：解析收到的命令并执行 ── */
void cmd_poll(void)
{
    if (!cmd_ready) return;
    cmd_ready = 0;

    if (cmd_eq(cmd_buf, "T2"))      { task_sched_set(TASK_2);  launch_triggered = 1; }
    else if (cmd_eq(cmd_buf, "T3")) { task_sched_set(TASK_3);  launch_triggered = 1; }
    else if (cmd_eq(cmd_buf, "T5")) { task_sched_set(TASK_5);  launch_triggered = 1; }
    else if (cmd_eq(cmd_buf, "T6")) { task_sched_set(TASK_6);  launch_triggered = 1; }
    else if (cmd_eq(cmd_buf, "IR")) { ir_test_cmd = 1; }
    else if (cmd_eq(cmd_buf, "STOP")) { ir_test_cmd = 0; }      /* 停止光电测试 */
    else if (cmd_eq(cmd_buf, "SPIN")) { cmd_spin('L'); }
    else if (cmd_eq(cmd_buf, "SPIN L")) { cmd_spin('L'); }
    else if (cmd_eq(cmd_buf, "SPIN R")) { cmd_spin('R'); }
    else if (cmd_eq(cmd_buf, "CALM")) { cmd_calibrate(); }
    else if (cmd_prefix_eq(cmd_buf, "SETCM")) { cmd_setcm(); }
    else if (cmd_eq(cmd_buf, "HELP"))
    {
        usb_cdc_write_string("T2/T3/T5/T6/IR/SPIN[L/R]/CALM/SETCM/STOP/HELP\n");
    }
}
