/*********************************************************************************************************************
* 文件名称          config.c
* 说明              PID 参数持久化（IAP/EEPROM）+ 校验和 + 边界验证
*
* Flash 布局 (FLASH_SIZE=16, 每槽 int16):
*   [0-2]   motor PID (共享): Kp, Ki, Kd
*   [3]     base_speed
*   [4-7]   (保留)
*   [8-12]  steer PID:  Kp, Ki, Kd, OutMax, OutMin
*   [13]    CONFIG_TAG  (0x5AA5)
*   [14]    XOR 校验和 (slot 0-13)
*   [15]    (保留)
********************************************************************************************************************/

#include "config.h"
#include "PID.h"
#include "control.h"
#include "menu_defs.h"   /* base_speed */

#define FLASH_ADDR   0x0000
#define CONFIG_TAG   0x5AA5

int16 flash_buff[FLASH_SIZE] = { 0 };
int16 down_buff[FLASH_SIZE] = { 0 };

/* ── 内部：计算 slot 0-13 的 XOR 校验和 ── */
static int16 calc_checksum(const int16 *buf)
{
    int16 cs = 0;
    uint8 i;
    for (i = 0; i <= 13; i++)
        cs ^= buf[i];
    return cs;
}

/* ── 内部：边界验证（拒绝明显异常值） ── */
static uint8 validate_bounds(void)
{
    uint8 idx;
    /* Kp/Ki/Kd: 0-200, base_speed: 1-200, OutMax/Min: 0-200 */
    static const int16 lo[] = {0,0,0, 1,  0,0,0,0,  0,0,0, 0, 0};
    static const int16 hi[] = {200,200,200, 200,  200,200,200,200,  200,200,200, 200, 200};
    for (idx = 0; idx <= 12; idx++)
        if (flash_buff[idx] < lo[idx] || flash_buff[idx] > hi[idx])
            return 0;
    return 1;
}

/* ═══════════════════════════════════════════════════════ */

void config_save(void)
{
    /* motor PID (共享) */
    down_buff[0]  = motor1_pid.Kp;
    down_buff[1]  = motor1_pid.Ki;
    down_buff[2]  = motor1_pid.Kd;
    down_buff[3]  = base_speed;

    /* 保留 */
    down_buff[4]  = 0;
    down_buff[5]  = 0;
    down_buff[6]  = 0;
    down_buff[7]  = 0;

    /* steer PID */
    down_buff[8]  = steer_pid.Kp;
    down_buff[9]  = steer_pid.Ki;
    down_buff[10] = steer_pid.Kd;
    down_buff[11] = steer_pid.OutMax;
    down_buff[12] = steer_pid.OutMin;

    down_buff[13] = CONFIG_TAG;
    down_buff[14] = calc_checksum(down_buff);
    down_buff[15] = 0;

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

    if (flash_buff[13] != CONFIG_TAG)
        return 0;

    expected_cs = calc_checksum(flash_buff);
    if (flash_buff[14] != expected_cs)
        return 0;

    if (!validate_bounds())
        return 0;

    return 1;
}

