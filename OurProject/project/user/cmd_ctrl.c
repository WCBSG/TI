/*********************************************************************************************************************
* 文件名称          cmd_ctrl.c
* 说明              USB-CDC 命令触发实现 — 上位机发命令现场调参
*
* 接收链路：
*   usb_cdc_rx_interrupt(cmd_rx_irq) 注册回调
*   → USB 中断（usb_out_ep1）收到 OUT 包 → cmd_rx_irq(buff, len)
*   → 只存字节凑行（cmd_ready=1），不在 ISR 做事
*   → 主循环 cmd_poll() 解析执行
*
* 命令（大小写不敏感）：SV 舵机测试 / BP 球稳参数调整 / HELP 帮助
* 比赛清理（2026-08）：删 T2/T3/T5/T6 任务启动（只靠菜单按键）+ IR/PROTO/UTEST/
* HOLD/SPIN/FAST/DBG/STOP 调试测试命令；保留现场调参命令 SV/BP。
********************************************************************************************************************/

#include "cmd_ctrl.h"
#include "ball_ctrl.h"     /* SV 舵机测试：SERVO_PWM / SERVO_DUTY_MIN/MAX/CENTER */

/* ── 命令缓冲（ISR 写入，主循环读取） ── */
#define CMD_BUF_MAX   16    /* 够 "BP PCM 11"，超长截断 */

static char  cmd_buf[CMD_BUF_MAX];
static uint8 cmd_idx;
static volatile uint8 cmd_ready;

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

/* ── 前缀匹配：a 以 b 开头（大小写不敏感），如 "SV 4500" 匹配 "SV" ── */
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
    usb_cdc_rx_interrupt(cmd_rx_irq);
}

/* ── 舵机测试：SV [<duty>] — 摆到指定位置（找中位/测行程）
 *   SV         → 中位 4500
 *   SV 1500    → 一端 / SV 7500 → 另一端
 *   摆杆水平时的 duty 即 servo_center_duty（标定参数） */
static void cmd_servo(void)
{
    int32 val = 0;
    uint8 i = 2;
    char buf[48];
    uint32 n = 0;

    while (cmd_buf[i] == ' ') i++;
    while (cmd_buf[i] >= '0' && cmd_buf[i] <= '9')
    {
        val = val * 10 + (cmd_buf[i] - '0');
        i++;
    }
    if (val == 0 && cmd_buf[i] == '\0') val = SERVO_DUTY_CENTER;  /* 裸 SV → 中位 */
    if (val < SERVO_DUTY_MIN) val = SERVO_DUTY_MIN;
    if (val > SERVO_DUTY_MAX) val = SERVO_DUTY_MAX;

    pwm_set_duty(SERVO_PWM, (uint32)val);
    n += zf_sprintf((int8 *)(buf + n), "[SV] duty=%d\n", (int32)val);
    usb_cdc_write_buffer((const uint8 *)buf, (uint16)n);
}

/* ── 球稳参数调整：BP <PARAM> <VALUE>（现场调参免烧录；菜单 Ball PID 页同步可调）
 *   BP KP 5    球 PID Kp
 *   BP KI 1    球 PID Ki
 *   BP KD 30   球 PID Kd
 *   BP SC 4500 舵机中位 duty（摆杆水平）
 *   BP PZ 175  O 点像素 X
 *   BP PCM 11  每 cm 像素数
 *   BP TGT 50  球目标位置（0.1cm，任务 6 用） */
static void cmd_ball_param(void)
{
    int32 val = 0;
    int32 min_val = 0, max_val = 0;
    uint8 neg = 0, known = 1;
    uint8 i = 2, j = 0;
    char param[8];
    char buf[64];
    uint32 n = 0;

    while (cmd_buf[i] == ' ') i++;
    while (cmd_buf[i] >= 'A' && cmd_buf[i] <= 'Z' && j < 7) param[j++] = cmd_buf[i++];
    param[j] = '\0';
    while (cmd_buf[i] == ' ') i++;
    if (cmd_buf[i] == '-') { neg = 1; i++; }
    while (cmd_buf[i] >= '0' && cmd_buf[i] <= '9')
    {
        val = val * 10 + (cmd_buf[i] - '0');
        i++;
    }
    if (neg) val = -val;

    if      (cmd_eq(param, "KP"))  { min_val = 0; max_val = 200; }
    else if (cmd_eq(param, "KI"))  { min_val = 0; max_val = 200; }
    else if (cmd_eq(param, "KD"))  { min_val = 0; max_val = 200; }
    else if (cmd_eq(param, "SC"))  { min_val = SERVO_DUTY_MIN; max_val = SERVO_DUTY_MAX; }
    else if (cmd_eq(param, "PZ"))  { min_val = 0; max_val = 319; }
    else if (cmd_eq(param, "PCM")) { min_val = 1; max_val = 200; }
    else if (cmd_eq(param, "TGT")) { min_val = -50; max_val = 50; }
    else known = 0;

    if (!known)
    {
        usb_cdc_write_string("[BP] KP/KI/KD/SC/PZ/PCM/TGT <val>\n");
        return;
    }
    if (val < min_val || val > max_val)
    {
        n += zf_sprintf((int8 *)(buf + n), "[BP] range %d..%d\n", min_val, max_val);
        usb_cdc_write_buffer((const uint8 *)buf, (uint16)n);
        return;
    }

    if      (cmd_eq(param, "KP"))  { ball_pid.Kp = (int16)val; }
    else if (cmd_eq(param, "KI"))  { ball_pid.Ki = (int16)val; }
    else if (cmd_eq(param, "KD"))  { ball_pid.Kd = (int16)val; }
    else if (cmd_eq(param, "SC"))  { servo_center_duty = (int16)val; }
    else if (cmd_eq(param, "PZ"))  { pixel_zero = (int16)val; }
    else if (cmd_eq(param, "PCM")) { px_per_cm = (int16)val; }
    else if (cmd_eq(param, "TGT")) { ball_target_cm_x10 = (int16)val; }

    n += zf_sprintf((int8 *)(buf + n), "[BP] %s=%d\n", param, (int32)val);
    usb_cdc_write_buffer((const uint8 *)buf, (uint16)n);
}

/* ── 主循环调用：解析收到的命令并执行 ── */
void cmd_poll(void)
{
    if (!cmd_ready) return;
    cmd_ready = 0;

    if (cmd_prefix_eq(cmd_buf, "SV")) { cmd_servo(); }
    else if (cmd_prefix_eq(cmd_buf, "BP")) { cmd_ball_param(); }
    else if (cmd_eq(cmd_buf, "HELP"))
    {
        usb_cdc_write_string("SV[<duty>]/BP[KP/KI/KD/SC/PZ/PCM/TGT <v>]/HELP\n");
    }
}
