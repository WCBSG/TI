/*********************************************************************************************************************
* 文件名称          task_sched.c
* 说明              任务框架实现
*
* 通用巡线引擎 line_drive_run()：任务 2/5/6 共用
*   - 红外巡线（line_ctrl）+ 可选球稳（ball_ctrl）并行
*   - 脱轨连续 3 次全灭 → FAIL
*   - 停车线判定暂关闭（调试）
*
* 任务 3 球往返：ball_ctrl 单独运行，目标序列 O→+5→-5，到位判定 ±1cm 持续 100ms
********************************************************************************************************************/

#include "task_sched.h"
#include "line_ctrl.h"
#include "ball_ctrl.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "config.h"
#include "menu_defs.h"
#include "WcTFT180.h"
#include "imu_ctrl.h"    /* imu_ctrl_tick, imu_gyro_dps_x10, imu_yaw_deg */
#include "cmd_ctrl.h"    /* cmd_poll：结果页也响应串口命令 */

/* ── 调试开关 ──
 * 停车/脱轨判定已删（专注巡线调参），任务只靠 30s 超时停；停车逻辑后续恢复 */
#define DEBUG_LINE_FOREVER   1

/* ── 全局状态 ── */
static TaskId current_task = TASK_IDLE;
static int    last_result  = TASK_RESULT_RUNNING;

/* ═══════════════════════════════════════════════════════════
 * 通用巡线引擎：任务 2/5/6 共用
 *   enable_ball=1 时并行运行球稳环（目标 ball_target）
 *   task_fast_line=1（FAST 命令）：纯巡线，无显示/串口/IMU，控制频率最高
 *   task_fast_line=0（DBG 命令）：完整调试（IMU + TFT + USB 帧）
 * ════════════════════════════════════════════════════════════ */
uint8 task_fast_line = 1;   /* 高速巡线开关：1=纯巡线（默认），0=完整调试（FAST/DBG 命令切换） */

/* 停车线检测：A 点垂直启停线（横跨跑道）→ ≥3 路连续红外同时亮（00000111）
 * 简化方案（学长参考）：检测到即停车，不做倒车往返/yaw 判定 */
static uint8 line_is_stop(const int s[8])
{
    uint8 i, run = 0, max_run = 0;
    for (i = 0; i < 8; i++)
    {
        if (s[i]) { run++; if (run > max_run) max_run = run; }
        else run = 0;
    }
    return (max_run >= 3);
}

static int line_drive_run(uint8 enable_ball, int16 ball_target)
{
    int s[8];
    uint8  display_cd = 0;
    uint8  stop_cd = 0;             /* 停车线连续确认计数 */
    uint8  stop_done = 0;           /* 停车完成标志 */
    uint32 start_tick, last_imu_tick, last_ball_tick, elapsed_ms;
    uint16 sec, tenth, imu_dt;
    int    result = TASK_RESULT_OK;

    /* 任务 2/5/6 超时保护（整圈 30s，任务 2 限 20s 但留余量） */
    const uint32 line_timeout_ms = 30000UL;

    motor1_control(0);
    motor2_control(0);
    if (enable_ball) ball_ctrl_set_target(ball_target);

    if (!task_fast_line)   /* 调试模式显示任务标题 */
    {
        WcTFT_Clear(RGB565_BLACK);
        WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
        WcTFT_PrintCenter(0, (enable_ball ? (ball_target ? "TASK 6" : "TASK 5") : "TASK 2"));
    }

    EA = 0; start_tick = pit_tick; last_imu_tick = pit_tick; last_ball_tick = pit_tick; EA = 1;

    while (1)
    {
        int   err;

        if (!task_fast_line)   /* 调试模式才采样 IMU（停车已不用 yaw，高速巡线不采样） */
        {
            EA = 0; imu_dt = (uint16)((pit_tick - last_imu_tick) * 5); last_imu_tick = pit_tick; EA = 1;
            imu_ctrl_tick(imu_dt);
        }

        IRPHOTO_Read(s);

        /* ═══ 任务 2 停车（简化，学长方案）：起步忽略 3s → 检测到停车线连续 3 帧 → 立即停 ═══
         * 任务 5/6（球稳）不停车，靠超时退出。 */
        if (!enable_ball)
        {
            uint8 is_st = line_is_stop(s);
            EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;

            if (elapsed_ms < 1000)      /* 起步忽略 车从 A 点压线出发 */
            {
                stop_cd = 0;
            }
            else if (is_st)             /* 检测到停车线（≥3 路连续亮）× 连续 3 帧 → 立即停 */
            {
                if (++stop_cd >= 3) { result = TASK_RESULT_OK; stop_done = 1; }
            }
            else stop_cd = 0;
        }

        /* 巡线 + 可选球稳 */
        err = calc_error(s);
        line_ctrl_set(err, base_speed);
        if (enable_ball)
        {
            uint32 now_tick;
            EA = 0; now_tick = pit_tick; EA = 1;
            if ((now_tick - last_ball_tick) >= 2)
            {
                last_ball_tick = now_tick;
                ball_ctrl_tick();
            }
        }

        if (!task_fast_line)   /* 调试模式：每 100ms 显示 + USB 串口帧 */
        {
            if (++display_cd >= 10)
            {
                display_cd = 0;
                EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
                sec   = (uint16)(elapsed_ms / 1000);
                tenth = (uint16)((elapsed_ms % 1000) / 100);

                /* ═══ TFT 屏幕显示已删（占性能），调试数据全走 USB-CDC 串口帧 ═══ */

                /* USB-CDC 串口调试帧（一行一帧，空格分隔，便于上位机解析） */
                {
                    char dbg[192];
                    uint32 n = 0;
                    char ir_bits[9];
                    uint8 i;
                    /* %d 参数必须 (int32) 转换：zf_sprintf 按 int32 读变参，int16 负数会读成 65535 */
                    for (i = 0; i < 8; i++) ir_bits[i] = s[i] ? '1' : '0';
                    ir_bits[8] = '\0';
                    n += zf_sprintf((int8 *)(dbg + n), "T=%d.%d ", (int32)sec, (int32)tenth);
                    n += zf_sprintf((int8 *)(dbg + n), "E=%d ", (int32)err);
                    n += zf_sprintf((int8 *)(dbg + n), "IR=%s ", ir_bits);
                    n += zf_sprintf((int8 *)(dbg + n), "D1=%d ", (int32)line_duty_lr);
                    n += zf_sprintf((int8 *)(dbg + n), "D2=%d ", (int32)line_duty_rr);
                    n += zf_sprintf((int8 *)(dbg + n), "SO=%d ", (int32)line_turn);
                    n += zf_sprintf((int8 *)(dbg + n), "E1=%d ", (int32)motor_get_encoder_lr());
                    n += zf_sprintf((int8 *)(dbg + n), "E2=%d ", (int32)motor_get_encoder_rr());
                    n += zf_sprintf((int8 *)(dbg + n), "BASE=%d ", (int32)base_speed);
                    n += zf_sprintf((int8 *)(dbg + n), "YAW=%d ", (int32)(imu_yaw_x100 / 100));   /* 航向度 */
                    n += zf_sprintf((int8 *)(dbg + n), "GZ=%d ", (int32)imu_gyro_dps_x10);       /* 0.1°/s */
                    if (enable_ball)
                    {
                        n += zf_sprintf((int8 *)(dbg + n), "B=%d ", (int32)ball_cm_x10);
                        n += zf_sprintf((int8 *)(dbg + n), "TGT=%d ", (int32)ball_pid.Target);
                        n += zf_sprintf((int8 *)(dbg + n), "PX=%d ", (int32)proto_ball_x);
                        n += zf_sprintf((int8 *)(dbg + n), "V=%d ", (int32)proto_ball_valid);
                        n += zf_sprintf((int8 *)(dbg + n), "BF=%d ", (int32)ball_feedback_fresh);
                        n += zf_sprintf((int8 *)(dbg + n), "SD=%d\n", (int32)ball_duty_out);
                    }
                    else
                    {
                        n += zf_sprintf((int8 *)(dbg + n), "\n");
                    }
                    usb_cdc_write_buffer((const uint8 *)dbg, (uint16)n);
                }
            }
        }

        /* 任务 2 停车完成：退出 */
        if (stop_done) break;

        /* 超时保护：整圈 30s 兜底（任务 2 限 20s 留余量；5/6 同限） */
        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
        if (elapsed_ms > line_timeout_ms)
        {
            result = TASK_RESULT_TIMEOUT;
            break;
        }
    }

    motor1_control(0);
    motor2_control(0);
    if (enable_ball) ball_ctrl_stop();

    return result;
}

/* ═══════════════════════════════════════════════════════════
 * 任务 3：球 O→+5cm→-5cm 往返（小车静止，纯 ball_ctrl）
 * ════════════════════════════════════════════════════════════ */
typedef enum { S3_HOLD, S3_TO_POS, S3_TO_NEG } Phase3;

static int task3_run(void)
{
    uint32 start_tick, elapsed_ms;
    uint16 sec, tenth;
    uint8  hold_cnt = 0, settle_cnt = 0, display_cd = 0;
    Phase3 ph = S3_HOLD;

    ball_ctrl_set_target(0);

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "TASK 3");

    EA = 0; start_tick = pit_tick; EA = 1;

    while (1)
    {
        ball_ctrl_tick();

        switch (ph)
        {
            case S3_HOLD:                          /* 球稳 O，起步 0.5s */
                if (++hold_cnt >= 50)
                {
                    ball_ctrl_set_target(50);      /* 目标 +5.0cm */
                    settle_cnt = 0;
                    ph = S3_TO_POS;
                }
                break;

            case S3_TO_POS:                        /* 球 → +5cm，±1cm 稳定后切 -5cm */
                if (!ball_feedback_fresh)
                {
                    settle_cnt = 0;
                }
                else if (ball_feedback_updated && ball_cm_x10 >= 40 && ball_cm_x10 <= 60)
                {
                    if (++settle_cnt >= 3)         /* 连续 3 个视觉帧，约 100~150ms */
                    {
                        ball_ctrl_set_target(-50);
                        settle_cnt = 0;
                        ph = S3_TO_NEG;
                    }
                }
                else if (ball_feedback_updated)
                {
                    settle_cnt = 0;
                }
                break;

            case S3_TO_NEG:                        /* 球 → -5cm，稳定即成功 */
                if (!ball_feedback_fresh)
                {
                    settle_cnt = 0;
                }
                else if (ball_feedback_updated && ball_cm_x10 <= -40 && ball_cm_x10 >= -60)
                {
                    if (++settle_cnt >= 3)
                    {
                        ball_ctrl_stop();
                        return TASK_RESULT_OK;
                    }
                }
                else if (ball_feedback_updated)
                {
                    settle_cnt = 0;
                }
                break;
        }

        system_delay_ms(10);
        if (++display_cd >= 10)
        {
            display_cd = 0;
            EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
            sec   = (uint16)(elapsed_ms / 1000);
            tenth = (uint16)((elapsed_ms % 1000) / 100);

            WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
            /* 每行一个参数 */
            WcTFT_PrintAt(0, 16, "T:");
            WcTFT_PrintIntAt(24, 16, (int32)sec);
            WcTFT_PrintAt(48, 16, ".");
            WcTFT_PrintIntAt(56, 16, (int32)tenth);
            WcTFT_PrintAt(96, 16, "s");
            WcTFT_PrintAt(0, 32, "B:");
            WcTFT_PrintFloatAt(24, 32, (double)ball_cm_x10 / 10.0, 1);
            WcTFT_PrintAt(0, 48, "Tgt:");
            WcTFT_PrintFloatAt(40, 48, (double)ball_pid.Target / 10.0, 1);
            WcTFT_PrintAt(0, 64, "Px:");
            WcTFT_PrintIntAt(24, 64, (int32)proto_ball_x);
            WcTFT_PrintAt(0, 80, "V:");
            WcTFT_PrintIntAt(24, 80, (int32)proto_ball_valid);
            WcTFT_PrintAt(0, 96, "D:");
            WcTFT_PrintIntAt(24, 96, (int32)ball_duty_out);

            /* USB-CDC 串口调试帧（一行一帧） */
            {
                char dbg[128];
                uint32 n = 0;
                n += zf_sprintf((int8 *)(dbg + n), "T=%d.%d ", (int32)sec, (int32)tenth);
                n += zf_sprintf((int8 *)(dbg + n), "B=%d ", (int32)ball_cm_x10);
                n += zf_sprintf((int8 *)(dbg + n), "TGT=%d ", (int32)ball_pid.Target);
                n += zf_sprintf((int8 *)(dbg + n), "PX=%d ", (int32)proto_ball_x);
                n += zf_sprintf((int8 *)(dbg + n), "V=%d ", (int32)proto_ball_valid);
                n += zf_sprintf((int8 *)(dbg + n), "BF=%d ", (int32)ball_feedback_fresh);
                n += zf_sprintf((int8 *)(dbg + n), "SD=%d\n", (int32)ball_duty_out);
                usb_cdc_write_buffer((const uint8 *)dbg, (uint16)n);
            }
        }

        /* 超时保护：15s */
        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
        if (elapsed_ms > 15000)
        {
            ball_ctrl_stop();
            return TASK_RESULT_TIMEOUT;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 * 结果页：显示任务号 + OK/FAILED/TIMEOUT，Key4 返回菜单
 * ════════════════════════════════════════════════════════════ */
static void task_sched_show_result(int result)
{
    uint16 color = (result == TASK_RESULT_OK) ? RGB565_GREEN : RGB565_RED;

    WcTFT_Clear(color);
    WcTFT_SetColor(RGB565_WHITE, color);

    switch (current_task)
    {
        case TASK_2: WcTFT_PrintCenter(1, "TASK 2"); break;
        case TASK_3: WcTFT_PrintCenter(1, "TASK 3"); break;
        case TASK_5: WcTFT_PrintCenter(1, "TASK 5"); break;
        case TASK_6: WcTFT_PrintCenter(1, "TASK 6"); break;
        default: break;
    }

    if (result == TASK_RESULT_OK)
        WcTFT_PrintCenter(3, "OK");
    else if (result == TASK_RESULT_TIMEOUT)
        WcTFT_PrintCenter(3, "TIMEOUT");
    else
        WcTFT_PrintCenter(3, "FAILED");

    WcTFT_PrintCenter(6, "Key4: Back");

    /* 等待 Key4 返回菜单；同时响应串口命令（T2 等可立即重跑，无需按键） */
    while (1)
    {
        button_control(0);
        cmd_poll();                      /* 结果页也处理 USB-CDC 命令 */
        if (key4_flag) { key4_flag = 0; break; }
        system_delay_ms(10);
    }
}

/* ═══════════════════════════════════════════════════════════
 * 公共接口
 * ════════════════════════════════════════════════════════════ */

void task_sched_set(int task_id)
{
    if (task_id >= TASK_IDLE && task_id <= TASK_6)
        current_task = (TaskId)task_id;
}

int task_sched_get(void)
{
    return (int)current_task;
}

int task_sched_result(void)
{
    return last_result;
}

void task_sched_run(void)
{
    int result;

    /* 任务 2 用更快巡线参数；其他任务（3/5/6）用球稳参数（菜单 Steer T2 / Steer 5/6 分别调） */
    if (current_task == TASK_2) { line_ctrl_apply_t2(); base_speed = base_speed_t2; }
    else { line_ctrl_apply_other(); base_speed = base_speed_ot; }

    switch (current_task)
    {
        case TASK_2: result = line_drive_run(0, 0);                   break;
        case TASK_5: result = line_drive_run(1, 0);                   break;
        case TASK_6: result = line_drive_run(1, ball_target_cm_x10);  break;
        case TASK_3: result = task3_run();                            break;
        default:     result = TASK_RESULT_FAIL;                       break;
    }

    last_result = result;
    task_sched_show_result(result);
}
