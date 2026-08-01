/*********************************************************************************************************************
* 文件名称          menu_defs.h
* 说明              菜单页面定义模块 —— 所有 MenuPage/MenuItem 和导航回调集中管理
*
* 使用：
*   main.c 中 #include "menu_defs.h"，直接引用 page_main / page_launch
********************************************************************************************************************/

#ifndef __MENU_DEFS_H__
#define __MENU_DEFS_H__

#include "WcMenu.h"

extern MenuPage page_main;        /* 主菜单页 */
extern MenuPage page_launch;      /* Launch 确认页 */
extern MenuPage page_ir_test;     /* IR 传感器测试页 */
extern uint8   launch_triggered;  /* cb_launch 回调置位 → main 退出菜单循环 */
extern int16   base_speed;        /* 基准 duty（0-10000 满占空比），差速开环发车用 */

#endif
