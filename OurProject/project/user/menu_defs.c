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
#include "Motor.h"


/* ═══════════════════════════════════════════════════════════
 * 页面定义
 * ════════════════════════════════════════════════════════════ */

/* ── Motor1 PID 页 ── */
static const MenuItem motor1_items[] = {
    MENU_ITEM_VAL(1, "Kp",  &motor1_pid.Kp,     1),
    MENU_ITEM_VAL(2, "Ki",  &motor1_pid.Ki,     1),
    MENU_ITEM_VAL(3, "Kd",  &motor1_pid.Kd,     1),
    MENU_ITEM_VAL(4, "Spd", &motor1_pid.Target, 10),
};
static MenuPage page_motor1 = MENU_PAGE("Motor1 PID", motor1_items, 4);

/* ── Motor2 PID 页 ── */
static const MenuItem motor2_items[] = {
    MENU_ITEM_VAL(1, "Kp",  &motor2_pid.Kp,     1),
    MENU_ITEM_VAL(2, "Ki",  &motor2_pid.Ki,     1),
    MENU_ITEM_VAL(3, "Kd",  &motor2_pid.Kd,     1),
    MENU_ITEM_VAL(4, "Spd", &motor2_pid.Target, 10),
};
static MenuPage page_motor2 = MENU_PAGE("Motor2 PID", motor2_items, 4);

/* ── Steer PID 页 ── */
static const MenuItem steer_items[] = {
    MENU_ITEM_VAL(1, "Kp",  &steer_pid.Kp,     1),
    MENU_ITEM_VAL(2, "Ki",  &steer_pid.Ki,     1),
    MENU_ITEM_VAL(3, "Kd",  &steer_pid.Kd,     1),
    MENU_ITEM_VAL(4, "Max", &steer_pid.OutMax, 5),
    MENU_ITEM_VAL(5, "Min", &steer_pid.OutMin, 5),
};
static MenuPage page_steer = MENU_PAGE("Steer PID", steer_items, 5);

/* ── Base Speed 页 ── */
static const MenuItem speed_items[] = {
    MENU_ITEM_VAL(1, "Spd", &motor1_pid.Target, 10),
};
static MenuPage page_speed = MENU_PAGE("Base Speed", speed_items, 1);

/* ── Motor Debug 页（直接调节占空比，driving=0 绕过 PID） ── */
static const MenuItem motor_debug_items[] = {
    MENU_ITEM_VAL(1, "M1 Duty", &motor1_duty, 50),
    MENU_ITEM_VAL(2, "M2 Duty", &motor2_duty, 50),
};
MenuPage page_motor_debug = MENU_PAGE("Motor Debug", motor_debug_items, 2);


/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */

static void cb_motor1(void) { Menu_Push(&page_motor1); }
static void cb_motor2(void) { Menu_Push(&page_motor2); }
static void cb_steer(void)  { Menu_Push(&page_steer);  }
static void cb_speed(void)  { Menu_Push(&page_speed);  }

static void cb_motor_debug(void)
{
    driving       = 0;              /* 关闭 PID 驱动 */
    motor1_duty   = 0;
    motor2_duty   = 0;
    motor1_control(0);              /* 进入调试页时停止电机 */
    motor2_control(0);
    Menu_Push(&page_motor_debug);
}


/* ═══════════════════════════════════════════════════════════
 * Launch 页面
 * ════════════════════════════════════════════════════════════ */

uint8 launch_triggered = 0;

static void cb_launch(void) { launch_triggered = 1; }

static const MenuItem launch_items[] = {
    MENU_ITEM(1, "GO!", cb_launch),
};
MenuPage page_launch = MENU_PAGE("LAUNCH", launch_items, 1);


/* ═══════════════════════════════════════════════════════════
 * 主菜单
 * ════════════════════════════════════════════════════════════ */

static const MenuItem main_items[] = {
    MENU_ITEM(1, "Motor1 PID",  cb_motor1),
    MENU_ITEM(2, "Motor2 PID",  cb_motor2),
    MENU_ITEM(3, "Steer PID",   cb_steer),
    MENU_ITEM(4, "Base Speed",  cb_speed),
    MENU_ITEM(5, "Motor Debug", cb_motor_debug),
};
MenuPage page_main = MENU_PAGE("Main Menu", main_items, 5);


/* ═══════════════════════════════════════════════════════════ */

void MenuDefs_Init(void)
{
    /* 预留：未来可在此做菜单相关的初始化 */
}
