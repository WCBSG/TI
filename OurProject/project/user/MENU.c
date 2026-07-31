#include "MENU.h"
#include "KEY.h"
#include "PID.h"
#include "Motor.h"
#include "control.h"
#include "config.h"

static uint8 menu_page = 0;
static uint8 menu_item = 0;
static uint8 prev_page = 0xFF;

static const uint8 item_count[5] = { 4, 4, 5, 1, 0 };

void menu_update(void)
{
    // ---- 翻页 (key1) ----
    if (key1_flag)
    {
        key1_flag = 0;
        config_save();
        menu_page++;
        menu_page %= 5;
        menu_item = 0;
    }

    // ---- 切换子项 (key2) ----
    if (key2_flag)
    {
        key2_flag = 0;
        menu_item++;
        menu_item %= item_count[menu_page];
    }

    // ---- 增加 (key3) ----
    if (key3_flag)
    {
        key3_flag = 0;
        switch (menu_page)
        {
            case 0:
                switch (menu_item) {
                    case 0: motor1_pid.Kp += 1; break;
                    case 1: motor1_pid.Ki += 1; break;
                    case 2: motor1_pid.Kd += 1; break;
                    case 3: motor1_pid.Target += 10; break;
                } break;
            case 1:
                switch (menu_item) {
                    case 0: motor2_pid.Kp += 1; break;
                    case 1: motor2_pid.Ki += 1; break;
                    case 2: motor2_pid.Kd += 1; break;
                    case 3: motor2_pid.Target += 10; break;
                } break;
            case 2:
                switch (menu_item) {
                    case 0: steer_pid.Kp += 1; break;
                    case 1: steer_pid.Ki += 1; break;
                    case 2: steer_pid.Kd += 1; break;
                    case 3: steer_pid.OutMax += 5; break;
                    case 4: steer_pid.OutMin += 5; break;
                } break;
            case 3:
                motor1_pid.Target += 10;
                motor2_pid.Target += 10;
                break;
        }
    }

    // ---- 减少 (key4) ----
    if (key4_flag)
    {
        key4_flag = 0;
        switch (menu_page)
        {
            case 0:
                switch (menu_item) {
                    case 0: if(motor1_pid.Kp > 0) motor1_pid.Kp -= 1; break;
                    case 1: if(motor1_pid.Ki > 0) motor1_pid.Ki -= 1; break;
                    case 2: if(motor1_pid.Kd > 0) motor1_pid.Kd -= 1; break;
                    case 3: if(motor1_pid.Target > 0) motor1_pid.Target -= 10; break;
                } break;
            case 1:
                switch (menu_item) {
                    case 0: if(motor2_pid.Kp > 0) motor2_pid.Kp -= 1; break;
                    case 1: if(motor2_pid.Ki > 0) motor2_pid.Ki -= 1; break;
                    case 2: if(motor2_pid.Kd > 0) motor2_pid.Kd -= 1; break;
                    case 3: if(motor2_pid.Target > 0) motor2_pid.Target -= 10; break;
                } break;
            case 2:
                switch (menu_item) {
                    case 0: if(steer_pid.Kp > 0) steer_pid.Kp -= 1; break;
                    case 1: if(steer_pid.Ki > 0) steer_pid.Ki -= 1; break;
                    case 2: if(steer_pid.Kd > 0) steer_pid.Kd -= 1; break;
                    case 3: if(steer_pid.OutMax > 0) steer_pid.OutMax -= 5; break;
                    case 4: steer_pid.OutMin -= 5; break;
                } break;
            case 3:
                if(motor1_pid.Target > 0) motor1_pid.Target -= 10;
                if(motor2_pid.Target > 0) motor2_pid.Target -= 10;
                break;
        }
    }
}

void menu_show(void)
{
    uint8 row = 1;

    if (menu_page != prev_page)
    {
        tft180_clear(0xFFFF);
        prev_page = menu_page;
    }

    if (menu_page == 0)
    {
        tft180_show_string(0, 0, " Motor1 PID");
        tft180_show_string(0, row*16, (menu_item==0)?"->Kp":"  Kp");
        tft180_show_int16(12*6, row*16, motor1_pid.Kp); row++;
        tft180_show_string(0, row*16, (menu_item==1)?"->Ki":"  Ki");
        tft180_show_int16(12*6, row*16, motor1_pid.Ki); row++;
        tft180_show_string(0, row*16, (menu_item==2)?"->Kd":"  Kd");
        tft180_show_int16(12*6, row*16, motor1_pid.Kd); row++;
        tft180_show_string(0, row*16, (menu_item==3)?"->Spd":"  Spd");
        tft180_show_int16(12*6, row*16, motor1_pid.Target); row++;
    }
    else if (menu_page == 1)
    {
        tft180_show_string(0, 0, " Motor2 PID");
        tft180_show_string(0, row*16, (menu_item==0)?"->Kp":"  Kp");
        tft180_show_int16(12*6, row*16, motor2_pid.Kp); row++;
        tft180_show_string(0, row*16, (menu_item==1)?"->Ki":"  Ki");
        tft180_show_int16(12*6, row*16, motor2_pid.Ki); row++;
        tft180_show_string(0, row*16, (menu_item==2)?"->Kd":"  Kd");
        tft180_show_int16(12*6, row*16, motor2_pid.Kd); row++;
        tft180_show_string(0, row*16, (menu_item==3)?"->Spd":"  Spd");
        tft180_show_int16(12*6, row*16, motor2_pid.Target); row++;
    }
    else if (menu_page == 2)
    {
        tft180_show_string(0, 0, " Steer PID");
        tft180_show_string(0, row*16, (menu_item==0)?"->Kp":"  Kp");
        tft180_show_int16(12*6, row*16, steer_pid.Kp); row++;
        tft180_show_string(0, row*16, (menu_item==1)?"->Ki":"  Ki");
        tft180_show_int16(12*6, row*16, steer_pid.Ki); row++;
        tft180_show_string(0, row*16, (menu_item==2)?"->Kd":"  Kd");
        tft180_show_int16(12*6, row*16, steer_pid.Kd); row++;
        tft180_show_string(0, row*16, (menu_item==3)?"->Max":"  Max");
        tft180_show_int16(12*6, row*16, steer_pid.OutMax); row++;
        tft180_show_string(0, row*16, (menu_item==4)?"->Min":"  Min");
        tft180_show_int16(12*6, row*16, steer_pid.OutMin); row++;
    }
    else if (menu_page == 3)
    {
        tft180_show_string(0, 0, " Base Speed");
        tft180_show_string(0, row*16, "->Spd");
        tft180_show_int16(12*6, row*16, motor1_pid.Target); row++;
    }
    else if (menu_page == 4)
    
    {
        tft180_show_string(0, 0, "  LAUNCH");
        tft180_show_string(0, row*16, " GO GO GO ");
    }
}

uint8 menu_launch(void)
{
    if (menu_page == 4 && key5_flag)
    {
        key5_flag = 0;
        return 1;
    }
    return 0;
}
