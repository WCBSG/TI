#include "Motor.h"

int16 encoder_data_dir_1, encoder_data_dir_2;
uint8 driving = 0;

void Motor_Init(void)
{
    pwm_init(PWM_1,17000,0);
    pwm_init(PWM_2,17000,0);
    gpio_init(DIR_1, GPO, 1, GPO_PUSH_PULL);
    gpio_init(DIR_2, GPO, 1, GPO_PUSH_PULL);

}

void encoder_init(void)
{
    encoder_dir_init(ENCODER_DIR_1, ENCODER_DIR_PULSE_1, ENCODER_DIR_DIR_1);   	
    encoder_dir_init(ENCODER_DIR_2, ENCODER_DIR_PULSE_2, ENCODER_DIR_DIR_2);
}

void motor1_control(int16 motor_duty)
{
   if(motor_duty >= 0)                                                          
   {
        gpio_set_level(DIR_1, 1);                        
        pwm_set_duty(PWM_1, motor_duty);                   
	}
	else 
	{
		gpio_set_level(DIR_1, 0);                               
      pwm_set_duty(PWM_1, -motor_duty);  
	}
                           
}

void motor2_control(int16 motor_duty)
{
   if(motor_duty >= 0)                                                          
   {
        gpio_set_level(DIR_2, 1);                                
        pwm_set_duty(PWM_2, motor_duty);                   
	}
	else 
	{
		gpio_set_level(DIR_2, 0);                             
      pwm_set_duty(PWM_2, -motor_duty);  
	}
                           
}

void pit_handler(void)
{
   
    encoder_data_dir_1 = encoder_get_count(ENCODER_DIR_1);
    encoder_data_dir_2 = encoder_get_count(ENCODER_DIR_2);

    encoder_clear_count(ENCODER_DIR_1);
    encoder_clear_count(ENCODER_DIR_2);

    if (driving == 1)
    {
        motor1_pid.Actual = encoder_data_dir_1;
        PID_Update(&motor1_pid);
        motor1_control(motor1_pid.Out);

        motor2_pid.Actual = encoder_data_dir_2;
        PID_Update(&motor2_pid);
        motor2_control(motor2_pid.Out);
    }
    
}