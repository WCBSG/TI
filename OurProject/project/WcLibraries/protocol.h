/*********************************************************************************************************************
* 文件名称          protocol.h
* 说明              OpenART ↔ STC32 通信协议处理
*
* 协议（OpenART 上电即持续发送，STC32 无 START/STOP 门控命令）：
*   OpenART → STC32 (UART3, P5.0=RX, 115200bps):
*     B,<cx>\n   — 检测到球，cx 为像素 X 坐标
*     N\n        — 未检测到球
*
* 使用：
*   Protocol_Init();      // 配 UART3 硬件（不使能接收）
*   Protocol_Start();     // 开 DMA 接收中断，中断自动维护 proto_ball_x/valid
*   Protocol_ReadBall(&x); // 主循环读取（消费），返回 1=有新数据
********************************************************************************************************************/

#ifndef _protocol_h_
#define _protocol_h_

#include "zf_common_headfile.h"

/* ── 协议状态（中断自动维护，全局可读） ── */
extern int16 proto_ball_x;       /* 最新球 X 坐标（像素），仅 proto_ball_valid=1 时有效 */
extern int16 proto_ball_valid;   /* 1=有有效球数据，0=未检测到（int16 以匹配菜单 BOOL 项） */
extern volatile uint16 proto_ball_frame_id; /* 每收到一帧 B/N 都递增，用于判断数据新鲜度 */

/* ── 初始化和控制 ── */
void Protocol_Init(void);        /* 初始化 UART3 硬件（不使能接收） */
void Protocol_Start(void);       /* 启动接收：使能 DMA 中断 + 开始接收，中断自动维护球位置 */
void Protocol_Stop(void);        /* 停止接收：失能 DMA 中断 */
uint8 Protocol_ReadBall(int16 *x); /* 快照当前球 X（不清状态），返回 1=当前检测有效 */

/* ── 调试（UTEST/PROTO 测试用） ── */
uint8 Protocol_DebugGet(uint8 *out, uint8 max); /* 读出积压的原始接收字节（消费） */
void Protocol_SendLoopback(void);    /* 回环测试：UART3 TX 发 "LOOP\n"（UTEST 用） */

#endif
