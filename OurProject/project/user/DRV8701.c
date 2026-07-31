#include "DRV8701.h"

int16 drv8701_state[4] = {0, 0, 0, 0};

static const uint8 drv8701_pins[4] = {
    DRV8701_IN1,   /* P60 */
    DRV8701_IN2,   /* P62 */
    DRV8701_IN3,   /* P40 */
    DRV8701_IN4,   /* P42 */
};

/** 初始化 4 脚为 GPIO 推挽输出 */
void DRV8701_TestInit(void)
{
    uint8 i;
    for (i = 0; i < 4; i++)
    {
        gpio_init(drv8701_pins[i], GPO, 0, GPO_PUSH_PULL);
    }
}

/** 将 drv8701_state[] 同步到 GPIO */
void DRV8701_SyncPins(void)
{
    uint8 i;
    for (i = 0; i < 4; i++)
    {
        gpio_set_level(drv8701_pins[i], drv8701_state[i] ? 1 : 0);
    }
}
