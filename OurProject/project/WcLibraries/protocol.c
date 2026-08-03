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

/* ── 接收状态 ── */
static char  rx_line[LINE_BUF_SIZE];
static uint8 rx_idx;

/* ── 全局状态 ── */
int16 proto_ball_x      = 0;
int16 proto_ball_valid  = 0;

/* ── 调试：原始字节环形缓冲（测试 P5.0 是否收到 OpenART 消息） ── */
#define RX_MON_SIZE   64
static volatile uint8 rx_mon[RX_MON_SIZE];
static volatile uint8 rx_mon_head = 0;   /* ISR 写指针 */
static volatile uint8 rx_mon_tail = 0;   /* 主循环读指针 */

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
    }
    else if (line[0] == 'N' || line[0] == 'n')
    {
        proto_ball_valid = 0;
    }
    /* 其他：忽略（可能是噪声或未来扩展） */
}

/* ── ISR：追加一个原始字节到环形缓冲（满则丢最新字节） ── */
static void rx_mon_put(uint8 d)
{
    uint8 nxt = (uint8)((rx_mon_head + 1) % RX_MON_SIZE);
    if (nxt != rx_mon_tail)
    {
        rx_mon[rx_mon_head] = d;
        rx_mon_head = nxt;
    }
}

/* ── DMA 接收回调（ISR 中调用，每次一个字节） ── */
static void rx_callback(uint8 dat)
{
    rx_mon_put(dat);   /* 调试：原始字节全部入环形缓冲 */

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

    /* 初始化 UART3: P5.0=RX, P5.1=TX, 115200bps */
    uart_init(UART_3, 115200, UART3_TX_P51, UART3_RX_P50);

    /* 注册 DMA 接收回调 + 使能 DMA 接收中断
     * ⚠️ 必须用 uart_rx_interrupt()：它置 DMA_UR3R_CFG bit7（接收中断使能）。
     *    曾直接赋值 uart_rx_handlers[UART_3]=rx_callback 漏了中断使能，
     *    → 数据进 DMA buffer 但回调永不触发（PROTO/UTEST 收 0 字节）。 */
    uart_rx_interrupt(UART_3, ENABLE, rx_callback);

    /* 启动 DMA 接收 */
    uart_rx_start_buff(UART_3);
}

/* 回环测试：从 UART3 TX (P5.1) 发一帧测试数据（配合 UTEST 命令 + P5.1→P5.0 短接） */
void Protocol_SendLoopback(void)
{
    uart_write_string(UART_3, "LOOP\n");
}

/* 调试：读出积压的原始接收字节（消费），返回读出数 */
uint8 Protocol_DebugGet(uint8 *out, uint8 max)
{
    uint8 n = 0;
    while (n < max && rx_mon_tail != rx_mon_head)
    {
        out[n++] = rx_mon[rx_mon_tail];
        rx_mon_tail = (uint8)((rx_mon_tail + 1) % RX_MON_SIZE);
    }
    return n;
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

/** 读取最新球位置并清空标志位（防止读到重复数据） */
uint8 Protocol_ReadBall(int16 *x)
{
    if (proto_ball_valid)
    {
        if (x) *x = proto_ball_x;
        proto_ball_valid = 0;   /* 消费后清零，避免下次读到相同数据 */
        return 1;
    }
    return 0;
}
