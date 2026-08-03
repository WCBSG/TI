/*********************************************************************************************************************
* 文件名称          main.c
* 说明              主程序 — 菜单待机 ⇄ 任务运行 ⇄ 结果回菜单（三态循环）
*
*   [菜单阶段]  按键导航菜单，Protocol 页调试球坐标
*   [任务阶段]  Launch 页点选任务 → task_sched_run() 阻塞运行 → 结果页
*   [回菜单]    结果页按 Key4 返回主菜单，可重新选任务
*
* 比赛清理（2026-08）：已删 IR/PROTO/UTEST/HOLD 测试函数、IMU/编码器/LED
* 调试外设、cmd_ctrl 串口命令模块（现场不允许连电脑）；保留菜单调参（Steer/Ball PID）。
********************************************************************************************************************/

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

    Protocol_Init();
    Protocol_Start();   /* 启动 UART3 接收：中断自动维护球位置（菜单/任务都能读） */

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
        /* ── 菜单阶段：按键导航 + USB-CDC 命令，等待 Launch 选任务 ── */
        while (!launch_triggered)
        {
            button_control(KEY_REPEAT_KEY1 | KEY_REPEAT_KEY2);

            if (key1_flag) { key1_flag = 0; Menu_Inc();           }  /* b2: 上/+ */
            if (key2_flag) { key2_flag = 0; Menu_Dec();           }  /* b3: 下/- */
            if (key3_flag) { key3_flag = 0; Menu_Edit();          }  /* b4: 确定/编辑 */

            if (key4_flag)                                       /* P3.2: 返回 */
            {
                key4_flag = 0;
                if (Menu_IsTop(&page_main))
                {
                    Menu_Push(&page_launch);    /* 主菜单按返回 → 进 Launch 选任务 */
                }
                else
                {
                    Menu_Cancel();              /* 子页按返回 → 回上层 */
                }
            }

            Protocol_ReadBall(&proto_ball_x);  /* 菜单阶段消费球坐标（调试） */

            system_delay_ms(10);
        }

        /* ── 任务阶段：阻塞运行所选任务（含结果页），Key4 返回 ── */
        task_sched_run();

        /* ── 任务结束 ── */
        if (launch_triggered)
        {
            launch_triggered = 0;      /* 清除，让循环重跑新任务 */
            continue;                  /* 不 Menu_Push 菜单，直接进下一轮 task_sched_run */
        }
        launch_triggered = 0;
        Menu_Push(&page_main);
    }
}
