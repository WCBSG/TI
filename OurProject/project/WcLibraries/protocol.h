/*********************************************************************************************************************
* 文件名称          protocol.h
* 说明              OpenART ↔ STC32 通信协议处理
*
* 协议：
*   OpenART → STC32 (UART3, P5.0=RX, 115200bps):
*     B,<cx>\n   — 检测到球，cx 为像素 X 坐标
*     N\n        — 未检测到球
*
*   STC32 → OpenART (UART3, P5.1=TX):
*     START\n    — 开始发送
*     STOP\n     — 停止发送
*     CAL\n      — 校准参考点
*
* 使用：
*   Protocol_Init();
*   // 在 PID 循环中读取:
*   if (proto_ball_valid) {
*       error = proto_ball_x - target_x;
*   }
********************************************************************************************************************/

#ifndef _protocol_h_
#define _protocol_h_

#include "zf_common_headfile.h"

/* ── 协议状态（全局可读） ── */
extern int16 proto_ball_x;       /* 最新球 X 坐标（像素），仅 proto_ball_valid=1 时有效 */
extern uint8 proto_ball_valid;   /* 1=有有效球数据，0=未检测到/超时 */

/* ── 初始化和控制 ── */
void Protocol_Init(void);        /* 初始化 UART3 + DMA 接收 */
void Protocol_SendStart(void);   /* 发送 START 命令给 OpenART */
void Protocol_SendStop(void);    /* 发送 STOP 命令 */
void Protocol_SendCal(void);     /* 发送 CAL 命令 */

#endif
