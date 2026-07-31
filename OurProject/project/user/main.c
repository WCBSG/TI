#include "zf_common_headfile.h"
#include "WcTFT180.h"
#include "menu_defs.h"
#include "control.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "PID.h"
#include "KEY.h"
#include "config.h"


/* ═══════════════════════════════════════════════════════════ */
void main(void)
{
    int s[8];

    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    WcTFT_Init();

    IRPHOTO_Init();
    button_init();
    Motor_Init();
    encoder_init();
    motor1_pid_init();
    motor2_pid_init();
    steer_init();
    config_load();

    pit_ms_init(PIT_ENCODER, 5, pit_handler);

    /* ── 启动主菜单 ── */
    Menu_Init();
    Menu_Push(&page_main);

    /* ── 主循环：按键 → 菜单 ── */
    launch_triggered = 0;
    while (!launch_triggered)
    {
        button_control(KEY_REPEAT_KEY1 | KEY_REPEAT_KEY2);

        if (key1_flag) { key1_flag = 0; Menu_Inc();           }  /* Key1: 上/+ */
        if (key2_flag) { key2_flag = 0; Menu_Dec();           }  /* Key2: 下/- */
        if (key3_flag) { key3_flag = 0; Menu_Edit();          }  /* Key3: 确定/编辑 */
        if (key4_flag) { key4_flag = 0; Menu_Cancel();        }  /* Key4: 取消/返回 */

        if (key5_flag)
        {
            key5_flag = 0;

            /* 主菜单按 Key5 → 保存配置并进入 Launch */
            if (Menu_IsTop(&page_main))
            {
                config_save();
                Menu_Push(&page_launch);
            }
            else
            {
                Menu_Home(&page_main);
            }
        }

        Motor_DebugSync();  /* 调试页：占空比 → 电机；离开自动停止 */
        system_delay_ms(50);
    }

    /* ── 行驶阶段 ── */
    WcTFT_Clear(RGB565_WHITE);
    motor1_pid.Target = motor2_pid.Target;  /* 确保两轮同速 */
    driving = 1;

    while (1)
    {
        IRPHOTO_Read(s);
        IRPHOTO_Display(s);

        if (is_stop(s))
        {
            motor1_control(0);
            motor2_control(0);
            break;
        }

        steer_set(calc_error(s), motor1_pid.Target);
        system_delay_ms(10);
    }
}
