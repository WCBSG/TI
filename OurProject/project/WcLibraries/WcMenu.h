/* WcMenu.h — 栈式菜单系统，支持参数编辑（TFT180 显示）
 * 4 键：Key1 上/+ | Key2 下/- | Key3 确定/编辑/保存 | Key4 返回/取消编辑 */
#ifndef _WcMenu_h_
#define _WcMenu_h_

#include "zf_common_headfile.h"

typedef struct {
    uint8       id;
    const char *name;
    void      (*callback)(void);   /* Key3 回调（value!=NULL 则进入编辑态） */
    int16      *value;             /* 可编辑值指针（NULL=纯导航项） */
    int16       step;              /* 编辑步长 */
    int16       min, max;          /* 值域钳位 */
} MenuItem;

typedef struct {
    const char     *title;
    const MenuItem *items;
    uint8           count;
    uint8           cursor, scroll;
    uint8           editing;       /* 0=导航, 1=编辑 */
    int16           edit_backup;   /* 编辑前值备份（取消时还原） */
} MenuPage;

#define MENU_ITEM(_id, _name, _cb)             { (_id), (_name), (_cb), NULL, 0, 0, 0 }
#define MENU_ITEM_VAL_RANGE(_id,_name,_p,_st,_min,_max) { (_id), (_name), NULL, (_p), (_st), (_min), (_max) }
#define MENU_PAGE(_title, _items, _cnt)         { (_title), (_items), (_cnt), 0, 0, 0, 0 }

void   Menu_Init(void);
uint8  Menu_IsTop(MenuPage *p);
void   Menu_Push(MenuPage *p);
void   Menu_Pop(void);
void   Menu_Inc(void);       /* Key1 */
void   Menu_Dec(void);       /* Key2 */
void   Menu_Edit(void);      /* Key3 */
void   Menu_Cancel(void);    /* Key4 */
void   Menu_Draw(void);

#endif