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

/* ── LED 灯带 PWM（PA2 = PWME_CH2，300Hz 与舵机同 PWME 组同频） ── */
#define LED_PWM_PIN    (IO_PA2)
#define LED_PWM_FREQ   300


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

    /* LED 灯带 PWM（PA2，300Hz 与舵机同频），亮度由 LED 调试页调节 */
    //pwm_init(LED_PWM_PIN, LED_PWM_FREQ, 0);
    gpio_init(LED_PWM_PIN, GPO, 1, GPO_PUSH_PULL);
    pit_ms_init(PIT_ENCODER, 5, pit_handler);

    /* ── 启动主菜单 ── */
    Menu_Init();
    Menu_Push(&page_main);

    /* ── 三态主循环：菜单 ⇄ 任务 ⇄ 结果 ── */
    launch_triggered = 0;
    while (1)
    {
        /* ── 菜单阶段：按键导航，等待 Launch 选任务 ── */
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
                    config_save();              /* 主菜单按返回 → 保存配置并进 Launch */
                    Menu_Push(&page_launch);
                }
                else
                {
                    Menu_Cancel();              /* 子页按返回 → 回上层 */
                }
            }

            Protocol_ReadBall(&proto_ball_x);  /* 菜单阶段消费球坐标（调试） */

            /* 灯带亮度实时写入 PA2 PWM（LED 调试页调节） */
            pwm_set_duty(LED_PWM_PIN, (uint32)led_duty);

            system_delay_ms(10);
        }

        /* ── 任务阶段：阻塞运行所选任务（含结果页），Key4 返回 ── */
        task_sched_run();

        /* ── 回菜单，可重新选择/重跑 ── */
        launch_triggered = 0;
        Menu_Push(&page_main);
    }
}
