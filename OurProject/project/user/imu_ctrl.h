/*********************************************************************************************************************
* 文件名称          imu_ctrl.h
* 说明              陀螺仪（IMU660RA）控制接口 — 车体航向反馈
*
* 硬件：IMU660RA 六轴（陀螺仪+加速度计），硬件 SPI3（库默认引脚）
*   SCL/SPC = P87(SPI3_SCK)   SDA/SDI = P85(SPI3_MOSI)
*   SA0/SDO = P86(SPI3_MISO)  CS      = P34
*   VCC=3V3  GND=地   INT2=P36（暂不使用，轮询读取）
*
* 数据流（每 10ms 主循环调用 imu_ctrl_tick）：
*   yaw轴角速度(原始, 实测为 GX) --去偏置--> dps(0.1°/s) --积分--> yaw(0.01°)
*
* 静态零点标定：imu_ctrl_init 内静止采样 200 次求 yaw 轴平均偏置，
*   消除积分漂移（这是航向可用性的关键）。
********************************************************************************************************************/

#ifndef __IMU_CTRL_H__
#define __IMU_CTRL_H__

#include "zf_common_headfile.h"

/* ── 全局诊断量 ── */
extern uint8  imu_ok;             /* 1=初始化+标定成功（否则所有功能自动失效） */
extern int16  imu_gyro_z_bias;    /* 静态零点偏置（原始量，标定时采集） */
extern int16  imu_gyro_dps_x10;   /* 最近一次 Z 轴角速度（0.1°/s，已去偏置） */
extern int16  imu_yaw_deg;        /* 航向角（整数度，诊断显示用） */
extern int32  imu_yaw_x100;       /* 航向角（0.01°，相对标定起点） */

/* ── 接口 ── */
void imu_ctrl_init(void);            /* IMU 初始化（失败重试，不阻塞主流程）+ 静止 1s 零点标定 */
void imu_ctrl_tick(uint16 dt_ms);    /* 周期调用：读 gyro → yaw 积分（dt_ms 由调用方用 pit_tick 实测） */
void imu_ctrl_reset_yaw(void);       /* 航向角清零 */

#endif
