/*********************************************************************************************************************
* 文件名称          menu_defs.c
* 说明              菜单页面定义实现
*
* 页面：
*   主菜单      Steer PID / Base Speed / Protocol
*   Launch      任务列表（Task2/3/5/6，点选即启动）+ Ball Tgt（任务6 球目标）
*   Steer PID   差速 PID 参数
*   Base Speed  基准 duty
*   Protocol    OpenART 协议调试（START/STOP/CAL + 球位置显示）
*
* 可编辑参数均用 MENU_ITEM_VAL_RANGE 设置真实值域，与 config.c 边界一致。
********************************************************************************************************************/

#include "menu_defs.h"
#include "line_ctrl.h"     /* steer_pid, kd_yaw */
#include "protocol.h"
#include "task_sched.h"    /* task_sched_set */
#include "ball_ctrl.h"     /* ball_target_cm_x10 */


/* ═══════════════════════════════════════════════════════════
 * 页面定义
 * ════════════════════════════════════════════════════════════ */

/* ── Steer PID 页（差速 PID，输出 duty 差；KdYaw=陀螺仪阻尼系数） ── */
static const MenuItem steer_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Kp",  &steer_pid.Kp,     1,  0, 200),
    MENU_ITEM_VAL_RANGE(2, "Ki",  &steer_pid.Ki,     1,  0, 200),
    MENU_ITEM_VAL_RANGE(3, "Kd",  &steer_pid.Kd,     1,  0, 200),
    MENU_ITEM_VAL_RANGE(4, "Max", &steer_pid.OutMax, 50, 0, 2000),
    MENU_ITEM_VAL_RANGE(5, "Min", &steer_pid.OutMin, 50, -2000, 0),
    MENU_ITEM_VAL_RANGE(6, "KdYaw", &kd_yaw, 1, 0, 200),
};
static MenuPage page_steer = MENU_PAGE("Steer PID", steer_items, 6);

/* ── Base Speed 页（基准 duty，0-10000 满占空比） ── */
static const MenuItem speed_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Spd", &base_speed, 100, 0, 4000),
};
static MenuPage page_speed = MENU_PAGE("Base Speed", speed_items, 1);

/* ── Protocol 调试页 ── */
static const MenuItem protocol_items[] = {
    MENU_ITEM(1, "Send START", Protocol_SendStart),
    MENU_ITEM(2, "Send STOP",  Protocol_SendStop),
    MENU_ITEM(3, "Send CAL",   Protocol_SendCal),
    MENU_ITEM_VAL(4, "Ball X",     &proto_ball_x, 1),
    MENU_ITEM_BOOL(5, "Ball Valid", &proto_ball_valid),
};
static MenuPage page_protocol = MENU_PAGE("Protocol", protocol_items, 5);


/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */

static void cb_steer(void)     { Menu_Push(&page_steer); }
static void cb_speed(void)     { Menu_Push(&page_speed); }
static void cb_protocol(void)  { Menu_Push(&page_protocol); }


/* ═══════════════════════════════════════════════════════════
 * Launch 任务选择页 — 点选任务项即启动
 * ════════════════════════════════════════════════════════════ */

uint8 launch_triggered = 0;
int16 base_speed        = 1500;   /* 基准 duty（0-10000 满量程），默认 15% */

static void cb_task2(void) { task_sched_set(TASK_2); launch_triggered = 1; }
static void cb_task3(void) { task_sched_set(TASK_3); launch_triggered = 1; }
static void cb_task5(void) { task_sched_set(TASK_5); launch_triggered = 1; }
static void cb_task6(void) { task_sched_set(TASK_6); launch_triggered = 1; }

static const MenuItem launch_items[] = {
    MENU_ITEM(1, "Task 2",  cb_task2),
    MENU_ITEM(2, "Task 3",  cb_task3),
    MENU_ITEM(3, "Task 5",  cb_task5),
    MENU_ITEM(4, "Task 6",  cb_task6),
    MENU_ITEM_VAL_RANGE(5, "Ball Tgt", &ball_target_cm_x10, 5, -50, 50),
};
MenuPage page_launch = MENU_PAGE("LAUNCH", launch_items, 5);


/* ═══════════════════════════════════════════════════════════
 * 主菜单
 * ════════════════════════════════════════════════════════════ */

static const MenuItem main_items[] = {
    MENU_ITEM(1, "Steer PID",  cb_steer),
    MENU_ITEM(2, "Base Speed", cb_speed),
    MENU_ITEM(3, "Protocol",   cb_protocol),
};
MenuPage page_main = MENU_PAGE("Main Menu", main_items, 3);
