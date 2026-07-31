/*********************************************************************************************************************
* 文件名称          WcMenu.c
* 说明              栈式菜单实现 — 5 键导航、参数编辑（带值域钳位）
*
* 绘制约定（128×160 PORTRAIT）：
*   正常选中: "> Name: val"  白底黑字
*   编辑中:   ">*Name: val"  蓝底白字
*   普通:     "  Name: val"  白字黑底
********************************************************************************************************************/

#include "WcMenu.h"
#include "WcTFT180.h"

#define STACK_MAX    8
#define ROW_H       16
#define MAX_VISIBLE  9

#define CLR_TITLE_BG   RGB565_BLUE
#define CLR_TITLE_FG   RGB565_WHITE
#define CLR_SEL_BG     RGB565_WHITE
#define CLR_SEL_FG     RGB565_BLACK
#define CLR_EDIT_BG    RGB565_BLUE
#define CLR_EDIT_FG    RGB565_WHITE
#define CLR_NORM_BG    RGB565_BLACK
#define CLR_NORM_FG    RGB565_WHITE

static MenuPage *stack[STACK_MAX];
static int8     top = -1;

static MenuPage* stack_top(void)
{
    return (top >= 0) ? stack[top] : NULL;
}

/* ── 值域钳位 ── */

static int16 clamp_val(int16 v, const MenuItem *item)
{
    if (v < item->min) return item->min;
    if (v > item->max) return item->max;
    return v;
}

/* ── 栈操作 ── */

void Menu_Push(MenuPage *page)
{
    if (!page || top >= STACK_MAX - 1) return;
    page->cursor  = 0;
    page->scroll  = 0;
    page->editing = 0;
    top++;
    stack[top] = page;
    Menu_Draw();
}

void Menu_Pop(void)
{
    if (top <= 0) return;   /* 栈底保留，不允许清空整栈 */
    top--;
    Menu_Draw();
}

void Menu_SwitchTo(MenuPage *page)
{
    if (!page || top < 0) return;
    page->cursor  = 0;
    page->scroll  = 0;
    page->editing = 0;
    stack[top] = page;
    Menu_Draw();
}

void Menu_Clear(void) { top = -1; WcTFT_Clear(RGB565_BLACK); }

/* ── 5 键导航 ── */

/** Key1: 上 / 参数+（带钳位） */
void Menu_Inc(void)
{
    MenuPage *p = stack_top();
    if (!p || p->count == 0) return;

    if (p->editing)
    {
        const MenuItem *item = &p->items[p->cursor];
        if (item->value)
        {
            *item->value = clamp_val((int16)(*item->value + item->step), item);
        }
    }
    else
    {
        if (p->cursor > 0)
        {
            p->cursor--;
            if (p->cursor < p->scroll) p->scroll = p->cursor;
        }
    }
    Menu_Draw();
}

/** Key2: 下 / 参数-（带钳位） */
void Menu_Dec(void)
{
    MenuPage *p = stack_top();
    if (!p || p->count == 0) return;

    if (p->editing)
    {
        const MenuItem *item = &p->items[p->cursor];
        if (item->value)
        {
            *item->value = clamp_val((int16)(*item->value - item->step), item);
        }
    }
    else
    {
        if (p->cursor < p->count - 1)
        {
            p->cursor++;
            if (p->cursor >= p->scroll + MAX_VISIBLE)
                p->scroll = (uint8)(p->cursor - MAX_VISIBLE + 1);
        }
    }
    Menu_Draw();
}

/** Key3: Ok / 进入编辑 / 保存退出 */
void Menu_Edit(void)
{
    MenuPage *p = stack_top();
    const MenuItem *item;

    if (!p || p->count == 0) return;
    item = &p->items[p->cursor];

    if (p->editing)
    {
        p->editing = 0;   /* 编辑态 → 保存退出 */
    }
    else if (item->value)
    {
        p->edit_backup = *item->value;  /* 进入编辑态 */
        p->editing = 1;
    }
    else if (item->callback)
    {
        item->callback();  /* 普通回调 */
    }
    Menu_Draw();
}

/** Key4: 返回 / 取消编辑 */
void Menu_Cancel(void)
{
    MenuPage *p = stack_top();
    if (!p) return;

    if (p->editing)
    {
        /* 还原并退出编辑 */
        const MenuItem *item = &p->items[p->cursor];
        if (item->value) *item->value = p->edit_backup;
        p->editing = 0;
        Menu_Draw();
    }
    else
    {
        Menu_Pop();
    }
}

/** Key5: 子页→回主菜单(清理栈) / 主菜单→由调用方处理 Launch */
void Menu_Home(MenuPage *main_page)
{
    int8 i, main_idx, j, count;

    if (!main_page || top < 0) return;
    if (stack[top]->editing) return;          /* 编辑态禁用 */

    /* 已在主菜单 → 无操作，由 main.c 检测后 Push Launch */
    if (stack[top] == main_page) return;

    /*
     * 清理栈：找到离栈顶最近的主菜单，清除其下方所有元素。
     * 例: [0,2,3,0,1,7,9] → 从栈顶向下找到第二个0(索引3)
     *     → 清除索引0/1/2 → [0,1,7,9] → Push(0) → [0,1,7,9,0]
     */
    main_idx = -1;
    for (i = top; i >= 0; i--)
    {
        if (stack[i] == main_page)
        {
            main_idx = i;
            break;
        }
    }

    if (main_idx >= 0)
    {
        count = (int8)(top - main_idx);
        for (j = 0; j <= count; j++)
            stack[j] = stack[(int8)(main_idx + j)];
        top = count;
    }

    /*
     * 修复：栈满时 Push 会静默失败。
     * 若清理后栈仍满，用主菜单直接覆盖栈顶（等价于 pop + push）。
     */
    if (top >= STACK_MAX - 1)
    {
        top = STACK_MAX - 1;
        stack[top] = main_page;
        main_page->cursor  = 0;
        main_page->scroll  = 0;
        main_page->editing = 0;
        Menu_Draw();
    }
    else
    {
        Menu_Push(main_page);
    }
}

/* ── 其他查询 ── */

void Menu_Init(void) { top = -1; }
uint8 Menu_IsActive(void)  { return (uint8)(top >= 0); }
uint8 Menu_IsEditing(void) { MenuPage *p = stack_top(); return (p && p->editing) ? 1 : 0; }
uint8 Menu_GetDepth(void)  { return (uint8)(top + 1); }
uint8 Menu_GetCurrentId(void) { MenuPage *p = stack_top(); return (p && p->count > 0) ? p->items[p->cursor].id : 0; }
uint8 Menu_IsTop(MenuPage *page) { return (page && stack_top() == page) ? 1 : 0; }

/* ── 绘制 ── */

/** 用空格填充矩形行（比 FillRect 快 ~8×） */
static void fill_row(uint8 y, uint16 fg, uint16 bg)
{
    uint8 c;
    WcTFT_SetColor(fg, bg);
    for (c = 0; c < tft180_x_max; c += 64)
        WcTFT_PrintAt(c, y, "        ");  /* 8 个空格 × 8px = 64px */
}

void Menu_Draw(void)
{
    MenuPage *p = stack_top();
    uint8 i, idx, y, title_len, title_x;

    if (!p) return;

    /* 标题栏 */
    fill_row(0, CLR_TITLE_FG, CLR_TITLE_BG);
    title_len = 0;
    while (p->title[title_len]) title_len++;
    if (title_len > (tft180_x_max / 8)) title_len = (uint8)(tft180_x_max / 8);
    title_x = (uint8)((tft180_x_max - title_len * 8) / 2);
    WcTFT_SetColor(CLR_TITLE_FG, CLR_TITLE_BG);
    WcTFT_PrintAt(title_x, 0, p->title);

    if (p->count == 0) return;

    /* 列表项 */
    for (i = 0; i < MAX_VISIBLE; i++)
    {
        y   = (uint8)((i + 1) * ROW_H);
        idx = p->scroll + i;
        if (idx >= p->count)
        {
            /* 超出行：黑底清空 */
            fill_row(y, CLR_NORM_FG, CLR_NORM_BG);
            continue;
        }

        {
            const MenuItem *it = &p->items[idx];
            uint8 sel = (idx == p->cursor);
            uint8 ed  = p->editing && sel;
            uint16 bg, fg;
            char pf[4];

            if (ed)      { pf[0]='>'; pf[1]='*'; pf[2]=' '; pf[3]=0; bg=CLR_EDIT_BG; fg=CLR_EDIT_FG; }
            else if (sel){ pf[0]='>'; pf[1]=' '; pf[2]=' '; pf[3]=0; bg=CLR_SEL_BG;  fg=CLR_SEL_FG;  }
            else         { pf[0]=' '; pf[1]=' '; pf[2]=' '; pf[3]=0; bg=CLR_NORM_BG; fg=CLR_NORM_FG; }

            /* 清空行背景 */
            fill_row(y, fg, bg);

            /* 内容覆盖 */
            WcTFT_PrintAt(0,  y, pf);
            WcTFT_PrintAt(16, y, it->name);

            if (it->value)
            {
                WcTFT_PrintAt(48, y, ":");
                WcTFT_PrintIntAt(56, y, *it->value);
            }
        }
    }
}
