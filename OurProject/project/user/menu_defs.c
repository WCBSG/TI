/*********************************************************************************************************************
* 文件名称          menu_defs.c
* 说明              菜单页面定义实现
*
* 页面：
*   主菜单      Steer PID / Spd(直接调) / Ball / Led(直接调)
*   Launch      任务列表（Task2/3/5/6，点选即启动）+ Ball Tgt（任务6 球目标）
*   Steer PID   差速 PID 参数
*   Ball        OpenART 球坐标调试（X / Valid，只读）
*
* 可编辑参数均用 MENU_ITEM_VAL_RANGE 设置真实值域，与 config.c 边界一致。
* Spd/Led 直接挂主菜单编辑（带 value 指针），无需进子页。
********************************************************************************************************************/

#include "menu_defs.h"
#include "line_ctrl.h"     /* steer_pid, kd_yaw */
#include "protocol.h"
#include "task_sched.h"    /* task_sched_set */
#include "ball_ctrl.h"     /* ball_target_cm_x10 */
#include "mileage.h"       /* 里程/弯道过弯参数 */


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

/* ── Ball 调试页（OpenART 球坐标，只读） ── */
static const MenuItem ball_items[] = {
    MENU_ITEM_VAL(1, "Ball X",     &proto_ball_x, 1),
    MENU_ITEM_BOOL(2, "Ball Valid", &proto_ball_valid),
};
static MenuPage page_ball = MENU_PAGE("Ball", ball_items, 2);

/* ── Mileage 页（编码器里程过弯参数，CALM 命令标定 p/cm） ── */
static const MenuItem mile_items[] = {
    MENU_ITEM_VAL_RANGE(1, "P/cm L", &pulses_per_cm_lr, 1, 1, 1000),
    MENU_ITEM_VAL_RANGE(2, "P/cm R", &pulses_per_cm_rr, 1, 1, 1000),
    MENU_ITEM_VAL_RANGE(3, "CvSpd", &curve_spd, 100, 0, 6000),
    MENU_ITEM_VAL_RANGE(4, "CvDiff", &curve_diff, 50, 0, 3000),
    MENU_ITEM_VAL_RANGE(5, "Straight", &straight_len_cm, 10, 50, 300),
    MENU_ITEM_VAL_RANGE(6, "Curve", &curve_len_cm, 5, 50, 300),
};
static MenuPage page_mile = MENU_PAGE("Mileage", mile_items, 6);

/* ── LED 灯带亮度（PA2 PWM 输出，0-10000，直接主菜单编辑） ── */
int16 led_duty = 0;   /* 灯带亮度 duty，main 菜单循环实时写入 PA2 PWM */


/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */

static void cb_steer(void)     { Menu_Push(&page_steer); }
static void cb_ball(void)      { Menu_Push(&page_ball); }
static void cb_mile(void)      { Menu_Push(&page_mile); }


/* ═══════════════════════════════════════════════════════════
 * Launch 任务选择页 — 点选任务项即启动
 * ════════════════════════════════════════════════════════════ */

uint8 launch_triggered = 0;
int16 base_speed        = 3000;   /* 基准 duty（0-10000 满量程），减速电机 30% */

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
    MENU_ITEM(1, "Steer PID",     cb_steer),
    MENU_ITEM_VAL_RANGE(2, "Spd", &base_speed, 100, 0, 6000),
    MENU_ITEM(3, "Ball",          cb_ball),
    MENU_ITEM(4, "Mileage",       cb_mile),
    MENU_ITEM_VAL_RANGE(5, "Led", &led_duty, 100, 0, 10000),
};
MenuPage page_main = MENU_PAGE("Main Menu", main_items, 5);
