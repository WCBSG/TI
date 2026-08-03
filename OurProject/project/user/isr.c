/*********************************************************************************************************************
* 文件名称          isr.c
* 说明              中断服务 — 精简版（只保留项目实际使用的中断）
*
* 实际使能的中断：
*   1. DMA_UART3   OpenART 摄像头 UART3 接收（protocol.c，逐字节 DMA 中断）
*   2. TM1         PIT 5ms 时基（main 里 pit_ms_init(PIT_ENCODER=TIM1_PIT) → pit_handler）
*
* 说明：
*   - USB 中断在库 usb.c（usb_isr, interrupt 25），不在此文件。
*   - 其余 GPIO/外部中断/其他定时器/mt9v03x 均未使用，已删除（避免空向量死机）。
********************************************************************************************************************/

#include "zf_common_headfile.h"

/* ═══════════════════════════════════════════════════════════
 * DMA UART3 接收中断：OpenART 摄像头球坐标（逐字节 DMA 接收）
 *   DMA 收到 1 字节 → 清标志 → 重启接收 → 调回调（protocol.c 解析）
 * ════════════════════════════════════════════════════════════ */
void DMA_UART3_IRQHandler(void) interrupt DMA_UR3R_VECTOR
{
    if (DMA_UR3R_STA & 0x01) /* 接收完成 */
    {
        DMA_UR3R_STA &= ~0x01;      /* 清标志位 */
        uart_rx_start_buff(UART_3); /* 重新开启一次接收，防止丢失 */

        if (uart_rx_handlers[UART_3] != NULL)
        {
            uart_rx_handlers[UART_3](uart_rx_buff[UART_3][0]);
        }
    }

    if (DMA_UR3R_STA & 0x02) /* 数据溢出 */
    {
        DMA_UR3R_STA &= ~0x02;      /* 清标志位 */
        uart_rx_start_buff(UART_3); /* 重新开启一次接收，防止丢失 */
    }
}

/* ═══════════════════════════════════════════════════════════
 * TIM1 定时器中断：PIT 5ms 时基（main 里 pit_ms_init(PIT_ENCODER, 5, pit_handler)）
 *   pit_handler 递增 pit_tick（计时）+ 编码器采样
 * ════════════════════════════════════════════════════════════ */
void TM1_IRQHandler() interrupt TMR1_VECTOR
{
    TIM1_CLEAR_FLAG;
    if (tim_irq_handlers[TIM_1] != NULL)
    {
        tim_irq_handlers[TIM_1]();
    }
}
