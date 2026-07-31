#ifndef __DRV8701_H__
#define __DRV8701_H__

#include "zf_common_headfile.h"

/* DRV8701 控制引脚 — 测试模式 */
#define DRV8701_IN1    IO_P60   /* 电机1 IN1 */
#define DRV8701_IN2    IO_P62   /* 电机1 IN2 */
#define DRV8701_IN3    IO_P40   /* 电机2 IN1 */
#define DRV8701_IN4    IO_P42   /* 电机2 IN2 */

extern int16 drv8701_state[4];   /* 4 路状态 0=Low 1=High */

void DRV8701_TestInit(void);     /* 初始化 4 脚为 GPIO 推挽输出 */
void DRV8701_SyncPins(void);     /* 将 drv8701_state[] 同步到 GPIO */

#endif
