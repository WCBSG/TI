#ifndef __PID_H
#define __PID_H

#include "zf_common_headfile.h"


typedef struct {
	int16 Target;
	int16 Actual;
	int16 Out;
	
	int16 Kp;
	int16 Ki;
	int16 Kd;
	
	int16 Error0;
	int16 Error1;
	int16 Error2;
	
	int16 OutMax;
	int16 OutMin;
} PID_t;

extern PID_t motor1_pid;
extern PID_t motor2_pid;

void PID_Update(PID_t *p);
void motor1_pid_init(void);
void motor2_pid_init(void);
#endif
