/*********************************************************************************************************************
* 文件名称          main.c
* 说明              主程序 — 菜单待机 ⇄ 任务运行 ⇄ 结果回菜单（三态循环）
*
*   [菜单阶段]  按键导航菜单，Protocol 页调试球坐标
*   [任务阶段]  Launch 页点选任务 → task_sched_run() 阻塞运行 → 结果页
*   [回菜单]    结果页按 Key4 返回主菜单，可重新选任务
********************************************************************************************************************/

#include "zf_common_headfile.h"
#include "WcTFT180.h"
#include "menu_defs.h"
#include "line_ctrl.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "config.h"
#include "protocol.h"
#include "task_sched.h"
#include "ball_ctrl.h"
#include "imu_ctrl.h"


/* ═══════════════════════════════════════════════════════════ */
void main(void)
{
    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    WcTFT_Init();

    IRPHOTO_Init();
    button_init();
    Motor_Init();
    encoder_init();

    Protocol_Init();

    /* 配置加载必须先于任何消费 flash_buff 的初始化，否则持久化参数不生效 */
    config_load();
    line_ctrl_init();
    ball_ctrl_init();
    if (config_valid())
        base_speed = flash_buff[5];   /* 恢复持久化的基准 duty */

    imu_ctrl_init();   /* 陀螺仪：硬件 SPI3 + 静止 1s 零点标定（失败不阻塞） */

    pit_ms_init(PIT_ENCODER, 5, pit_handler);

    /* ── 启动主菜单 ── */
    Menu_Init();
    Menu_Push(&page_main);

    /* ── 三态主循环：菜单 ⇄ 任务 ⇄ 结果 ── */
    launch_triggered = 0;
    while (1)
    {
        uint8 ball_dbg_cd = 0;

        /* ── 菜单阶段：按键导航 + 球坐标串口输出，等待 Launch 选任务 ── */
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

            Protocol_ReadBall(&proto_ball_x);  /* 菜单阶段消费球坐标（调试） */

            /* 每 100ms 输出 OpenART 球坐标到 USB-CDC（验证通信 + 调试球稳） */
            if (++ball_dbg_cd >= 10)
            {
                ball_dbg_cd = 0;
                {
                    char dbg[64];
                    uint32 n = 0;
                    n += zf_sprintf((int8 *)(dbg + n), "PX=%d V=%d\n",
                                    (int32)proto_ball_x, (int32)proto_ball_valid);
                    usb_cdc_write_buffer((const uint8 *)dbg, (uint16)n);
                }
            }

            system_delay_ms(10);
        }

        /* ── 任务阶段：阻塞运行所选任务（含结果页），Key4 返回 ── */
        task_sched_run();

        /* ── 回菜单，可重新选择/重跑 ── */
        launch_triggered = 0;
        Menu_Push(&page_main);
    }
}
