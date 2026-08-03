/* task_sched.c — 任务框架：任务 2/5/6 共用巡线引擎 + 任务 3 球往返
 * 任务2/5/6：红外巡线 + 可选 servo 球稳（5ms 中断自动追踪，目标像素）
 *   停车线检测（任务2/5/6 共用）：起步忽略 1s → 连续 3 帧确认
 *   任务 2 立即停；任务 5/6 缓停（球稳保持，电机 500ms 线性减速）
 * 任务 3：球 O→+5cm→-5cm（像素域状态机，稳定 |球位-目标|<12px 持续 200ms）
 * 运行时零刷屏 + 1Hz 时钟，结束结果页显示耗时 */
#include "task_sched.h"
#include "line_ctrl.h"
#include "servo.h"
#include "imu_ctrl.h"   /* 任务 2 停车辅助：yaw 确认走完一圈 */
#include "protocol.h"   /* proto_ball_x / proto_ball_valid（任务3 稳定判定） */
#include "IRPHOTO.h"
#include "Motor.h"
#include "KEY.h"
#include "menu_defs.h"
#include "WcTFT180.h"

static TaskId current_task = TASK_IDLE;
static int    last_result  = TASK_RESULT_RUNNING;
static uint32 last_elapsed_ms = 0;

#define T3_STABLE_ERR     12     /* 任务3 稳定判定：误差（像素） */
#define T3_STABLE_MS      200    /* 任务3 稳定判定：持续（ms） */
#define STOP_SLOW_MS      500    /* 任务5/6 缓停时长（ms） */

/* 球目标 cm(0.1cm) → 像素：offset = cm * px_per_cm / 10 */
static void ball_set_cm(int16 cm_x10)
{
    g_servo_target = (int16)(pixel_zero + ((int32)cm_x10 * px_per_cm) / 10);
}

static uint8 line_is_stop(const int s[8])   /* ≥3 路连续亮 = 停车线 */
{
    uint8 i, run = 0, max_run = 0;
    for (i = 0; i < 8; i++)
    {
        if (s[i]) { run++; if (run > max_run) max_run = run; }
        else run = 0;
    }
    return (max_run >= 3);
}

/* ═══ 任务 2/5/6 共用巡线引擎 ═══
 * enable_ball=1 时 servo 使能 + 球目标=ball_target（5ms 中断自动追踪）
 * 停车线确认后：任务 2 立即停；任务 5/6 缓停（电机线性减速，球稳保持） */
static int line_drive_run(uint8 enable_ball, int16 ball_target)
{
    int s[8];
    uint8  stop_cd = 0, stop_done = 0;
    uint32 start_tick, last_imu_tick, elapsed_ms;
    uint32 last_sec = 0xFFFFFFFF;   /* 上次显示的秒（初始非法值强制首帧刷新） */
    int    result = TASK_RESULT_OK;
    const uint32 line_timeout_ms = (current_task == TASK_2) ? 20000UL : 30000UL;

    motor1_control(0);
    motor2_control(0);
    if (enable_ball) { Servo_Enable(); ball_set_cm(ball_target); }
    else Servo_Control_Init();   /* 任务 2：失能 + 回中 + 关 UART3 接收 */

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, (enable_ball ? (ball_target ? "TASK 6" : "TASK 5") : "TASK 2"));

    EA = 0; start_tick = pit_tick; last_imu_tick = pit_tick; EA = 1;

    while (1)
    {
        uint8 is_st;
        int   err;

        IRPHOTO_Read(s);

        /* 任务 2：陀螺仪采样积分（每 5ms），停车用 yaw 确认走完一圈 */
        if (!enable_ball && imu_active)
        {
            uint32 now;
            EA = 0; now = pit_tick; EA = 1;
            if ((now - last_imu_tick) >= 1)
            {
                imu_ctrl_tick((uint16)((now - last_imu_tick) * 5));
                last_imu_tick = now;
            }
        }

        /* 停车线检测（任务 2/5/6 共用）：起步忽略 1s → 连续 3 帧确认
         * 任务 2 需 yaw 累计 ≥300°（走完一圈，防弯道误停）；任务 5/6 纯红外（陀螺仪已停） */
        is_st = line_is_stop(s);
        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;

        if (elapsed_ms < 1000) stop_cd = 0;
        else if (is_st && (!imu_active || (imu_yaw_x100 < 0 ? -imu_yaw_x100 : imu_yaw_x100) >= YAW_LAP_MIN * 100))
        {
            if (++stop_cd >= 3)
            {
                result = TASK_RESULT_OK;
                if (enable_ball)
                {
                    /* 任务5/6 缓停：球稳保持，电机 500ms 线性减速（继续巡线保方向） */
                    uint32 t0, dt;
                    EA = 0; t0 = pit_tick; EA = 1;
                    while (1)
                    {
                        EA = 0; dt = (pit_tick - t0) * 5; EA = 1;
                        if (dt >= STOP_SLOW_MS) break;
                        IRPHOTO_Read(s);
                        line_ctrl_set(calc_error(s),
                                      (int16)((int32)base_speed * (STOP_SLOW_MS - (int32)dt) / STOP_SLOW_MS));
                        system_delay_ms(10);
                    }
                }
                motor1_control(0);
                motor2_control(0);
                stop_done = 1;
            }
        }
        else stop_cd = 0;

        /* 巡线（球稳由 5ms 中断驱动，主循环不调 tick） */
        err = calc_error(s);
        line_ctrl_set(err, base_speed);

        if (stop_done) break;

        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;

        /* 简易时钟：整秒变化才刷新（1Hz，省算力） */
        if (elapsed_ms / 1000 != last_sec)
        {
            last_sec = elapsed_ms / 1000;
            WcTFT_PrintAt(0, 32, "                ");
            WcTFT_PrintAt(0, 32, "T:");
            WcTFT_PrintIntAt(24, 32, (int32)last_sec);
            WcTFT_PrintAt(48, 32, "s");
        }

        if (elapsed_ms > line_timeout_ms) { result = TASK_RESULT_TIMEOUT; break; }
    }

    motor1_control(0);
    motor2_control(0);
    if (enable_ball) Servo_Control_Init();   /* 舵机回中 + 复位 + 禁用 */

    EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
    last_elapsed_ms = elapsed_ms;
    return result;
}

/* ═══ 任务 3：球 O→+5cm→-5cm 往返（静止，servo 5ms 中断追踪，主循环管状态机） ═══ */
static int task3_run(void)
{
    uint32 start_tick, elapsed_ms;
    uint32 last_sec = 0xFFFFFFFF;
    uint8  ph = 0;              /* 0=O, 1=+5cm, 2=-5cm, 3=完成 */
    uint32 stable_since = 0;
    int16  tgt;

    Servo_SetTask3Params();   /* 任务 3 特调参数（与归中/任务5/6 分开，现场调宏） */
    Servo_Enable();
    ball_set_cm(0);

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "TASK 3");

    EA = 0; start_tick = pit_tick; EA = 1;

    while (1)
    {
        if (ph == 0)      tgt = pixel_zero;
        else if (ph == 1) tgt = (int16)(pixel_zero + 5 * px_per_cm);
        else              tgt = (int16)(pixel_zero - 5 * px_per_cm);
        g_servo_target = tgt;

        if (ph < 3)
        {
            int16 err = (int16)(proto_ball_x - tgt);
            int16 ae  = (err < 0) ? (int16)(-err) : err;
            if (proto_ball_valid && ae < T3_STABLE_ERR)
            {
                if (stable_since == 0) stable_since = g_servo_ms;
                if (g_servo_ms - stable_since >= T3_STABLE_MS) { ph++; stable_since = 0; }
            }
            else stable_since = 0;
        }
        else
        {
            /* 保持 -5cm 不回中：结果页期间球停在任务位置，退出结果页后才回中（避免扯皮） */
            EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;
            last_elapsed_ms = elapsed_ms;
            return TASK_RESULT_OK;
        }

        system_delay_ms(10);

        EA = 0; elapsed_ms = (pit_tick - start_tick) * 5; EA = 1;

        /* 简易时钟：整秒变化才刷新（1Hz，省算力） */
        if (elapsed_ms / 1000 != last_sec)
        {
            last_sec = elapsed_ms / 1000;
            WcTFT_PrintAt(0, 32, "                ");
            WcTFT_PrintAt(0, 32, "T:");
            WcTFT_PrintIntAt(24, 32, (int32)last_sec);
            WcTFT_PrintAt(48, 32, "s");
        }

        if (elapsed_ms > 5000)      /* 赛题任务3 限 5s */
        {
            last_elapsed_ms = elapsed_ms;
            return TASK_RESULT_TIMEOUT;   /* 同样保持球位，退出结果页统一回中 */
        }
    }
}

/* ═══ 结果页：任务号 + OK/FAILED/TIMEOUT + 耗时，Key4 返回 ═══ */
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
    while (1)
    {
        button_control(0);
        if (key4_flag) { key4_flag = 0; break; }
        system_delay_ms(10);
    }
}

/* ═══ 公共接口 ═══ */
void task_sched_set(int task_id)
{
    if (task_id >= TASK_IDLE && task_id <= TASK_6)
        current_task = (TaskId)task_id;
}

int task_sched_get(void)        { return (int)current_task; }
int task_sched_result(void)     { return last_result; }

void task_sched_run(void)
{
    int result;

    if (current_task == TASK_2) { line_ctrl_apply_t2(); base_speed = base_speed_t2; }
    else { line_ctrl_apply_other(); base_speed = base_speed_ot; }

    if (current_task != TASK_2) imu_ctrl_stop();   /* 非任务 2 停陀螺仪省算力 */

    switch (current_task)
    {
        case TASK_2: result = line_drive_run(0, 0);                   break;
        case TASK_5: result = line_drive_run(1, 0);                   break;
        case TASK_6: result = line_drive_run(1, ball_target_cm_x10);  break;
        case TASK_3: result = task3_run();                            break;
        default:     result = TASK_RESULT_FAIL;                       break;
    }

    last_result = result;
    Servo_Enable();           /* 任务结束恢复常开（任务 2 运行中已关闭，其他任务回中后恢复控球） */
    task_sched_show_result(result);

    /* 任务 3：结果页保持球位（-5cm）避免扯皮，退出结果页才回中 */
    if (current_task == TASK_3) Servo_Control_Init();
}