#include "IRPHOTO.h"

// 八路红外循迹传感器引脚
static const gpio_pin_enum ir_pins[8] = {
    IO_P94, IO_P54, IO_P93, IO_P92,
    IO_P14, IO_P11, IO_P07, IO_P06,
};

void IRPHOTO_Init(void)
{
    int i;
    for (i = 0; i < 8; i++)
    {
        gpio_init(ir_pins[i], GPI, 1, GPI_PULL_UP);
    }
}

// 加权偏差计算
// 权重: {-7,-5,-3,-1, 1,3,5,7} — 两侧权重高，中间低
// 正值=偏右，负值=偏左，0=居中
int calc_error(int s[8])
{
    static const int8 weight[8] = { -7, -5, -3, -1, 1, 3, 5, 7 };
    int error = 0;
    int i;
    for (i = 0; i < 8; i++)
    {
        if (s[i] == 1)
            error += weight[i];
    }
    return error;
}


void IRPHOTO_Read(int s[8])
{
    int i;
    for (i = 0; i < 8; i++)
        s[i] = !gpio_get_level(ir_pins[i]);  // 黑=1, 白=0
}