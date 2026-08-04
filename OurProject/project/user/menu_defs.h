/*********************************************************************************************************************
* 文件名称          menu_defs.h
* 说明              菜单页面定义模块 —— 所有 MenuPage/MenuItem 和导航回调集中管理
*
* 使用：
*   main.c 中 #include "menu_defs.h"，直接引用 page_main / page_launch
*
* 页面：
*   主菜单      Steer PID(子页) / Spd(直接调) / Ball(子页) / Led(直接调)
*   LAUNCH      主菜单按 P3.2 进入，点选 Task 即启动
********************************************************************************************************************/

#ifndef __MENU_DEFS_H__
#define __MENU_DEFS_H__

#include "WcMenu.h"

extern MenuPage page_main;        /* 主菜单页 */
extern MenuPage page_launch;      /* Launch 确认页 */
extern int16    t2_backup_enable; /* 任务2 停车后倒车开关：1=倒车，0=马上停（int16 匹配菜单 value） */
extern int16   ball_display;      /* Launch 页 Ball 显示当前球位置（main 每 100ms 刷新） */
extern uint8   launch_triggered;  /* cb_launch 回调置位 → main 退出菜单循环 */
extern int16   base_speed;        /* 当前激活基准 duty（任务启动时应用） */
extern int16   base_speed_t2;     /* 任务 2 基准 duty（40%） */
extern int16   base_speed_t4;     /* 任务 4 基准 duty（更快，8s 走 AB） */
extern int16   base_speed_ot;     /* 其他任务基准 duty（21%） */

#endif
