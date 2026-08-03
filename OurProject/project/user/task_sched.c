/*********************************************************************************************************************
* 文件名称          task_sched.c
* 说明              任务框架实现
*
* 通用巡线引擎 line_drive_run()：任务 2/5/6 共用
*   - 红外巡线（line_ctrl）+ 可选球稳（ball_ctrl）并行
*   - 任务 2 停车（学长方案）：起步忽略 1s → 停车线连续 3 帧 → 立即停
*
* 任务 3 球往返：ball_ctrl 单独运行，目标序列 O→+5→-5，到位判定 ±1cm 持续 100ms
*
* 比赛清理（2026-08）：删 IMU 采样 / USB 调试帧 / task_fast_line 开关；
* 运行时零刷屏（省算力），结束结果页显示耗时。
********************************************************************************************************************/

#include "task_sched.h"
#include "line_ctrl.h"
#include "ball_ctrl.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "menu_defs.h"
#include "WcTFT180.h"

/* ── 全局状态 ── */
static TaskId current_task = TASK_IDLE;
static int    last_result  = TASK_RESULT_RUNNING;
static uint32 last_elapsed_ms = 0;   /* 最近一次任务耗时（结果页显示） */

/* ═══════════════════════════════════════════════════════════
 * 停车线检测：A 点垂直启停线（横跨跑道）→ ≥3 路连续红外同时亮（00000111）
 * 简化方案（学长参考）：检测到即停车，不做倒车往返/yaw 判定
 * ════════════════════════════════════════════════════════════ */
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

/* ═══════════════════════════════════════════════════════════
 * 通用巡线引擎：任务 2/5/6 共用
 *   enable_ball=1 时并行运行球稳环（目标 ball_target）
 *   每 100ms 屏幕显示计时（+球位），比赛无上位机全走屏幕
 * ════════════════════════════════════════════════════════════ */
static int line_drive_run(uint8 enable_ball, int16 ball_target)
{
    int s[8];
    uint8  stop_cd = 0;             /* 停车线连续确认计数 */
    uint8  stop_done = 0;           /* 停车完成标志 */
    uint32 start_tick, last_ball_tick, elapsed_ms;
    int    result = TASK_RESULT_OK;

    /* 任务 2/5/6 超时保护（整圈 30s，任务 2 限 20s 但留余量） */
    const uint32 line_timeout_ms = 30000UL;

    motor1_control(0);
    motor2_control(0);
    if (enable_ball) ball_ctrl_set_target(ball_target);

    /* 屏幕标题 */
    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, (enable_ball ? (ball_target ? "TASK 6" : "TASK 5") : "TASK 2"));

    EA = 0; start_tick = pit_tick; last_ball_tick = pit_tick; EA = 1;

    while (1)
    {
        int   err;

        IRPHOTO_Read(s);

        /* ═══ 任务 2 停车（学长方案）：起步忽略 1s → 检测到停车线连续 3 帧 → 立即停 ═══
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

        /* 任务 2 停车完成：退出 */
        if (stop_done) break;

        /* 超时保护：整圈 30s 兜底 */
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

    /* 记录本次耗时（结果页显示） */
    EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
    last_elapsed_ms = elapsed_ms;

    return result;
}

/* ═══════════════════════════════════════════════════════════
 * 任务 3：球 O→+5cm→-5cm 往返（小车静止，纯 ball_ctrl）
 * ════════════════════════════════════════════════════════════ */
typedef enum { S3_HOLD, S3_TO_POS, S3_TO_NEG } Phase3;

static int task3_run(void)
{
    uint32 start_tick, elapsed_ms;
    uint8  hold_cnt = 0, settle_cnt = 0;
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
                        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
                        last_elapsed_ms = elapsed_ms;
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

        /* 超时保护：15s */
        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
        if (elapsed_ms > 15000)
        {
            ball_ctrl_stop();
            last_elapsed_ms = elapsed_ms;
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

    /* 结果页显示本次耗时（秒.十分之一） */
    {
        uint16 ms = (uint16)(last_elapsed_ms % 1000);
        uint16 s  = (uint16)(last_elapsed_ms / 1000);
        WcTFT_PrintAt(0, 80, "Time:");
        WcTFT_PrintIntAt(48, 80, (int32)s);
        WcTFT_PrintAt(64, 80, ".");
        WcTFT_PrintIntAt(72, 80, (int32)(ms / 100));
        WcTFT_PrintAt(88, 80, "s");
    }

    WcTFT_PrintCenter(6, "Key4: Back");

    /* 等待 Key4 返回菜单 */
    while (1)
    {
        button_control(0);
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
