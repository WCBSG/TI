/*********************************************************************************************************************
* 文件名称          config.c
* 说明              PID 参数持久化（IAP/EEPROM）+ 校验和 + 边界验证
*
* Flash 布局 (FLASH_SIZE=32, 每槽 int16):
*   [0-2]   steer PID: Kp, Ki, Kd
*   [3]     steer OutMax
*   [4]     steer OutMin
*   [5]     base_speed (基准 duty)
*   [6-11]  (保留)
*   [12-16] ball PID: Kp, Ki, Kd, OutMax, OutMin
*   [17]    servo_center_duty (舵机中位 duty)
*   [18]    pixel_zero (O 点像素 X)
*   [19]    px_per_cm (每 cm 像素数)
*   [20]    ball_target_cm_x10 (任务6 球目标位置, 0.1cm)
*   [21]    kd_yaw (陀螺仪阻尼系数, 0-200)
*   [22-27] (保留)
*   [28]    CONFIG_TAG  (0x5AA5)
*   [29]    XOR 校验和 (slot 0-28)
*   [30-31] (保留)
*
* 注意：
*   1. config_load() 必须在任何消费 flash_buff 的初始化之前调用。
*   2. 布局变更会使旧版本保存的配置失效（TAG 位置不同），需重新保存一次。
********************************************************************************************************************/

#include "config.h"
#include "line_ctrl.h"    /* steer_pid */
#include "menu_defs.h"    /* base_speed */
#include "ball_ctrl.h"    /* ball_pid + 标定参数 */

#define FLASH_ADDR   0x0000
#define CONFIG_TAG   0x5AA5

int16 flash_buff[FLASH_SIZE] = { 0 };
int16 down_buff[FLASH_SIZE] = { 0 };

/* ── 内部：计算 slot 0-28 的 XOR 校验和 ── */
static int16 calc_checksum(const int16 *buf)
{
    int16 cs = 0;
    uint8 i;
    for (i = 0; i <= 28; i++)
        cs ^= buf[i];
    return cs;
}

/* ── 内部：边界验证（拒绝明显异常值） ── */
static uint8 validate_bounds(void)
{
    uint8 idx;
    static const int16 lo[] = {
        /* steer Kp/Ki/Kd  OutMax  OutMin  base  | 保留6-11 */
        0,0,0,  0, -2000, 0,  0,0,0,0,0,0,
        /* ball Kp/Ki/Kd  OutMax  OutMin  | 标定 */
        0,0,0,  0, -500,  1500, 0, 1, -50,   /* [17] servo_center_duty 按 300Hz(0.5~2.5ms=1500~7500) */
        /* [21] kd_yaw, 保留22-27 */
        0,0,0,0,0,0,0
    };
    static const int16 hi[] = {
        /* steer */
        200,200,200, 2000, 0, 4000,  0,0,0,0,0,0,
        /* ball */
        200,200,200, 500, 0,  7500, 320, 200, 50,   /* [17] servo_center_duty 300Hz 上限 */
        /* [21] kd_yaw, 保留22-27 */
        200,0,0,0,0,0,0
    };
    for (idx = 0; idx <= 27; idx++)
        if (flash_buff[idx] < lo[idx] || flash_buff[idx] > hi[idx])
            return 0;
    return 1;
}

/* ═══════════════════════════════════════════════════════ */

void config_save(void)
{
    /* steer PID */
    down_buff[0]  = steer_pid.Kp;
    down_buff[1]  = steer_pid.Ki;
    down_buff[2]  = steer_pid.Kd;
    down_buff[3]  = steer_pid.OutMax;
    down_buff[4]  = steer_pid.OutMin;
    down_buff[5]  = base_speed;

    /* 保留 6-11 */
    down_buff[6]  = 0; down_buff[7]  = 0;
    down_buff[8]  = 0; down_buff[9]  = 0;
    down_buff[10] = 0; down_buff[11] = 0;

    /* ball PID */
    down_buff[12] = ball_pid.Kp;
    down_buff[13] = ball_pid.Ki;
    down_buff[14] = ball_pid.Kd;
    down_buff[15] = ball_pid.OutMax;
    down_buff[16] = ball_pid.OutMin;

    /* 标定参数 */
    down_buff[17] = servo_center_duty;
    down_buff[18] = pixel_zero;
    down_buff[19] = px_per_cm;
    down_buff[20] = ball_target_cm_x10;

    /* [21] kd_yaw 陀螺仪阻尼, [22-27] 保留 */
    down_buff[21] = kd_yaw;
    down_buff[22] = 0; down_buff[23] = 0;
    down_buff[24] = 0; down_buff[25] = 0;
    down_buff[26] = 0; down_buff[27] = 0;

    down_buff[28] = CONFIG_TAG;
    down_buff[29] = calc_checksum(down_buff);
    down_buff[30] = 0;
    down_buff[31] = 0;

    iap_erase_page(FLASH_ADDR);
    iap_write_buff(FLASH_ADDR, (uint8 *)down_buff, sizeof(down_buff));
}

void config_load(void)
{
    iap_read_buff(FLASH_ADDR, (uint8 *)flash_buff, sizeof(flash_buff));
}

uint8 config_valid(void)
{
    int16 expected_cs;

    if (flash_buff[28] != CONFIG_TAG)
        return 0;

    expected_cs = calc_checksum(flash_buff);
    if (flash_buff[29] != expected_cs)
        return 0;

    if (!validate_bounds())
        return 0;

    return 1;
}
