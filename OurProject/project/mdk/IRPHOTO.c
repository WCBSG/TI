#include "IRPHOTO.h"

// static const gpio_pin_enum ir_pins[8] = {
//     IO_P00, IO_P01, IO_P02, IO_P03,  
//     IO_P04, IO_P05, IO_P06, IO_P07,
// };//预设

// void IRPHOTO_Init(void)
// {
//     int i;
//     for (i = 0; i < 8; i++)
//     {
//          gpio_init(ir_pins[i], GPI, 0, GPI_PULL_UP);  
//     }
// }


int calc_error(int s[8])//八路光电管循迹
{
    int error = 0;
    int last_error = 0;
    int weight[8] = { -7, -5, -3, -1, 1, 3, 5, 7 };
    int i;
    for (i = 0; i < 8; i++)
    {
        if (s[i] == 1)
        {
            error += weight[i]; 
        }
    } 
    return error;
}

int is_stop(int s[8])//停止标志。连续三个光电管为黑色时，返回1，否则返回0
{
    int i;
    int count = 0;
    int has_line = 0; // 标记是否有光电管检测到黑色线
    for (i = 0; i < 8; i++)
    {
        if (s[i] == 1)
        {
            count++;
            has_line = 1; // 标记有光电管检测到黑色线
        
            if (count >= 3 )//停止标志
            {
                return 1;
            }

        }
        else
        {
        count = 0; // 重置计数器
        }
    }
    if (!has_line)
        return 1; // 出界保护

    return 0; // 没有停止标志
}


    