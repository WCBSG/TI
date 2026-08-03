/* isr.c — 中断服务（精简，只保留实际使能的中断）
 * DMA_UART3：OpenART UART3 接收（protocol 逐字节）| TM0：servo 5ms 舵机控制
 * TM1：5ms 时基（pit_tick 计时）；USB 中断在库 usb.c */

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
 * TIM0 定时器中断：servo 舵机 5ms PIT（pit_ms_init(TIM0_PIT, 5, Servo_Timer_Callback)）
 *   中断里读球像素 → 像素域 PID+前馈 → 控舵机
 * ════════════════════════════════════════════════════════════ */
void TM0_IRQHandler() interrupt TMR0_VECTOR
{
    TIM0_CLEAR_FLAG;
    if (tim_irq_handlers[TIM_0] != NULL)
    {
        tim_irq_handlers[TIM_0]();
    }
}

/* ═══════════════════════════════════════════════════════════
 * TIM1 定时器中断：PIT 5ms 时基（main 里 pit_ms_init(PIT_ENCODER, 5, pit_handler)）
 *   pit_handler 递增 pit_tick（计时）
 * ════════════════════════════════════════════════════════════ */
void TM1_IRQHandler() interrupt TMR1_VECTOR
{
    TIM1_CLEAR_FLAG;
    if (tim_irq_handlers[TIM_1] != NULL)
    {
        tim_irq_handlers[TIM_1]();
    }
}
