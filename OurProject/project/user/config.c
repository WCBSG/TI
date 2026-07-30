#include "config.h"
#include "PID.h"
#include "control.h"

#define FLASH_ADDR  0x0000

void config_save(void)
{
    config_t cfg;

    cfg.motor1_kp     = motor1_pid.Kp;
    cfg.motor1_ki     = motor1_pid.Ki;
    cfg.motor1_kd     = motor1_pid.Kd;
    cfg.motor1_target = motor1_pid.Target;

    cfg.motor2_kp     = motor2_pid.Kp;
    cfg.motor2_ki     = motor2_pid.Ki;
    cfg.motor2_kd     = motor2_pid.Kd;
    cfg.motor2_target = motor2_pid.Target;

    cfg.steer_kp      = steer_pid.Kp;
    cfg.steer_ki      = steer_pid.Ki;
    cfg.steer_kd      = steer_pid.Kd;
    cfg.steer_max     = steer_pid.OutMax;
    cfg.steer_min     = steer_pid.OutMin;

    iap_erase_page(FLASH_ADDR);
    iap_write_buff(FLASH_ADDR, (uint8 *)&cfg, sizeof(config_t));
}

void config_load(void)
{
    config_t cfg;

    iap_read_buff(FLASH_ADDR, (uint8 *)&cfg, sizeof(config_t));

    // 检查是否有效（全 FF 说明没写过）
    if ((uint8)(cfg.motor1_kp) == 0xFF) return;

    motor1_pid.Kp     = cfg.motor1_kp;
    motor1_pid.Ki     = cfg.motor1_ki;
    motor1_pid.Kd     = cfg.motor1_kd;
    motor1_pid.Target = cfg.motor1_target;

    motor2_pid.Kp     = cfg.motor2_kp;
    motor2_pid.Ki     = cfg.motor2_ki;
    motor2_pid.Kd     = cfg.motor2_kd;
    motor2_pid.Target = cfg.motor2_target;

    steer_pid.Kp      = cfg.steer_kp;
    steer_pid.Ki      = cfg.steer_ki;
    steer_pid.Kd      = cfg.steer_kd;
    steer_pid.OutMax  = cfg.steer_max;
    steer_pid.OutMin  = cfg.steer_min;
}
