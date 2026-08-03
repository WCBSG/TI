/* imu_ctrl.c — IMU660RA 陀螺仪（仅任务 2 停车辅助，其他场景不采样省算力）
 * 硬件：SPI3 SCK=P87 MOSI=P85 MISO=P86 CS=P34（库默认，与 CLAUDE.md 一致）
 * yaw 轴 = X 轴（模块竖插），方向 IMU_YAW_SIGN 反了改 -1
 * 任务 2 停车辅助：yaw 累计 ≥YAW_LAP_MIN 确认走完一整圈 */
#include "imu_ctrl.h"

int32 imu_yaw_x100 = 0;      /* 0.01°，yaw 累计角度 */
uint8  imu_active  = 0;      /* 1=采样中 */

static int16 gyro_bias = 0;  /* 零点偏置（消除积分漂移） */

#define IMU_YAW_RAW()   imu660ra_gyro_x
#define IMU_YAW_SIGN    (1)

/* 初始化 + 静止零点标定（200 次平均偏置） */
void imu_ctrl_start(void)
{
    uint32 sum = 0;
    uint16 i;
    if (imu660ra_init() != 0) { imu_active = 0; return; }   /* 初始化失败不阻塞 */
    for (i = 0; i < 200; i++)
    {
        imu660ra_get_gyro();
        sum += (uint32)(IMU_YAW_SIGN * IMU_YAW_RAW());
        system_delay_ms(5);
    }
    gyro_bias = (int16)(sum / 200);
    imu_yaw_x100 = 0;
    imu_active = 1;
}

/* 周期调用（任务 2 主循环每 ~5ms）：读角速度 → yaw 积分 */
void imu_ctrl_tick(uint16 dt_ms)
{
    int32 raw, dps_x100;
    if (!imu_active) return;
    imu660ra_get_gyro();
    raw = (int32)IMU_YAW_SIGN * IMU_YAW_RAW() - gyro_bias;
    dps_x100 = (int32)(imu660ra_gyro_transition((int16)raw) * 100.0f);   /* 0.01°/s */
    imu_yaw_x100 += dps_x100 * (int32)dt_ms / 1000;   /* θ_x100 += w_x100 * dt_ms/1000 */
}

void imu_ctrl_stop(void)
{
    imu_active = 0;
    imu_yaw_x100 = 0;
}
