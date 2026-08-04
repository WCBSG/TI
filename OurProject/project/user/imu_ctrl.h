/* imu_ctrl.h — IMU660RA 陀螺仪（任务 2 停车辅助专用，task2 内管理） */
#ifndef __IMU_CTRL_H__
#define __IMU_CTRL_H__

#include "zf_common_headfile.h"

#define YAW_LAP_MIN   300    /* 任务 2 停车辅助：yaw 累计 ≥300° 才算走完一圈 */

extern int32 imu_yaw_x100;   /* 0.01°，yaw 累计角度 */
extern uint8 imu_active;     /* 1=采样中 */

void imu_ctrl_start(void);           /* SPI3 初始化 + 静止零点标定（约 1s） */
void imu_ctrl_tick(uint16 dt_ms);    /* 周期调用：读角速度 → yaw 积分 */
void imu_ctrl_stop(void);            /* 停止采样 + 清零 */

#endif