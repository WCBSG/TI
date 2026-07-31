/*********************************************************************************************************************
* 文件名称          menu_defs.c
* 说明              菜单页面定义实现
*
* 依赖（extern 全局变量）：
*   motor1_pid, motor2_pid — PID.h
*   steer_pid              — control.h
********************************************************************************************************************/

#include "menu_defs.h"
#include "PID.h"
#include "control.h"
#include "config.h"


/* ═══════════════════════════════════════════════════════════
 * 页面定义
 * ════════════════════════════════════════════════════════════ */

/* ── Motor PID 页（两电机共用 Kp/Ki/Kd） ── */
static const MenuItem motor_pid_items[] = {
    MENU_ITEM_VAL(1, "Kp", &motor1_pid.Kp, 1),
    MENU_ITEM_VAL(2, "Ki", &motor1_pid.Ki, 1),
    MENU_ITEM_VAL(3, "Kd", &motor1_pid.Kd, 1),
};
static MenuPage page_motor_pid = MENU_PAGE("Motor PID", motor_pid_items, 3);

/* ── Steer PID 页 ── */
static const MenuItem steer_items[] = {
    MENU_ITEM_VAL(1, "Kp",  &steer_pid.Kp,     1),
    MENU_ITEM_VAL(2, "Ki",  &steer_pid.Ki,     1),
    MENU_ITEM_VAL(3, "Kd",  &steer_pid.Kd,     1),
    MENU_ITEM_VAL(4, "Max", &steer_pid.OutMax, 5),
    MENU_ITEM_VAL(5, "Min", &steer_pid.OutMin, 5),
};
static MenuPage page_steer = MENU_PAGE("Steer PID", steer_items, 5);

/* ── Base Speed 页（独立全局变量，不再复用 motor1_pid.Target） ── */
static const MenuItem speed_items[] = {
    MENU_ITEM_VAL(1, "Spd", &base_speed, 10),
};
static MenuPage page_speed = MENU_PAGE("Base Speed", speed_items, 1);

/* ── IR Test 页（实时显示 8 路传感器值，count=0 无菜单项，由 IRPHOTO_TestSync 绘制） ── */
MenuPage page_ir_test = MENU_PAGE("IR Sensors", NULL, 0);


/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */

static void cb_motor_pid(void) { Menu_Push(&page_motor_pid); }
static void cb_steer(void)     { Menu_Push(&page_steer);     }
static void cb_speed(void)     { Menu_Push(&page_speed);     }

static void cb_ir_test(void) { Menu_Push(&page_ir_test); }


/* ═══════════════════════════════════════════════════════════
 * Launch 页面
 * ════════════════════════════════════════════════════════════ */

uint8 launch_triggered = 0;
int16 base_speed        = 30;    /* 全局基速，菜单 Base Speed 页编辑 */

static void cb_launch(void) { launch_triggered = 1; }

static const MenuItem launch_items[] = {
    MENU_ITEM(1, "GO!", cb_launch),
};
MenuPage page_launch = MENU_PAGE("LAUNCH", launch_items, 1);


/* ═══════════════════════════════════════════════════════════
 * 主菜单
 * ════════════════════════════════════════════════════════════ */

static const MenuItem main_items[] = {
    MENU_ITEM(1, "Motor PID",  cb_motor_pid),
    MENU_ITEM(2, "Steer PID",  cb_steer),
    MENU_ITEM(3, "Base Speed", cb_speed),
    MENU_ITEM(4, "IR Test",    cb_ir_test),
};
MenuPage page_main = MENU_PAGE("Main Menu", main_items, 4);


