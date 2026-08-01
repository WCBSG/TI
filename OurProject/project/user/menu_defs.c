/*********************************************************************************************************************
* 文件名称          menu_defs.c
* 说明              菜单页面定义实现
*
* 依赖（extern 全局变量）：
*   steer_pid — control.h（唯一 PID，差速）
*   base_speed — 基准 duty（0-10000 满占空比）
*
* 可编辑参数均用 MENU_ITEM_VAL_RANGE 设置真实值域，与 config.c 边界一致，
* 避免越界值保存后被整体判无效。
********************************************************************************************************************/

#include "menu_defs.h"
#include "control.h"
#include "protocol.h"


/* ═══════════════════════════════════════════════════════════
 * 页面定义
 * ════════════════════════════════════════════════════════════ */

/* ── Steer PID 页（唯一 PID，差速输出 duty 差） ── */
static const MenuItem steer_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Kp",  &steer_pid.Kp,     1,  0, 200),
    MENU_ITEM_VAL_RANGE(2, "Ki",  &steer_pid.Ki,     1,  0, 200),
    MENU_ITEM_VAL_RANGE(3, "Kd",  &steer_pid.Kd,     1,  0, 200),
    MENU_ITEM_VAL_RANGE(4, "Max", &steer_pid.OutMax, 50, 0, 2000),
    MENU_ITEM_VAL_RANGE(5, "Min", &steer_pid.OutMin, 50, -2000, 0),
};
static MenuPage page_steer = MENU_PAGE("Steer PID", steer_items, 5);

/* ── Base Speed 页（基准 duty，0-10000 满占空比） ── */
static const MenuItem speed_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Spd", &base_speed, 100, 0, 4000),
};

static const MenuItem protocol_items[] = {
    MENU_ITEM(1, "Send START", Protocol_SendStart),
    MENU_ITEM(2, "Send STOP",  Protocol_SendStop),
    MENU_ITEM(3, "Send CAL",   Protocol_SendCal),

    // 显示小球值
    MENU_ITEM(4, "Ball X",     &proto_ball_x, 1),
    MENU_ITEM_BOOL(5, "Ball Valid", &proto_ball_valid), 
};

static MenuPage page_speed = MENU_PAGE("Base Speed", speed_items, 1);
static MenuPage page_protocol = MENU_PAGE("Protocol", protocol_items, 5);


/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */

static void cb_steer(void)     { Menu_Push(&page_steer); }
static void cb_speed(void)     { Menu_Push(&page_speed); }
static void cb_protocol(void)  { Menu_Push(&page_protocol); }


/* ═══════════════════════════════════════════════════════════
 * Launch 页面
 * ════════════════════════════════════════════════════════════ */

uint8 launch_triggered = 0;
int16 base_speed        = 1500;   /* 基准 duty（0-10000 满量程），默认 15% */

static void cb_launch(void) { launch_triggered = 1; }

static const MenuItem launch_items[] = {
    MENU_ITEM(1, "GO!", cb_launch),
};
MenuPage page_launch = MENU_PAGE("LAUNCH", launch_items, 1);


/* ═══════════════════════════════════════════════════════════
 * 主菜单
 * ════════════════════════════════════════════════════════════ */

static const MenuItem main_items[] = {
    MENU_ITEM(1, "Steer PID",  cb_steer),
    MENU_ITEM(2, "Base Speed", cb_speed),
    MENU_ITEM(3, "Protocol", cb_protocol),
};
MenuPage page_main = MENU_PAGE("Main Menu", main_items, 3);