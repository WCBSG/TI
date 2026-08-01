/*********************************************************************************************************************
* 文件名称          imu_ctrl.c
* 说明              陀螺仪（IMU660RA）实现 — 采集 + yaw 积分
*
* 关键点：
*   1. 初始化失败重试 5 次后放弃（imu_ok=0），不阻塞主流程（IMU 没接时其他功能照常）
*   2. 静态零点标定：静止采样 200 次（×5ms≈1s）求 yaw 轴平均偏置
*   3. 角速度定点转换：°/s = raw/16.4，0.1°/s 定点 → raw*100/164 = raw*25/41
*   4. yaw 积分（0.01°）：每步 = dps_x10 * dt_ms / 100
*   5. 采集放主循环（非中断）：软硬件 SPI 在中断里跑有被高优先级打断的风险，
*      主循环 10ms 节拍对航向反馈足够
*
* yaw 轴选择（重要）：
*   模块竖插时，车绕竖轴(yaw)的角速度投影在模块 X 轴上（实测 GX 响应最大），
*   故用 X 轴做偏航。若实际安装导致方向反了（顺时针转 YAW 减小），
*   把 IMU_YAW_SIGN 改为 -1 即可。
********************************************************************************************************************/

#include "imu_ctrl.h"

/* ── yaw 轴配置 ──
 * IMU_YAW_RAW() : 车绕竖轴(yaw)的角速度原始量所在轴（实测 = X 轴）
 * IMU_YAW_SIGN  : 方向（+1 顺时针转 YAW 增；反了改 -1） */
#define IMU_YAW_RAW()   (imu660ra_gyro_x)
#define IMU_YAW_SIGN    (1)

uint8  imu_ok           = 0;
int16  imu_gyro_z_bias  = 0;   /* yaw 轴（X）零点偏置，命名沿用历史 */
int16  imu_gyro_dps_x10 = 0;
int16  imu_yaw_deg      = 0;
int32  imu_yaw_x100     = 0;

void imu_ctrl_init(void)
{
    uint16 i;
    int32  sum = 0;
    uint8  retry;

    /* 初始化：失败重试 5 次（IMU 上电自检可能需多次），仍失败则放弃不阻塞 */
    for (retry = 0; retry < 5; retry++)
    {
        if (!imu660ra_init()) break;
        system_delay_ms(100);
    }
    if (retry >= 5)
    {
        imu_ok = 0;
        return;
    }

    /* 静态零点标定：静止采样 200 次（×5ms≈1s）求 yaw 轴平均偏置 */
    for (i = 0; i < 200; i++)
    {
        imu660ra_get_gyro();
        sum += (int32)IMU_YAW_SIGN * IMU_YAW_RAW();
        system_delay_ms(5);
    }
    imu_gyro_z_bias = (int16)(sum / 200);

    imu_yaw_x100  = 0;
    imu_yaw_deg   = 0;
    imu_gyro_dps_x10 = 0;
    imu_ok        = 1;
}

void imu_ctrl_reset_yaw(void)
{
    imu_yaw_x100 = 0;
    imu_yaw_deg  = 0;
}

void imu_ctrl_tick(uint16 dt_ms)
{
    int32 raw, dps_x10;

    if (!imu_ok) return;

    imu660ra_get_gyro();

    /* yaw 轴角速度（去偏置）→ 0.1°/s：raw/16.4*10 = raw*100/164 = raw*25/41 */
    raw     = (int32)IMU_YAW_SIGN * IMU_YAW_RAW() - imu_gyro_z_bias;
    dps_x10 = raw * 25 / 41;
    if (dps_x10 > 32767)  dps_x10 = 32767;
    if (dps_x10 < -32768) dps_x10 = -32768;
    imu_gyro_dps_x10 = (int16)dps_x10;

    /* yaw 积分（0.01°）：每步 = dps(°/s) * dt(s) → °/100 = dps_x10 * dt_ms / 100 */
    imu_yaw_x100 += (int32)imu_gyro_dps_x10 * (int32)dt_ms / 100;

    /* 整数度（诊断显示，±32767° 远超一圈够用） */
    imu_yaw_deg = (int16)(imu_yaw_x100 / 100);
}
