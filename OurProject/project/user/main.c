#include "zf_common_headfile.h"
#include "WcTFT180.h"
#include "menu_defs.h"
#include "control.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "config.h"

#include "protocol.h"


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

    Protocol_Init();

    /* 配置加载必须先于任何消费 flash_buff 的初始化，否则持久化参数不生效 */
    config_load();
    steer_init();
    if (config_valid())
        base_speed = flash_buff[5];   /* 恢复持久化的基准 duty */

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

        Protocol_ReadBall(&proto_ball_x);  /* 消费最新球 X 坐标，更新 proto_ball_valid */

        system_delay_ms(10);
    }

    /* ═══════════════════════════════════════════════════════════
     * 行驶阶段 — 状态机：
     *   FOLLOW   → 巡线（弯道自适应减速）
     *   HALT     → 脱轨/停车 → 刹车 → 显示结果
     *   DONE     → 永久停车
     * ════════════════════════════════════════════════════════════ */

    {
        typedef enum { PHASE_FOLLOW, PHASE_HALT, PHASE_DONE } DrivePhase;

        uint32 start_tick;
        uint16 sec, tenth;
        uint8  display_cd = 0;
        uint8  off_track  = 0;  /* 1=脱轨失败 */
        uint8  off_count  = 0;  /* 脱轨去抖计数（连续全灭次数） */
        DrivePhase phase  = PHASE_FOLLOW;

        WcTFT_Clear(RGB565_BLACK);
        WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
        WcTFT_PrintCenter(0, "TASK 2");

        /* 差速开环发车：首个 steer_set 即输出基准 duty */
        EA = 0; start_tick = pit_tick; EA = 1;

        while (phase != PHASE_DONE)
        {
            IRPHOTO_Read(s);

            if (phase == PHASE_FOLLOW)
            {
                uint8 reason = is_stop(s);
                int   err    = calc_error(s);
                int   abs_err = (err < 0) ? -err : err;
                int16 spd;

                /* 脱轨：连续 3 次采样（≈30ms）全灭才确认，去抖防误判 */
                if (reason == 2)
                {
                    if (++off_count >= 3)
                    {
                        off_track = 1;
                        phase     = PHASE_HALT;
                    }
                }
                else
                {
                    off_count = 0;
                }
                /* 停车线到达（调试：关闭） */
                //if (reason == 1 && tick > 100) phase = PHASE_HALT;

                if (phase == PHASE_FOLLOW)
                {
                    /* 弯道自适应减速：|error|>4→70%, >2→85%, else 100%（int32 防溢出） */
                    if      (abs_err <= 2) spd = base_speed;
                    else if (abs_err <= 4) spd = (int16)((int32)base_speed * 85 / 100);
                    else                   spd = (int16)((int32)base_speed * 70 / 100);

                    steer_set(err, spd);
                }
            }

            if (phase == PHASE_HALT)
            {
                motor1_control(0);
                motor2_control(0);
                phase = PHASE_DONE;
            }

            system_delay_ms(10);
            display_cd++;

            /* 每 100ms 刷新显示（硬件时基计时） */
            if (display_cd >= 10 && phase == PHASE_FOLLOW)
            {
                uint32 elapsed_ms;

                display_cd = 0;
                EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
                sec   = (uint16)(elapsed_ms / 1000);
                tenth = (uint16)((elapsed_ms % 1000) / 100);

                WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
                WcTFT_PrintAt(0, 32, "T:    .  s  E:     ");
                WcTFT_PrintIntAt(16, 32, (int32)sec);
                WcTFT_PrintAt(24, 32, ".");
                WcTFT_PrintIntAt(32, 32, (int32)tenth);
                WcTFT_PrintIntAt(80, 32, (int32)calc_error(s));
                /* 编码器诊断（实际轮速） */
                WcTFT_PrintAt(0, 48, "E1:");
                WcTFT_PrintIntAt(24, 48, (int32)motor_get_encoder_lr());
                WcTFT_PrintAt(64, 48, "E2:");
                WcTFT_PrintIntAt(88, 48, (int32)motor_get_encoder_rr());
            }
        }

        /* ── 显示结果 ── */
        {
            uint32 elapsed_ms;
            uint16 color = off_track ? RGB565_RED : RGB565_GREEN;

            EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
            sec   = (uint16)(elapsed_ms / 1000);
            tenth = (uint16)((elapsed_ms % 1000) / 100);

            WcTFT_Clear(color);
            WcTFT_SetColor(RGB565_WHITE, color);
            if (off_track)
            {
                WcTFT_PrintCenter(3, "OFF TRACK");
                WcTFT_PrintCenter(4, "FAILED");
            }
            else
            {
                WcTFT_PrintCenter(2, "TASK 2 OK");
            }
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
