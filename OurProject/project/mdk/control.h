#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "zf_common_headfile.h"


void steer_init(void);
void steer_set(int error, int base_speed);  // 输入误差和基础速度，自动差速


#endif