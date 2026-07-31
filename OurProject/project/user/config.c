#include "config.h"
#include "PID.h"
#include "control.h"
#include "Motor.h"

#define FLASH_ADDR  0x0000
#define CONFIG_TAG  0x5AA5

int16 flash_buff[FLASH_SIZE] = { 0 };
int16 down_buff[FLASH_SIZE] = { 0 };

void config_save(void)
{
    down_buff[0]  = motor1_pid.Kp;
    down_buff[1]  = motor1_pid.Ki;
    down_buff[2]  = motor1_pid.Kd;
    down_buff[3]  = motor1_pid.Target;

    down_buff[4]  = motor2_pid.Kp;
    down_buff[5]  = motor2_pid.Ki;
    down_buff[6]  = motor2_pid.Kd;
    down_buff[7]  = motor2_pid.Target;

    down_buff[8]  = steer_pid.Kp;
    down_buff[9]  = steer_pid.Ki;
    down_buff[10] = steer_pid.Kd;
    down_buff[11] = steer_pid.OutMax;
    down_buff[12] = steer_pid.OutMin;

    down_buff[13] = CONFIG_TAG;

    iap_erase_page(FLASH_ADDR);
    iap_write_buff(FLASH_ADDR, (uint8 *)down_buff, sizeof(down_buff));
}

void config_load(void)
{
    iap_read_buff(FLASH_ADDR, (uint8 *)flash_buff, sizeof(flash_buff));
}

uint8 config_valid(void)
{
    return (flash_buff[13] == CONFIG_TAG);
}
