/*********************************************************************************************************************
* 文件名称          task_sched.h
* 说明              任务框架 — 任务选择、统一运行、结果返回
*
* 任务表（比赛要求）：
*   TASK_2  巡线一圈停车
*   TASK_3  球 O→+5cm→-5cm 往返稳定（小车静止）
*   TASK_5  巡线一圈 + 球稳 O
*   TASK_6  巡线一圈 + 球稳指定位置（ball_target_cm_x10）
*
* 用法：
*   task_sched_set(task_id);   选中任务（Launch 菜单）
*   task_sched_run();          阻塞运行直至完成 + 显示结果 + Key4 返回
********************************************************************************************************************/

#ifndef __TASK_SCHED_H__
#define __TASK_SCHED_H__

#include "zf_common_headfile.h"

typedef enum {
    TASK_IDLE = 0,
    TASK_2,    /* 巡线一圈 */
    TASK_3,    /* 球 O→+5→-5 往返 */
    TASK_4,    /* A→B 球稳 O，跑 8s */
    TASK_5,    /* 一圈 + 球稳 O */
    TASK_6     /* 一圈 + 球稳指定位置 */
} TaskId;

typedef enum {
    TASK_RESULT_RUNNING = 0,
    TASK_RESULT_OK,        /* 任务完成 */
    TASK_RESULT_FAIL,      /* 任务失败（脱轨等） */
    TASK_RESULT_TIMEOUT    /* 超时 */
} TaskResult;

void task_sched_set(int task_id);
int  task_sched_get(void);
void task_sched_run(void);      /* 阻塞运行所选任务直至完成，显示结果页 */
int  task_sched_result(void);   /* 最近一次任务结果 */

#endif
