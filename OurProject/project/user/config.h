#ifndef __CONFIG_H__
#define __CONFIG_H__

#include "zf_common_headfile.h"

typedef struct {
    int16 motor1_kp, motor1_ki, motor1_kd, motor1_target;
    int16 motor2_kp, motor2_ki, motor2_kd, motor2_target;
    int16 steer_kp,  steer_ki,  steer_kd,  steer_max, steer_min;
} config_t;

void config_save(void);
void config_load(void);

#endif
