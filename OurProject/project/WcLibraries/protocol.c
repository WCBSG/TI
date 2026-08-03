/*********************************************************************************************************************
* 文件名称          protocol.c
* 说明              OpenART ↔ STC32 UART 协议处理
*
* 接线：
*   OpenART UART12 TX (LPSR_06) → STC32 P5.0 (UART3 RX)
*   OpenART UART12 RX (LPSR_07) → STC32 P5.1 (UART3 TX)
*
* 数据流：
*   OpenART 每帧发送 "B,<cx>\n" 或 "N\n"
*   STC32 DMA 逐字节接收 → line buffer → 解析 → 更新 proto_ball_x / proto_ball_valid
*
* 比赛清理（2026-08）：删原始字节环形缓冲（rx_mon）/ DebugGet / SendLoopback（测试用）
********************************************************************************************************************/

#include "protocol.h"

/* ── 常量 ── */
#define LINE_BUF_SIZE   16      /* "B,320\n" = 7 字节，16 足够 */

/* ── 接收状态 ── */
static char  rx_line[LINE_BUF_SIZE];
static uint8 rx_idx;

/* ── 全局状态 ── */
int16 proto_ball_x      = 0;
int16 proto_ball_valid  = 0;
volatile uint16 proto_ball_frame_id = 0;

/* ── 内部：解析一行 ── */
static void parse_line(const char *line)
{
    /* 格式: B,<cx> */
    if (line[0] == 'B' || line[0] == 'b')
    {
        int32 val = 0;
        uint8 has_digit = 0;
        uint8 i = 2;  /* 跳过 "B," */

        if (line[1] != ',') return;  /* 格式错误 */

        while (line[i] >= '0' && line[i] <= '9')
        {
            has_digit = 1;
            val = val * 10 + (line[i] - '0');
            i++;
        }
        if (!has_digit || line[i] != '\0' || val > 319) return;

        proto_ball_x      = (int16)val;
        proto_ball_valid  = 1;
        proto_ball_frame_id++;
    }
    else if (line[0] == 'N' || line[0] == 'n')
    {
        proto_ball_valid = 0;
        proto_ball_frame_id++;
    }
    /* 其他：忽略（可能是噪声或未来扩展） */
}

/* ── DMA 接收回调（ISR 中调用，每次一个字节） ── */
static void rx_callback(uint8 dat)
{
    if (dat == '\n')
    {
        /* 行结束：解析 */
        rx_line[rx_idx] = '\0';
        parse_line(rx_line);
        rx_idx = 0;
    }
    else if (dat == '\r')
    {
        /* 忽略回车 */
    }
    else
    {
        /* 普通字节：追加到缓冲区 */
        if (rx_idx < LINE_BUF_SIZE - 1)
        {
            rx_line[rx_idx] = (char)dat;
            rx_idx++;
        }
        else
        {
            /* 缓冲区溢出：丢弃整行，重新开始 */
            rx_idx = 0;
        }
    }
}

/* ═══════════════════════════════════════════════════════
 * 公开 API
 * ════════════════════════════════════════════════════════ */

void Protocol_Init(void)
{
    rx_idx       = 0;

    proto_ball_x     = 0;
    proto_ball_valid = 0;
    proto_ball_frame_id = 0;

    /* 初始化 UART3: P5.0=RX, P5.1=TX, 115200bps（不使能接收，由 Protocol_Start 控制） */
    uart_init(UART_3, 115200, UART3_TX_P51, UART3_RX_P50);
}

void Protocol_Start(void)
{
    /* 注册 DMA 接收回调 + 使能 DMA 接收中断
     * ⚠️ 必须用 uart_rx_interrupt()：它置 DMA_UR3R_CFG bit7（接收中断使能）。
     *    曾直接赋值 uart_rx_handlers[UART_3]=rx_callback 漏了中断使能，
     *    → 数据进 DMA buffer 但回调永不触发（PROTO/UTEST 收 0 字节）。
     * 启动后中断自动维护 proto_ball_x / proto_ball_valid（rx_callback 解析）。 */
    uart_rx_interrupt(UART_3, ENABLE, rx_callback);

    /* 启动 DMA 接收 */
    uart_rx_start_buff(UART_3);
}

void Protocol_Stop(void)
{
    /* 失能 DMA 接收中断：停止回调触发（不再维护球位置变量） */
    uart_rx_interrupt(UART_3, DISABLE, rx_callback);
}

/** 快照当前球位置；有效状态由接收中断持续维护，不在读取时消费。 */
uint8 Protocol_ReadBall(int16 *x)
{
    if (proto_ball_valid)
    {
        if (x) *x = proto_ball_x;
        return 1;
    }
    return 0;
}
