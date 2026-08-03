/*********************************************************************************************************************
* 文件名称          menu_defs.c
* 说明              菜单页面定义实现
*
* 页面：
*   主菜单     Steer T2 / Steer 5/6 / Ball Cal
*   Steer T2   任务 2 巡线参数（更快）
*   Steer 5/6  任务 5/6 巡线参数（更稳）
*   Ball Cal   球稳标定（Center / Pix0 / P/cm / BallPx 实时球位）
*   Launch     任务列表（Task 2/3/5/6，点选即启动）+ Ball Tgt（任务6 球目标）
*
* 参数均用 MENU_ITEM_VAL_RANGE 设置真实值域；无断电保持（config 已删），
* 运行中修改重启恢复代码默认值。
********************************************************************************************************************/

#include "menu_defs.h"
#include "line_ctrl.h"     /* kp_t2 / kp_ot 等巡线参数 */
#include "task_sched.h"    /* task_sched_set */
#include "servo.h"         /* servo_center_duty / pixel_zero / px_per_cm / ball_target_cm_x10 */
#include "protocol.h"      /* proto_ball_x（Ball Cal 页实时球位） */

/* ═══════════════════════════════════════════════════════════
 * 页面定义
 * ════════════════════════════════════════════════════════════ */

/* ── Steer T2 页：任务 2 巡线参数（更快） ── */
static const MenuItem steer2_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Kp",     &kp_t2,        10,   0,   500),
    MENU_ITEM_VAL_RANGE(2, "Smooth", &smooth_t2,     1,   0,   100),
    MENU_ITEM_VAL_RANGE(3, "Lim",    &lim_t2,       10, 100,   200),
    MENU_ITEM_VAL_RANGE(4, "Decel",  &dec_t2,        5,   0,    50),
    MENU_ITEM_VAL_RANGE(5, "Spd",    &base_speed_t2, 100, 0,  6000),
};
static MenuPage page_steer2 = MENU_PAGE("Steer T2", steer2_items, 5);

/* ── Steer 5/6 页：任务 5/6 巡线参数（更稳） ── */
static const MenuItem steer56_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Kp",     &kp_ot,        10,   0,   500),
    MENU_ITEM_VAL_RANGE(2, "Smooth", &smooth_ot,     1,   0,   100),
    MENU_ITEM_VAL_RANGE(3, "Lim",    &lim_ot,       10, 100,   200),
    MENU_ITEM_VAL_RANGE(4, "Decel",  &dec_ot,        5,   0,    50),
    MENU_ITEM_VAL_RANGE(5, "Spd",    &base_speed_ot, 100, 0,  6000),
};
static MenuPage page_steer56 = MENU_PAGE("Steer 5/6", steer56_items, 5);

/* ── Ball Cal 页：球稳标定（servo 像素域 PID 宏定死，现场只调标定）
 *   BallPx 只读项：实时显示球像素 X（main 每 100ms 刷新），标定用：
 *     球放 O → 读 BallPx → 填 Pix0；球移 ±5cm → 读像素差 ÷5 → 填 P/cm */
static const MenuItem ballpid_items[] = {
    MENU_ITEM_VAL_RANGE(1, "Center", &servo_center_duty, 10, SERVO_MIN, SERVO_MAX),
    MENU_ITEM_VAL_RANGE(2, "Pix0",   &pixel_zero,         1,  0,        320),
    MENU_ITEM_VAL_RANGE(3, "P/cm",   &px_per_cm,          1,  1,        100),
    MENU_ITEM_VAL_RANGE(4, "BallPx", &proto_ball_x,       0,  0,        319),
};
static MenuPage page_ballpid = MENU_PAGE("Ball Cal", ballpid_items, 4);

/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */
static void cb_steer2(void)  { Menu_Push(&page_steer2); }
static void cb_steer56(void) { Menu_Push(&page_steer56); }
static void cb_ballpid(void) { Menu_Push(&page_ballpid); }

/* ═══════════════════════════════════════════════════════════
 * Launch 任务选择页 — 点选任务项即启动
 * ════════════════════════════════════════════════════════════ */
uint8 launch_triggered = 0;
int16 base_speed        = 3000;   /* 当前激活基准 duty（任务启动时应用对应任务值） */
int16 base_speed_t2     = 4000;   /* 任务 2：40% 占空比 */
int16 base_speed_ot     = 2100;   /* 其他任务：21% 占空比 */

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
    MENU_ITEM(1, "Steer T2",  cb_steer2),
    MENU_ITEM(2, "Steer 5/6", cb_steer56),
    MENU_ITEM(3, "Ball Cal",  cb_ballpid),
};
MenuPage page_main = MENU_PAGE("Main Menu", main_items, 3);
