/* protocol.h — OpenART ↔ STC32 UART 协议
 * OpenART 上电即发 B,<cx>\n（球像素 X）/ N\n；STC32 中断自动维护
 * 用法：Protocol_Init(); Protocol_Start(); 主循环 Protocol_ReadBall(&x) */
#ifndef _protocol_h_
#define _protocol_h_

#include "zf_common_headfile.h"

extern int16  proto_ball_x;            /* 最新球像素 X（valid=1 时有效） */
extern int16  proto_ball_valid;        /* 1=有有效球数据，0=未检测到 */
extern volatile uint16 proto_ball_frame_id;  /* 每收一帧递增（数据新鲜度） */

void Protocol_Init(void);              /* 配 UART3 硬件（不使能接收） */
void Protocol_Start(void);             /* 开 DMA 接收中断 + 开始接收 */
void Protocol_Stop(void);              /* 关接收 */
uint8 Protocol_ReadBall(int16 *x);     /* 快照球 X，返回 1=当前有效 */

#endif
