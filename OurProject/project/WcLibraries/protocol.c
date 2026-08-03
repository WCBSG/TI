/* protocol.c — OpenART ↔ STC32 UART 协议
 * 接线：OpenART UART12 TX→P5.0(UART3 RX)；帧 B,<cx>\n（球像素 X）/ N\n
 * DMA 逐字节接收 → 行缓冲 → 中断自动维护 proto_ball_x/valid */
#include "protocol.h"

#define LINE_BUF_SIZE  16      /* "B,320\n" 7 字节够 */

static char  rx_line[LINE_BUF_SIZE];
static uint8 rx_idx;

int16  proto_ball_x      = 0;
int16  proto_ball_valid  = 0;
volatile uint16 proto_ball_frame_id = 0;

static void parse_line(const char *line)
{
    if (line[0] == 'B' || line[0] == 'b')
    {
        int32 val = 0;
        uint8 has_digit = 0, i = 2;   /* 跳过 "B," */
        if (line[1] != ',') return;
        while (line[i] >= '0' && line[i] <= '9') { has_digit = 1; val = val * 10 + (line[i] - '0'); i++; }
        if (!has_digit || line[i] != '\0' || val > 319) return;
        proto_ball_x = (int16)val;  proto_ball_valid = 1;  proto_ball_frame_id++;
    }
    else if (line[0] == 'N' || line[0] == 'n')
    {
        proto_ball_valid = 0;  proto_ball_frame_id++;
    }
}

static void rx_callback(uint8 dat)
{
    if (dat == '\n')          { rx_line[rx_idx] = '\0'; parse_line(rx_line); rx_idx = 0; }
    else if (dat == '\r')     { /* 忽略 */ }
    else if (rx_idx < LINE_BUF_SIZE - 1) rx_line[rx_idx++] = (char)dat;
    else rx_idx = 0;          /* 溢出：丢弃整行 */
}

void Protocol_Init(void)
{
    rx_idx = 0;
    proto_ball_x = 0;  proto_ball_valid = 0;  proto_ball_frame_id = 0;
    uart_init(UART_3, 115200, UART3_TX_P51, UART3_RX_P50);
}

void Protocol_Start(void)
{
    /* ⚠️ 必须 uart_rx_interrupt()：置 DMA_UR3R_CFG bit7 使能接收中断（曾漏设收 0 字节） */
    uart_rx_interrupt(UART_3, ENABLE, rx_callback);
    uart_rx_start_buff(UART_3);
}

void Protocol_Stop(void) { uart_rx_interrupt(UART_3, DISABLE, rx_callback); }

uint8 Protocol_ReadBall(int16 *x)
{
    if (proto_ball_valid) { if (x) *x = proto_ball_x; return 1; }
    return 0;
}
