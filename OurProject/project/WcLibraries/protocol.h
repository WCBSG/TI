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
*   int16 ball_x;
*   if (Protocol_ReadBall(&ball_x)) {
*       error = ball_x - target_x;
*   }
********************************************************************************************************************/

#ifndef _protocol_h_
#define _protocol_h_

#include "zf_common_headfile.h"

/* ── 协议状态（全局可读） ── */
extern int16 proto_ball_x;       /* 最新球 X 坐标（像素），仅 proto_ball_valid=1 时有效 */
extern int16 proto_ball_valid;   /* 1=有有效球数据，0=未检测到（int16 以匹配菜单 BOOL 项） */

/* ── 初始化和控制 ── */
void Protocol_Init(void);        /* 初始化 UART3 硬件（不使能接收） */
void Protocol_Start(void);       /* 启动接收：使能 DMA 中断 + 开始接收，中断自动维护球位置 */
void Protocol_Stop(void);        /* 停止接收：失能 DMA 中断 */
uint8 Protocol_DebugGet(uint8 *out, uint8 max); /* 调试：读出积压的原始接收字节（消费） */
void Protocol_SendLoopback(void);    /* 回环测试：UART3 TX 发 "LOOP\n"（UTEST 用） */
void Protocol_SendStart(void);   /* 发送 START 命令给 OpenART */
void Protocol_SendStop(void);    /* 发送 STOP 命令 */
void Protocol_SendCal(void);     /* 发送 CAL 命令 */
uint8 Protocol_ReadBall(int16 *x); /* 读取最新球 X 坐标并消费（返回 1=有效, 0=无新数据） */

#endif
