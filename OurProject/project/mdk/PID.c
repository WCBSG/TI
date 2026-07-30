#include "PID.h"

PID_t motor1_pid;
PID_t motor2_pid;

void PID_Update(PID_t *p)
{
	
	p->Error2 = p->Error1;
	p->Error1 = p->Error0;					
	p->Error0 = p->Target - p->Actual;		

	p->Out += p->Kp * (p->Error0-p->Error1)
		   + (p->Ki/10.0) * p->Error0
		   + p->Kd * (p->Error0 - 2*p->Error1+p->Error2);
	
	if (p->Out > p->OutMax) {p->Out = p->OutMax;}	
	if (p->Out < p->OutMin) {p->Out = p->OutMin;}	
}

void motor1_pid_init(void)
{
	motor1_pid.Target=0;
	motor1_pid.Kp=38;      //flash_buff[0]
	motor1_pid.Ki=30;       //flash_buff[1]     
	motor1_pid.Kd=0;
	motor1_pid.OutMax=3000;
	motor1_pid.OutMin=-3000;
}

void motor2_pid_init(void)
{
	motor2_pid.Target=0;
	motor2_pid.Kp=38;      //flash_buff[2]
	motor2_pid.Ki=30;      //flash_buff[3]
	motor2_pid.Kd=0;
	motor2_pid.OutMax=3000;
	motor2_pid.OutMin=-3000;
}
