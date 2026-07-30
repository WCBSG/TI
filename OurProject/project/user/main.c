#include "zf_common_headfile.h"
#include "WcTFT180.h"
#include "WcMenu.h"
#include "control.h"
#include "IRPHOTO.h"
#include "Motor.h"
#include "PID.h"
#include "KEY.h"
#include "config.h"


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


/* ═══════════════════════════════════════════════════════════
 * 回调：子页导航
 * ════════════════════════════════════════════════════════════ */

static void cb_motor1(void) { Menu_Push(&page_motor1); }
static void cb_motor2(void) { Menu_Push(&page_motor2); }
static void cb_steer(void)  { Menu_Push(&page_steer);  }
static void cb_speed(void)  { Menu_Push(&page_speed);  }


/* ═══════════════════════════════════════════════════════════
 * Launch 页面
 * ════════════════════════════════════════════════════════════ */

static uint8 launch_triggered = 0;

static const MenuItem launch_items[] = {
    MENU_ITEM(1, "GO!", NULL),     /* callback 由按键逻辑处理 */
};
static MenuPage page_launch = MENU_PAGE("LAUNCH", launch_items, 1);


/* ═══════════════════════════════════════════════════════════
 * 主菜单
 * ════════════════════════════════════════════════════════════ */

static const MenuItem main_items[] = {
    MENU_ITEM(1, "Motor1 PID", cb_motor1),
    MENU_ITEM(2, "Motor2 PID", cb_motor2),
    MENU_ITEM(3, "Steer PID",  cb_steer),
    MENU_ITEM(4, "Base Speed", cb_speed),
};
static MenuPage page_main = MENU_PAGE("Main Menu", main_items, 4);


/* ═══════════════════════════════════════════════════════════ */
void main(void)
{
    int s[8];

    clock_init(SYSTEM_CLOCK_96M);
    debug_init();
    WcTFT_Init();

    IRPHOTO_Init();
    button_init();
    Motor_Init();
    encoder_init();
    motor1_pid_init();
    motor2_pid_init();
    steer_init();
    config_load();

    pit_ms_init(PIT_ENCODER, 5, pit_handler);

    /* ── 启动主菜单 ── */
    Menu_Init();
    Menu_Push(&page_main);

    /* ── 主循环：按键 → 菜单 ── */
    launch_triggered = 0;
    while (!launch_triggered)
    {
        button_control();

        if (key1_flag) { key1_flag = 0; Menu_Inc();           }  /* Key1: 上/+ */
        if (key2_flag) { key2_flag = 0; Menu_Dec();           }  /* Key2: 下/- */
        if (key3_flag) { key3_flag = 0; Menu_Edit();          }  /* Key3: 确定/编辑 */
        if (key4_flag) { key4_flag = 0; Menu_Cancel();        }  /* Key4: 取消/返回 */

        if (key5_flag)
        {
            key5_flag = 0;

            /* 主菜单按 Key5 → 进入 Launch */
            if (Menu_IsTop(&page_main))  // 简化检查
            {
                /* 保存配置并进入 Launch */
                config_save();
                Menu_Push(&page_launch);
            }
            else
            {
                Menu_Home(&page_main);
            }
        }

        /* Launch 页按 Key3 触发发车 */
        if (Menu_IsTop(&page_launch) && key3_flag)
        {
            key3_flag = 0;
            launch_triggered = 1;
        }

        system_delay_ms(50);
    }

    /* ── 行驶阶段 ── */
    WcTFT_Clear(RGB565_WHITE);
    motor1_pid.Target = motor2_pid.Target;  /* 确保两轮同速 */

    while (1)
    {
        IRPHOTO_Read(s);
        IRPHOTO_Display(s);

        if (is_stop(s))
        {
            motor1_control(0);
            motor2_control(0);
            break;
        }

        steer_set(calc_error(s), motor1_pid.Target);
        system_delay_ms(10);
    }
}
