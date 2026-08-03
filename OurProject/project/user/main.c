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
#include "cmd_ctrl.h"    /* USB-CDC 命令触发测试 */

/* ── LED 灯带 PWM（PA2 = PWME_CH2，300Hz 与舵机同 PWME 组同频） ── */
#define LED_PWM_PIN    (IO_PA2)
#define LED_PWM_FREQ   300


/* ═══════════════════════════════════════════════════════════
 * 光电传感器测试（串口 IR 命令触发）：循环读 8 路红外 → USB-CDC 输出
 * 串口格式: IR=00011000 E=2 ST=0   （IR 左→右 8 位，1=黑线；E=加权偏差；ST=停车标志）
 * 退出：收到 STOP 命令 / 按任意键
 * ════════════════════════════════════════════════════════════ */
static void ir_test_run(void)
{
    int  s[8];
    char bits[9];
    uint8 i;

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "IR TEST");
    WcTFT_PrintCenter(3, "STOP: exit");

    while (1)
    {
        IRPHOTO_Read(s);

        for (i = 0; i < 8; i++) bits[i] = s[i] ? '1' : '0';
        bits[8] = '\0';

        /* USB-CDC：位串 + 偏差（%s 用位串，%d 必须 (int32)） */
        {
            char buf[48];
            uint32 n = 0;
            n += zf_sprintf((int8 *)(buf + n), "IR=%s E=%d\n",
                            bits, (int32)calc_error(s));
            usb_cdc_write_buffer((const uint8 *)buf, (uint16)n);
        }

        /* 屏幕同步显示（安全封装，越界自动截断） */
        WcTFT_PrintAt(0, 48, "IR:");
        WcTFT_PrintAt(20, 48, bits);
        WcTFT_PrintAt(0, 64, "E:");
        WcTFT_PrintIntAt(20, 64, (int32)calc_error(s));

        /* 检查 STOP 命令 / 任意键退出 */
        cmd_poll();                      /* 处理 IR 测试期间的命令（STOP 等） */
        if (!ir_test_cmd) break;         /* STOP 命令清零标志 → 退出 */
        button_control(0);
        if (key1_flag || key2_flag || key3_flag || key4_flag)
        {
            key1_flag = key2_flag = key3_flag = key4_flag = 0;
            ir_test_cmd = 0;
            break;
        }

        system_delay_ms(100);
    }
}


/* ═══════════════════════════════════════════════════════════
 * UART3 协议测试（PROTO 命令触发）：验证 P5.0 能否收到 OpenART 摄像头消息
 *   把 P5.0 收到的原始字节转发到 USB-CDC（hex+ASCII），每 100ms 一行状态
 *   上位机看到字节流 = P5.0 收通；PX/V 显示解析出的球坐标
 * 退出：收到 STOP 命令 / 按任意键
 * ════════════════════════════════════════════════════════════ */
static void proto_test_run(void)
{
    uint8  raw[16], i, n;
    uint32 n_byte = 0;
    uint8  dbg_cd = 0;
    char   buf[96];
    uint32 nb;

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "UART3 TEST");
    WcTFT_PrintCenter(3, "STOP: exit");

    usb_cdc_write_string("[PROTO] listening P5.0 (UART3 RX)...\n");

    while (1)
    {
        n = Protocol_DebugGet(raw, sizeof(raw));
        if (n)
        {
            for (i = 0; i < n; i++)
            {
                nb = 0;
                nb += zf_sprintf((int8 *)(buf + nb), "%X%c ",
                                 (int32)raw[i],
                                 (raw[i] >= 0x20 && raw[i] < 0x7F) ? (char)raw[i] : '.');
                usb_cdc_write_buffer((const uint8 *)buf, (uint16)nb);
                n_byte++;
            }
        }

        if (++dbg_cd >= 10)              /* 每 100ms 状态行 */
        {
            dbg_cd = 0;
            nb = 0;
            nb += zf_sprintf((int8 *)(buf + nb), "[%d] PX=%d V=%d\n",
                             (int32)n_byte, (int32)proto_ball_x, (int32)proto_ball_valid);
            usb_cdc_write_buffer((const uint8 *)buf, (uint16)nb);

            WcTFT_PrintAt(0, 32, "Px:");
            WcTFT_PrintIntAt(24, 32, (int32)proto_ball_x);
            WcTFT_PrintAt(0, 48, "V:");
            WcTFT_PrintIntAt(24, 48, (int32)proto_ball_valid);
            WcTFT_PrintAt(0, 64, "Rx:");
            WcTFT_PrintIntAt(24, 64, (int32)n_byte);
        }

        cmd_poll();                      /* 处理协议测试期间的命令（STOP 等） */
        if (!proto_test_cmd) break;
        button_control(0);
        if (key1_flag || key2_flag || key3_flag || key4_flag)
        {
            key1_flag = key2_flag = key3_flag = key4_flag = 0;
            proto_test_cmd = 0;
            break;
        }

        system_delay_ms(10);
    }

    usb_cdc_write_string("[PROTO] stopped\n");
}


/* ═══════════════════════════════════════════════════════════
 * UART3 回环自测（UTEST 命令触发）：验证 STC32 UART3 收发链路
 *   用法：把 OpenART TX 线从 P5.0 拔掉，用杜邦线短接 P5.1→P5.0
 *   然后发 UTEST：STC32 每 500ms 从 P5.1 自发 "LOOP\n"，
 *   若 P5.0 收到（USB-CDC 看到 4C4F4F50 4C L O O P 字节）→ STC32 收发正常
 *   收不到 → STC32 UART3 配置/引脚问题
 * 退出：收到 STOP 命令 / 按任意键
 * ════════════════════════════════════════════════════════════ */
static void utest_run(void)
{
    uint8  raw[16], i, n;
    uint32 t_last, send_cnt = 0;
    uint8  dbg_cd = 0;
    char   buf[160];
    uint32 nb;

    WcTFT_Clear(RGB565_BLACK);
    WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
    WcTFT_PrintCenter(0, "UART LOOP");
    WcTFT_PrintCenter(3, "STOP: exit");

    /* 诊断：UART3/DMA 寄存器初始状态 */
    nb = 0;
    nb += zf_sprintf((int8 *)(buf + nb), "[D] RCFG=%X RCR=%X RSTA=%X TCFG=%X TSTA=%X\n",
                     (int32)DMA_UR3R_CFG, (int32)DMA_UR3R_CR, (int32)DMA_UR3R_STA,
                     (int32)DMA_UR3T_CFG, (int32)DMA_UR3T_STA);
    nb += zf_sprintf((int8 *)(buf + nb), "[D] S3CON=%X P_SW2=%X TICK=%d\n",
                     (int32)S3CON, (int32)P_SW2, (int32)pit_tick);
    usb_cdc_write_buffer((const uint8 *)buf, (uint16)nb);

    usb_cdc_write_string("[UTEST] short P5.1 to P5.0, sending LOOP every 500ms...\n");
    EA = 0; t_last = pit_tick; EA = 1;

    while (1)
    {
        n = Protocol_DebugGet(raw, sizeof(raw));
        if (n)
        {
            for (i = 0; i < n; i++)
            {
                nb = 0;
                nb += zf_sprintf((int8 *)(buf + nb), "%X%c ",
                                 (int32)raw[i],
                                 (raw[i] >= 0x20 && raw[i] < 0x7F) ? (char)raw[i] : '.');
                usb_cdc_write_buffer((const uint8 *)buf, (uint16)nb);
            }
        }

        EA = 0; if ((pit_tick - t_last) * 5 >= 500) { EA = 1; Protocol_SendLoopback(); t_last = pit_tick; send_cnt++; }
        else EA = 1;

        if (++dbg_cd >= 10)              /* 每 100ms 诊断心跳 */
        {
            dbg_cd = 0;
            nb = 0;
            nb += zf_sprintf((int8 *)(buf + nb), "SEND=%d TICK=%d RSTA=%X TSTA=%X P50=%d P51=%d\n",
                             (int32)send_cnt, (int32)pit_tick,
                             (int32)DMA_UR3R_STA, (int32)DMA_UR3T_STA,
                             (int32)gpio_get_level(IO_P50), (int32)gpio_get_level(IO_P51));
            usb_cdc_write_buffer((const uint8 *)buf, (uint16)nb);
        }

        cmd_poll();                      /* 处理测试期间的命令（STOP 等） */
        if (!utest_cmd) break;
        button_control(0);
        if (key1_flag || key2_flag || key3_flag || key4_flag)
        {
            key1_flag = key2_flag = key3_flag = key4_flag = 0;
            utest_cmd = 0;
            break;
        }

        system_delay_ms(10);
    }

    usb_cdc_write_string("[UTEST] stopped\n");
}


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
    Protocol_Start();   /* 启动 UART3 接收：中断自动维护球位置（菜单/任务都能读） */

    /* 配置加载必须先于任何消费 flash_buff 的初始化，否则持久化参数不生效 */
    config_load();
    line_ctrl_init();
    ball_ctrl_init();
    /* base_speed 由 task_sched_run 按任务应用（base_speed_t2 / base_speed_ot），不再从 flash 恢复 */

    imu_ctrl_init();   /* 陀螺仪：硬件 SPI3 + 静止 1s 零点标定（失败不阻塞） */

    /* LED 灯带 PWM（PA2，300Hz 与舵机同频），亮度由 LED 调试页调节 */
    //pwm_init(LED_PWM_PIN, LED_PWM_FREQ, 0);
    gpio_init(LED_PWM_PIN, GPO, 1, GPO_PUSH_PULL);
    pit_ms_init(PIT_ENCODER, 5, pit_handler);

    cmd_ctrl_init();   /* USB-CDC 命令触发：上位机发 T2/IR 等命令启动测试 */

    /* ── 启动主菜单 ── */
    Menu_Init();
    Menu_Push(&page_main);

    /* 版本识别码（测试用） */
    WcTFT_PrintAt(96, 0, "V5");

    /* ── 三态主循环：菜单 ⇄ 任务 ⇄ 结果 ── */
    launch_triggered = 0;
    while (1)
    {
        /* ── 菜单阶段：按键导航 + USB-CDC 命令，等待 Launch/命令 选任务 ── */
        while (!launch_triggered)
        {
            cmd_poll();                    /* USB-CDC 命令：T2/T3/T5/T6 启动任务，IR 光电测试 */

            if (ir_test_cmd)               /* IR 命令 → 光电测试（STOP/按键退出） */
            {
                ir_test_run();
                ir_test_cmd = 0;           /* 退出后再清，防重复进入 */
                WcTFT_Clear(RGB565_BLACK);
                WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
                Menu_Draw();               /* 测试结束后重绘菜单 */
            }

            if (proto_test_cmd)            /* PROTO 命令 → UART3 协议测试 */
            {
                proto_test_run();
                proto_test_cmd = 0;        /* 退出后再清，防重复进入 */
                WcTFT_Clear(RGB565_BLACK);
                WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
                Menu_Draw();
            }

            if (utest_cmd)                 /* UTEST 命令 → UART3 回环自测 */
            {
                utest_run();
                utest_cmd = 0;             /* 退出后再清，防重复进入 */
                WcTFT_Clear(RGB565_BLACK);
                WcTFT_SetColor(RGB565_WHITE, RGB565_BLACK);
                Menu_Draw();
            }

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

        /* ── 任务结束 ──
         * 结果页可能已收到串口命令（T2 等）置位 launch_triggered → 立即重跑
         * 否则正常回菜单 */
        if (launch_triggered)
        {
            launch_triggered = 0;      /* 清除，让循环重跑新任务 */
            continue;                  /* 不 Menu_Push 菜单，直接进下一轮 task_sched_run */
        }
        launch_triggered = 0;
        Menu_Push(&page_main);
    }
}
