#ifndef __CONTROL_H__
#define __CONTROL_H__

#include "zf_common_headfile.h"
#include "PID.h"

extern PID_t steer_pid;

void steer_init(void);
void steer_set(int error, int base_speed);

#endif
