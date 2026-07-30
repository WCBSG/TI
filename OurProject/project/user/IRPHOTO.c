#include "IRPHOTO.h"

// 八路红外循迹传感器引脚 (P0.0 ~ P0.7)
static const gpio_pin_enum ir_pins[8] = {
    IO_P00, IO_P01, IO_P02, IO_P03,
    IO_P04, IO_P05, IO_P06, IO_P07,
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

// 在 TFT180 屏幕上显示 8 路红外传感器状态
void IRPHOTO_Display(int s[8])
{
    int i;
    for (i = 0; i < 8; i++)
    {
        tft180_show_uint8(10 + i * 8, 50, s[i]);   // 横向排列 8 个值 (0/1)
    }
    tft180_show_int8(10, 70, calc_error(s));        // 下一行显示加权偏差
}

// 停车标志检测：≥3 个连续传感器同时检测到黑线
// 附加保护：全部 8 路未检测到黑线 → 视为冲出赛道，返回 2（区别于正常停车 1）
int is_stop(int s[8])
{
    int i;
    int count = 0;
    int has_line = 0;

    for (i = 0; i < 8; i++)
    {
        if (s[i] == 1)
        {
            count++;
            has_line = 1;
            if (count >= 4)
                return 1;       // 停车标志
        }
        else
        {
            count = 0;
        }
    }

    if (!has_line)
        return 2;               // 冲出赛道保护（区别于停车）

    return 0;                   // 正常行驶
}


void IRPHOTO_Read(int s[8])
{
    int i;
    for (i = 0; i < 8; i++)
        s[i] = !gpio_get_level(ir_pins[i]);  // 黑=1, 白=0
}