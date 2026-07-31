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
        system_delay_ms(10);
    }

    /* ═══════════════════════════════════════════════════════════
     * 行驶阶段：任务 2 — 纯巡线一圈（无球控制）
     *
     * 阶段：
     *   1. 计时巡线（IR 加权偏差 → steer PID → 差速转向）
     *   2. 停车 + 显示结果
     * ════════════════════════════════════════════════════════════ */

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "TASK 2");

    {
        int16 base_speed = motor1_pid.Target;  /* 绝对基速（正值，从菜单继承） */

        /* 左后轮（motor1）反向安装：负 Target → 前进 */
        motor1_pid.Target = -base_speed;
        motor2_pid.Target = base_speed;
    }
    driving = 1;

    {
        int16  base_speed = motor2_pid.Target;  /* 绝对基速 */
        uint16 tick       = 0;  /* 10ms 滴答计数器 */
        uint8  off_track  = 0;  /* 1=冲出赛道失败 */
        uint8  display_cd = 0;  /* 显示倒计时（每 10 tick = 100ms 刷新） */

        /* ── 阶段 1：盲开逃逸启停线（调试时注释掉） ── */
        //while (1)
        //{
        //    IRPHOTO_Read(s);
        //    if (is_stop(s) == 0) break;  /* 完全离开启停线 */
        //    system_delay_ms(10);
        //    tick++;
        //}

        /* ── 计时巡线 ── */
        while (1)
        {
            IRPHOTO_Read(s);

            {
                uint8 reason = is_stop(s);
                //if (reason == 2)      { off_track = 1; break; }  /* 冲出赛道 */
                //if (reason == 1)      { off_track = 0; break; }  /* 正常停车 */
                (void)reason;  /* 暂不使用 */
            }

            steer_set(calc_error(s), base_speed);
            system_delay_ms(10);
            tick++;
            display_cd++;

            if (display_cd >= 10)  /* 每 100ms 刷新显示 */
            {
                display_cd = 0;
                {
                    uint16 sec   = tick / 100;
                    uint16 tenth = (tick % 100) / 10;
                    int8   err   = (int8)calc_error(s);

                    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
                    WcTFT_PrintAt(0, 32, "T:    . s  E:     ");
                    WcTFT_PrintIntAt(16, 32, (int32)sec);
                    WcTFT_PrintAt(24, 32, ".");
                    WcTFT_PrintIntAt(32, 32, (int32)tenth);
                    WcTFT_PrintIntAt(80, 32, (int32)err);
                }
            }
        }

        /* ── 阶段 1：停电机 ── */
        motor1_control(0);
        motor2_control(0);
        driving = 0;

        /* ── 阶段 2：显示结果 ── */
        if (off_track)
        {
            WcTFT_Clear(RGB565_RED);
            WcTFT_SetColor(RGB565_WHITE, RGB565_RED);
            WcTFT_PrintCenter(3, "OFF TRACK");
            WcTFT_PrintCenter(4, "FAILED");
        }
        else
        {
            uint16 sec   = tick / 100;
            uint16 tenth = (tick % 100) / 10;

            WcTFT_Clear(RGB565_GREEN);
            WcTFT_SetColor(RGB565_WHITE, RGB565_GREEN);
            WcTFT_PrintCenter(2, "TASK 2 OK");

            WcTFT_PrintAt(24, 64, "T:");
            WcTFT_PrintIntAt(40, 64, (int32)sec);
            WcTFT_PrintAt(48, 64, ".");
            WcTFT_PrintIntAt(56, 64, (int32)tenth);
            WcTFT_PrintAt(64, 64, " s");
        }
    }

    /* 永久停车 */
    while (1);
}
