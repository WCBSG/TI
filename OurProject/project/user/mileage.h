/*********************************************************************************************************************
* 文件名称          mileage.h
* 说明              里程/弯道过弯参数 — 编码器里程开环过弯
*
* 参数（config 槽 22-27，menu "Mileage" 页可编辑，CALM 命令标定）：
*   pulses_per_cm_lr  左轮每 cm 脉冲数（CALM 标定）
*   pulses_per_cm_rr  右轮每 cm 脉冲数（弯道里程用内轮=右轮）
*   curve_spd         弯道固定基准 duty
*   curve_diff        弯道固定差速
*   straight_len_cm   AB/CD 直线长 cm（默认 150）
*   curve_len_cm      半圆弧长 cm（默认 157，r=0.5m）
*
* 用法：main config_load 后调 mileage_load() 加载；config_save 自动保存
********************************************************************************************************************/

#ifndef __MILEAGE_H__
#define __MILEAGE_H__

#include "zf_common_headfile.h"

extern int16 pulses_per_cm_lr;   /* [22] 左轮每 cm 脉冲 */
extern int16 pulses_per_cm_rr;   /* [23] 右轮每 cm 脉冲 */
extern int16 curve_spd;          /* [24] 弯道基准 duty */
extern int16 curve_diff;         /* [25] 弯道差速 */
extern int16 straight_len_cm;    /* [26] AB/CD 直线长 cm */
extern int16 curve_len_cm;       /* [27] 半圆弧长 cm */

void mileage_load(void);         /* 从 flash_buff 加载（config_valid 后调用） */

#endif
