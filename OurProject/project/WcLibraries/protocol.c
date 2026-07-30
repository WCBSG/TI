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
********************************************************************************************************************/

#include "protocol.h"

/* ── 常量 ── */
#define LINE_BUF_SIZE   16      /* "B,320\n" = 7 字节，16 足够 */
#define BALL_TIMEOUT    200     /* 超时 ms，超过此时间无数据则置 invalid */

/* ── 接收状态 ── */
static char  rx_line[LINE_BUF_SIZE];
static uint8 rx_idx;
static uint8 data_timeout;      /* 倒计时计数器（在定时器中递减） */

/* ── 全局状态 ── */
int16 proto_ball_x      = 0;
uint8 proto_ball_valid  = 0;

/* ── 内部：解析一行 ── */
static void parse_line(const char *line)
{
    /* 格式: B,<cx> */
    if (line[0] == 'B' || line[0] == 'b')
    {
        int16 val = 0;
        uint8 neg = 0;
        uint8 i = 2;  /* 跳过 "B," */

        if (line[1] != ',') return;  /* 格式错误 */

        if (line[i] == '-') { neg = 1; i++; }

        while (line[i] >= '0' && line[i] <= '9')
        {
            val = (int16)(val * 10 + (line[i] - '0'));
            i++;
        }

        proto_ball_x      = neg ? (int16)(-val) : val;
        proto_ball_valid  = 1;
        data_timeout      = BALL_TIMEOUT;
    }
    else if (line[0] == 'N' || line[0] == 'n')
    {
        proto_ball_valid = 0;
        data_timeout      = BALL_TIMEOUT;  /* 收到 N 也算数据活跃 */
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
    data_timeout = 0;

    proto_ball_x     = 0;
    proto_ball_valid = 0;

    /* 初始化 UART3: P5.0=RX, P5.1=TX, 115200bps */
    uart_init(UART_3, 115200, UART3_TX_P51, UART3_RX_P50);

    /* 注册 DMA 接收回调 */
    uart_rx_handlers[UART_3] = rx_callback;

    /* 启动 DMA 接收 */
    uart_rx_start_buff(UART_3);
}

void Protocol_SendStart(void)
{
    uart_write_string(UART_3, "START\n");
}

void Protocol_SendStop(void)
{
    uart_write_string(UART_3, "STOP\n");
}

void Protocol_SendCal(void)
{
    uart_write_string(UART_3, "CAL\n");
}
