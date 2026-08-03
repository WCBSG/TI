/*********************************************************************************************************************
* 文件名称          cmd_ctrl.h
* 说明              USB-CDC 命令触发模块 — 上位机发命令启动测试
*
* 协议（\n 结尾，大小写不敏感）：
*   T2 / T3 / T5 / T6   启动对应任务（等效菜单 Launch 选任务）
*   IR                  启动光电测试（持续输出 8 路红外）
*   PROTO               启动 UART3 协议测试（转发 P5.0 原始字节 + 球坐标）
*   FAST / DBG          高速纯巡线 / 完整调试切换（task_fast_line）
*   STOP                置测试停止标志（光电/协议测试用）
*   HELP                回显支持的命令
*
* 架构：
*   1. cmd_rx_irq 注册到 usb_cdc_rx_interrupt，USB 中断上下文只存字节凑行
*   2. cmd_poll 在主循环（菜单阶段每 10ms）调用，解析执行
*   ISR 里绝不调任务/格式化（USB 中断只能做轻量操作）
********************************************************************************************************************/

#ifndef __CMD_CTRL_H__
#define __CMD_CTRL_H__

#include "zf_common_headfile.h"

/* ── 测试请求标志（cmd_poll 置位，main 菜单循环检测） ── */
extern volatile uint8 ir_test_cmd;
extern volatile uint8 proto_test_cmd;   /* PROTO 协议测试 */
extern volatile uint8 utest_cmd;        /* UTEST 回环测试 */
extern volatile uint8 hold_cmd;         /* HOLD 球稳中点测试 */

void cmd_ctrl_init(void);   /* 注册 USB-CDC 接收回调（main 初始化后调用） */
void cmd_poll(void);        /* 主循环调用：解析收到的命令并执行 */

#endif
