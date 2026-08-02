/*********************************************************************************************************************
* 文件名称          mileage.c
* 说明              里程/弯道过弯参数实现
*
* 默认值：
*   pulses_per_cm_lr/rr = 100（待 CALM 标定）
*   curve_spd = 2500, curve_diff = 800（弯道右转固定差速）
*   straight_len_cm = 150, curve_len_cm = 157
*
* mileage_load() 在 config_valid() 时从 flash_buff[22-27] 加载
********************************************************************************************************************/

#include "mileage.h"
#include "config.h"    /* flash_buff, config_valid */

int16 pulses_per_cm_lr = 100;    /* [22] 左轮每 cm 脉冲 */
int16 pulses_per_cm_rr = 100;    /* [23] 右轮每 cm 脉冲 */
int16 curve_spd        = 2500;   /* [24] 弯道基准 duty */
int16 curve_diff       = 1300;   /* [25] 弯道差速：800 转弯半径太大出线，调大缩小半径 */
int16 straight_len_cm  = 150;    /* [26] AB/CD 直线长 cm */
int16 curve_len_cm     = 157;    /* [27] 半圆弧长 cm */

void mileage_load(void)
{
    if (config_valid())
    {
        pulses_per_cm_lr = flash_buff[22];
        pulses_per_cm_rr = flash_buff[23];
        curve_spd        = flash_buff[24];
        curve_diff       = flash_buff[25];
        straight_len_cm  = flash_buff[26];
        curve_len_cm     = flash_buff[27];
    }
    /* 未标定保护：每 cm 脉冲数必须为正，否则用默认 */
    if (pulses_per_cm_lr <= 0) pulses_per_cm_lr = 100;
    if (pulses_per_cm_rr <= 0) pulses_per_cm_rr = 100;
}
