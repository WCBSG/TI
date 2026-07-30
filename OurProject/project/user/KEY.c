#include "KEY.h"

#define KEY1_PIN        IO_P92
#define KEY2_PIN        IO_P93
#define KEY3_PIN        IO_P94
#define KEY4_PIN        IO_P95
#define KEY5_PIN        IO_P96

#define SWITCH1_PIN     IO_P90
#define SWITCH2_PIN     IO_P91

uint8 key1_status;
uint8 key2_status;
uint8 key3_status;
uint8 key4_status;
uint8 key5_status;

uint8 key1_last_status=1;
uint8 key2_last_status=1;
uint8 key3_last_status=1;
uint8 key4_last_status=1;
uint8 key5_last_status=1;

uint8 key1_flag;
uint8 key2_flag;
uint8 key3_flag;
uint8 key4_flag;
uint8 key5_flag;

void button_init(void)
{
    gpio_init(KEY1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY2_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY3_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY4_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(KEY5_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(SWITCH1_PIN, GPI, 1, GPI_PULL_UP);
    gpio_init(SWITCH2_PIN, GPI, 1, GPI_PULL_UP);
}

void button_control(void)
{
    key1_status = gpio_get_level(KEY1_PIN);
    key2_status = gpio_get_level(KEY2_PIN);
    key3_status = gpio_get_level(KEY3_PIN);
    key4_status = gpio_get_level(KEY4_PIN);
    key5_status = gpio_get_level(KEY5_PIN);

    if(key1_status && !key1_last_status)    key1_flag = 1;
    if(key2_status && !key2_last_status)    key2_flag = 1;
    if(key3_status && !key3_last_status)    key3_flag = 1;
    if(key4_status && !key4_last_status)    key4_flag = 1;
    if(key5_status && !key5_last_status)    key5_flag = 1;

    key1_last_status = key1_status;
    key2_last_status = key2_status;
    key3_last_status = key3_status;
    key4_last_status = key4_status;
    key5_last_status = key5_status;
}
