/*********************************************************************************************************************
* 文件名称          config.c
* 说明              配置持久化（IAP/EEPROM）+ 校验和 + 边界验证
*
* Flash 布局 (FLASH_SIZE=32, 每槽 int16):
*   [0]     steer_kp (纯比例巡线增益, 官方 280)
*   [1]     base_speed (基准 duty)
*   [2-6]   ball PID: Kp, Ki, Kd, OutMax, OutMin
*   [7]     servo_center_duty (舵机中位 duty)
*   [8]     pixel_zero (O 点像素 X)
*   [9]     px_per_cm (每 cm 像素数)
*   [10]    ball_target_cm_x10 (任务6 球目标位置, 0.1cm)
*   [11]    CONFIG_TAG  (0x5AA5)
*   [12]    XOR 校验和 (slot 0-11)
*   [13-31] (保留)
*
* 注意：
*   1. config_load() 必须在任何消费 flash_buff 的初始化之前调用。
*   2. 布局变更会使旧版本保存的配置失效（TAG 位置不同），需重新保存一次。
********************************************************************************************************************/

#include "config.h"
#include "line_ctrl.h"    /* steer_kp */
#include "menu_defs.h"    /* base_speed */
#include "ball_ctrl.h"    /* ball_pid + 标定参数 */

#define FLASH_ADDR   0x0000
#define CONFIG_TAG   0x5AA5

int16 flash_buff[FLASH_SIZE] = { 0 };
int16 down_buff[FLASH_SIZE] = { 0 };

/* ── 内部：计算 slot 0-11 的 XOR 校验和 ── */
static int16 calc_checksum(const int16 *buf)
{
    int16 cs = 0;
    uint8 i;
    for (i = 0; i <= 11; i++)
        cs ^= buf[i];
    return cs;
}

/* ── 内部：边界验证（拒绝明显异常值） ── */
static uint8 validate_bounds(void)
{
    uint8 idx;
    static const int16 lo[] = {
        /* [0]steer_kp [1]base | [2-6]ball Kp/Ki/Kd/OutMax/OutMin | [7-10]标定 */
        0,  0,  0,0,0,  0, -500,  3500, 0, 1, -50
    };
    static const int16 hi[] = {
        /* [0]steer_kp [1]base | [2-6]ball Kp/Ki/Kd/OutMax/OutMin | [7-10]标定 */
        500, 6000,  200,200,200,  500, 0,  5500, 319, 200, 50
    };
    for (idx = 0; idx <= 10; idx++)
        if (flash_buff[idx] < lo[idx] || flash_buff[idx] > hi[idx])
            return 0;
    return 1;
}

/* ═══════════════════════════════════════════════════════ */

void config_save(void)
{
    /* [0] steer_kp | [1] base_speed */
    down_buff[0] = steer_kp;
    down_buff[1] = base_speed;

    /* [2-6] ball PID: Kp, Ki, Kd, OutMax, OutMin */
    down_buff[2] = ball_pid.Kp;
    down_buff[3] = ball_pid.Ki;
    down_buff[4] = ball_pid.Kd;
    down_buff[5] = ball_pid.OutMax;
    down_buff[6] = ball_pid.OutMin;

    /* [7-10] 标定参数 */
    down_buff[7]  = servo_center_duty;
    down_buff[8]  = pixel_zero;
    down_buff[9]  = px_per_cm;
    down_buff[10] = ball_target_cm_x10;

    down_buff[11] = CONFIG_TAG;
    down_buff[12] = calc_checksum(down_buff);

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

    if (flash_buff[11] != CONFIG_TAG)
        return 0;

    expected_cs = calc_checksum(flash_buff);
    if (flash_buff[12] != expected_cs)
        return 0;

    if (!validate_bounds())
        return 0;

    return 1;
}
