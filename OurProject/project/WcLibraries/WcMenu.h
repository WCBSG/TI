/*********************************************************************************************************************
* 文件名称          WcMenu.h
* 说明              栈式菜单系统，支持参数编辑 —— TFT180 显示
*
* 按键模型（4 键，主板 b2/b3/b4/P3.2）：
*   Key1 → Menu_Inc()    正常=上移光标  编辑中=参数+1步长
*   Key2 → Menu_Dec()    正常=下移光标  编辑中=参数-1步长
*   Key3 → Menu_Edit()   正常=Ok/进入编辑  编辑中=保存退出
*   Key4 → Menu_Cancel() 正常=返回上层  编辑中=取消编辑(还原值)
*
* 参数编辑流程：
*   1. 在带 value 指针的菜单项上按 Key3 → 进入编辑态 (editing=1)
*   2. 编辑态中 Key1/Key2 调整值, Key3 保存, Key4 取消还原, Key5 禁用
*   3. 退出编辑态后自动重绘
********************************************************************************************************************/

#ifndef _WcMenu_h_
#define _WcMenu_h_

#include "zf_common_headfile.h"

/* ── 类型定义 ── */

typedef struct {
    uint8       id;
    const char *name;
    void      (*callback)(void);   /* Key3 回调（value!=NULL 则用于进入编辑态） */
    int16      *value;             /* 可编辑值指针（NULL=纯导航项） */
    int16       step;              /* 编辑步长 */
    int16       min;               /* 值下限（含） */
    int16       max;               /* 值上限（含） */
} MenuItem;

typedef struct {
    const char     *title;
    const MenuItem *items;
    uint8           count;
    uint8           cursor;
    uint8           scroll;
    uint8           editing;       /* 0=正常导航, 1=正在编辑当前项的值 */
    int16           edit_backup;   /* 编辑前的值备份（Key4 取消时还原） */
} MenuPage;

/* ── 构造宏 ── */
#define MENU_ITEM(_id, _name, _cb)              { (_id), (_name), (_cb), NULL, 0,     0,      0 }
#define MENU_ITEM_VAL(_id, _name, _val_ptr, _st) { (_id), (_name), NULL, (_val_ptr), (_st), -32768, 32767 }
#define MENU_ITEM_VAL_RANGE(_id, _name, _val_ptr, _st, _min, _max) { (_id), (_name), NULL, (_val_ptr), (_st), (_min), (_max) }
#define MENU_ITEM_BOOL(_id, _name, _val_ptr)      { (_id), (_name), NULL, (_val_ptr), 1,     0,      1 }
#define MENU_PAGE(_title, _items, _cnt)          { (_title), (_items), (_cnt), 0, 0, 0, 0 }

/* ── 初始化和状态 ── */
void     Menu_Init(void);
uint8    Menu_IsActive(void);
uint8    Menu_IsEditing(void);
uint8    Menu_IsTop(MenuPage *page);              /* 给定页指针是否是栈顶 */
uint8    Menu_GetCurrentId(void);
uint8    Menu_GetDepth(void);

/* ── 栈操作 ── */
void     Menu_Push(MenuPage *page);
void     Menu_Pop(void);
void     Menu_SwitchTo(MenuPage *page);
void     Menu_Clear(void);

/* ── 5 键导航 ── */
void     Menu_Inc(void);                        /* Key1: 上移/参数+ */
void     Menu_Dec(void);                        /* Key2: 下移/参数- */
void     Menu_Edit(void);                       /* Key3: Ok/进入编辑/保存退出 */
void     Menu_Cancel(void);                     /* Key4: 返回/取消编辑 */

/* ── 绘制 ── */
void     Menu_Draw(void);

#endif
