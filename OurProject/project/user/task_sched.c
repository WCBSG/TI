/* task_sched.c — 任务框架：任务 2/5/6 共用巡线引擎 + 任务 3 球往返
 * 任务2/5/6：红外巡线 + 可选 servo 球稳（5ms 中断自动追踪，目标像素）
 *   任务 2 停车（学长方案）：起步忽略 1s → 停车线连续 3 帧 → 立即停
 * 任务 3：球 O→+5cm→-5cm（像素域状态机，稳定 |球位-目标|<12px 持续 200ms）
 * 运行时零刷屏（省算力），结束结果页显示耗时 */

#include "task_sched.h"
#include "line_ctrl.h"
#include "servo.h"
#include "protocol.h"   /* proto_ball_x / proto_ball_valid（任务3 稳定判定） */
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "menu_defs.h"
#include "WcTFT180.h"

/* ── 全局状态 ── */
static TaskId current_task = TASK_IDLE;
static int    last_result  = TASK_RESULT_RUNNING;
static uint32 last_elapsed_ms = 0;   /* 最近一次任务耗时（结果页显示） */

/* 任务3 稳定判定（像素域，移植自舵机项目） */
#define T3_STABLE_ERR   12      /* 稳定判定：误差阈值（像素） */
#define T3_STABLE_MS    200     /* 稳定判定：持续时长（ms，g_servo_ms 计时） */

/* 球目标 cm(0.1cm) → 像素：offset = cm * px_per_cm / 10 */
static void ball_set_cm(int16 cm_x10)
{
    g_servo_target = (int16)(pixel_zero + ((int32)cm_x10 * px_per_cm) / 10);
}

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
 *   enable_ball=1 时球稳目标=ball_target（servo 5ms 中断自动追踪）
 * ════════════════════════════════════════════════════════════ */
static int line_drive_run(uint8 enable_ball, int16 ball_target)
{
    int s[8];
    uint8  stop_cd = 0;             /* 停车线连续确认计数 */
    uint8  stop_done = 0;           /* 停车完成标志 */
    uint32 start_tick, elapsed_ms;
    int    result = TASK_RESULT_OK;

    /* 任务 2/5/6 超时保护（整圈 30s，任务 2 限 20s 但留余量） */
    const uint32 line_timeout_ms = 30000UL;

    motor1_control(0);
    motor2_control(0);
    if (enable_ball) ball_set_cm(ball_target);   /* 球目标像素，servo 中断自动追踪 */

    /* 屏幕标题 */
    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, (enable_ball ? (ball_target ? "TASK 6" : "TASK 5") : "TASK 2"));

    EA = 0; start_tick = pit_tick; EA = 1;

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

        /* 巡线（球稳由 5ms TIM0 中断驱动，主循环不调 tick） */
        err = calc_error(s);
        line_ctrl_set(err, base_speed);

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
    if (enable_ball) Servo_PWM_Set((uint16)servo_center_duty);   /* 舵机回中 */

    /* 记录本次耗时（结果页显示） */
    EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
    last_elapsed_ms = elapsed_ms;

    return result;
}

/* ═══════════════════════════════════════════════════════════
 * 任务 3：球 O→+5cm→-5cm 往返（小车静止，servo 5ms 中断追踪，主循环管状态机）
 * ════════════════════════════════════════════════════════════ */
static int task3_run(void)
{
    uint32 start_tick, elapsed_ms;
    uint8  ph = 0;              /* 0=O, 1=+5cm, 2=-5cm, 3=完成 */
    uint32 stable_since = 0;    /* 连续稳定起始毫秒（g_servo_ms） */
    int16  tgt;

    ball_set_cm(0);             /* 目标 O（像素） */

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "TASK 3");

    EA = 0; start_tick = pit_tick; EA = 1;

    while (1)
    {
        /* 阶段目标（像素）：O → +5cm → -5cm */
        if (ph == 0)      tgt = pixel_zero;
        else if (ph == 1) tgt = (int16)(pixel_zero + 5 * px_per_cm);   /* +5.0cm */
        else              tgt = (int16)(pixel_zero - 5 * px_per_cm);   /* -5.0cm */
        g_servo_target = tgt;

        if (ph < 3)
        {
            int16 err = (int16)(proto_ball_x - tgt);
            int16 ae  = (err < 0) ? (int16)(-err) : err;

            /* 稳定判定：|球位-目标| < 12px 持续 200ms → 下一阶段 */
            if (proto_ball_valid && ae < T3_STABLE_ERR)
            {
                if (stable_since == 0) stable_since = g_servo_ms;
                if (g_servo_ms - stable_since >= T3_STABLE_MS)
                {
                    ph++;
                    stable_since = 0;
                }
            }
            else
            {
                stable_since = 0;
            }
        }
        else
        {
            /* 完成：舵机回中 */
            Servo_PWM_Set((uint16)servo_center_duty);
            EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
            last_elapsed_ms = elapsed_ms;
            return TASK_RESULT_OK;
        }

        system_delay_ms(10);

        /* 超时保护：15s */
        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
        if (elapsed_ms > 15000)
        {
            Servo_PWM_Set((uint16)servo_center_duty);
            last_elapsed_ms = elapsed_ms;
            return TASK_RESULT_TIMEOUT;
        }
    }
}

/* ═══════════════════════════════════════════════════════════
 * 结果页：显示任务号 + OK/FAILED/TIMEOUT + 耗时，Key4 返回菜单
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