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

	int32 Integral;      /* 位置式积分累积（Ki 以 0.1 为单位，余数累积） */
	int32 DFilter;       /* D 项定点低通状态（放大 16 倍） */

	int16 OutMax;
	int16 OutMin;
} PID_t;

void PID_Reset(PID_t *p);
void PID_Update(PID_t *p);
#endif
