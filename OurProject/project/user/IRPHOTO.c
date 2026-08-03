/* IRPHOTO.c — 八路红外循迹：GPIO 读取 + 加权偏差
 * 引脚：与 IMU SPI3 (P87/P85/P86/P34) 完全错开，无冲突 */
#include "IRPHOTO.h"

static const gpio_pin_enum ir_pins[8] = {
    IO_P94, IO_P54, IO_P93, IO_P92,
    IO_P14, IO_P11, IO_P07, IO_P06,
};

void IRPHOTO_Init(void)
{
    int i;
    for (i = 0; i < 8; i++) gpio_init(ir_pins[i], GPI, 1, GPI_PULL_UP);
}

int calc_error(int s[8])   /* 权重 {-10,-7,-4,-1,1,4,7,10}；正值偏右，负值偏左，0 居中 */
{
    static const int8 weight[8] = { -10, -7, -4, -1, 1, 4, 7, 10 };
    int error = 0, i;
    for (i = 0; i < 8; i++) if (s[i]) error += weight[i];
    return error;
}

void IRPHOTO_Read(int s[8])
{
    int i;
    for (i = 0; i < 8; i++) s[i] = !gpio_get_level(ir_pins[i]);   /* 黑=1, 白=0 */
}
