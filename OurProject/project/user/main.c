/* main.c — 主程序：菜单待机 ⇄ 任务运行 ⇄ 结果回菜单（三态循环）
 * 菜单按键导航调参；Launch 选任务启动；结果页 Key4 回主菜单 */

#include "zf_common_headfile.h"
#include "WcTFT180.h"
#include "menu_defs.h"
#include "line_ctrl.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "protocol.h"
#include "task_sched.h"
#include "servo.h"

void main(void)
{
    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    WcTFT_Init();

    IRPHOTO_Init();
    button_init();
    Motor_Init();

    Protocol_Init();    /* UART3 硬件（不使能接收；由 Servo_Enable/Control_Init 控制 Start/Stop） */

    line_ctrl_init();
    Servo_Init();
    Servo_Control_Init();
    Servo_Timer_Init();       /* 5ms 中断：像素域 PID+前馈 控舵机（TIM0） */
    /* base_speed 由 task_sched_run 按任务应用（base_speed_t2 / base_speed_ot） */

    pit_ms_init(PIT_ENCODER, 5, pit_handler);   /* 5ms 硬件时基（计时，TM1） */

    /* ── 启动主菜单 ── */
    Menu_Init();
    Menu_Push(&page_main);

    /* ── 三态主循环：菜单 ⇄ 任务 ⇄ 结果 ── */
    launch_triggered = 0;
    while (1)
    {
        /* ── 菜单阶段：按键导航，等待 Launch 选任务 ── */
        {
            uint8 refresh_cd = 0;    /* 每 100ms 刷新菜单（Ball Cal 页看实时球位） */
            while (!launch_triggered)
            {
                button_control(KEY_REPEAT_KEY1 | KEY_REPEAT_KEY2);
                if (key1_flag) { key1_flag = 0; Menu_Inc(); }
                if (key2_flag) { key2_flag = 0; Menu_Dec(); }
                if (key3_flag) { key3_flag = 0; Menu_Edit(); }
                if (key4_flag)             /* P3.2: 返回/取消 */
                {
                    key4_flag = 0;
                    if (Menu_IsTop(&page_main)) Menu_Push(&page_launch);   /* 主菜单 → Launch */
                    else                        Menu_Cancel();             /* 子页 → 回上层 */
                }

                /* BallCal 页使能 servo（标定球位稳定，含 Protocol_Start），离开禁用回中 */
                if (Menu_IsTop(&page_ballpid)) Servo_Enable();
                else if (servo_enable) Servo_Control_Init();

                /* 100ms：刷新 BallPx 显示副本 + 重绘菜单 */
                if (++refresh_cd >= 10) { refresh_cd = 0; ball_px_display = proto_ball_x; Menu_Draw(); }
                system_delay_ms(10);
            }
        }

        /* ── 任务阶段：阻塞运行所选任务（含结果页），Key4 返回 ── */
        task_sched_run();

        /* ── 任务结束：清标志回主菜单（结果页 Key4 已返回） ── */
        launch_triggered = 0;
        Menu_Push(&page_main);
    }
}
