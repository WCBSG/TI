/* WcMenu.c — 栈式菜单：4 键导航 + 参数编辑（值域钳位）
 * 绘制：选中 "> Name: val" 白底黑字 / 编辑 ">*Name: val" 蓝底白字 / 普通 "  Name: val" 白字黑底 */
#include "WcMenu.h"
#include "WcTFT180.h"

#define STACK_MAX  8
#define ROW_H      16
#define MAX_VISIBLE 9

#define CLR_TITLE_BG RGB565_BLUE
#define CLR_TITLE_FG RGB565_WHITE
#define CLR_SEL_BG   RGB565_WHITE
#define CLR_SEL_FG   RGB565_BLACK
#define CLR_EDIT_BG  RGB565_BLUE
#define CLR_EDIT_FG  RGB565_WHITE
#define CLR_NORM_BG  RGB565_BLACK
#define CLR_NORM_FG  RGB565_WHITE

static MenuPage *stack[STACK_MAX];
static int8     top = -1;

static MenuPage *stack_top(void)     { return (top >= 0) ? stack[top] : NULL; }
static int16 clamp_val(int16 v, const MenuItem *it)
{ return (v < it->min) ? it->min : (v > it->max) ? it->max : v; }

void   Menu_Init(void) { top = -1; }
uint8  Menu_IsTop(MenuPage *p) { return (p && stack_top() == p) ? 1 : 0; }

void Menu_Push(MenuPage *p)
{
    if (!p || top >= STACK_MAX - 1) return;
    p->cursor = p->scroll = p->editing = 0;
    stack[++top] = p;
    Menu_Draw();
}

void Menu_Pop(void)
{
    if (top <= 0) return;   /* 栈底保留 */
    top--;
    Menu_Draw();
}

void Menu_Inc(void)   /* Key1: 上 / 参数+ */
{
    MenuPage *p = stack_top();
    if (!p || p->count == 0) return;
    if (p->editing)
    {
        const MenuItem *it = &p->items[p->cursor];
        if (it->value) *it->value = clamp_val((int16)(*it->value + it->step), it);
    }
    else if (p->cursor > 0)
    {
        p->cursor--;
        if (p->cursor < p->scroll) p->scroll = p->cursor;
    }
    Menu_Draw();
}

void Menu_Dec(void)   /* Key2: 下 / 参数- */
{
    MenuPage *p = stack_top();
    if (!p || p->count == 0) return;
    if (p->editing)
    {
        const MenuItem *it = &p->items[p->cursor];
        if (it->value) *it->value = clamp_val((int16)(*it->value - it->step), it);
    }
    else if (p->cursor < p->count - 1)
    {
        p->cursor++;
        if (p->cursor >= p->scroll + MAX_VISIBLE) p->scroll = (uint8)(p->cursor - MAX_VISIBLE + 1);
    }
    Menu_Draw();
}

void Menu_Edit(void)  /* Key3: Ok / 进入编辑 / 保存退出 */
{
    MenuPage *p = stack_top();
    const MenuItem *it;
    if (!p || p->count == 0) return;
    it = &p->items[p->cursor];
    if (p->editing)         p->editing = 0;
    else if (it->value)     { p->edit_backup = *it->value; p->editing = 1; }
    else if (it->callback)  it->callback();
    Menu_Draw();
}

void Menu_Cancel(void)  /* Key4: 返回 / 取消编辑（还原） */
{
    MenuPage *p = stack_top();
    if (!p) return;
    if (p->editing)
    {
        const MenuItem *it = &p->items[p->cursor];
        if (it->value) *it->value = p->edit_backup;
        p->editing = 0;
        Menu_Draw();
    }
    else Menu_Pop();
}

/* 空格清整行：32 空格，WcTFT_PrintAt 自动截断 */
static void fill_row(uint8 y, uint16 fg, uint16 bg)
{ WcTFT_SetColor(fg, bg); WcTFT_PrintAt(0, y, "                                "); }

void Menu_Draw(void)
{
    MenuPage *p = stack_top();
    uint8 i, idx, y, title_len, title_x;
    if (!p) return;

    fill_row(0, CLR_TITLE_FG, CLR_TITLE_BG);
    for (title_len = 0; p->title[title_len]; title_len++) ;
    if (title_len > (tft180_x_max / 8)) title_len = (uint8)(tft180_x_max / 8);
    title_x = (uint8)((tft180_x_max - title_len * 8) / 2);
    WcTFT_SetColor(CLR_TITLE_FG, CLR_TITLE_BG);
    WcTFT_PrintAt(title_x, 0, p->title);

    if (p->count == 0) return;
    for (i = 0; i < MAX_VISIBLE; i++)
    {
        const MenuItem *it;
        uint8 sel, ed;
        uint16 bg, fg;
        char pf[4];
        y   = (uint8)((i + 1) * ROW_H);
        idx = p->scroll + i;
        if (idx >= p->count) { fill_row(y, CLR_NORM_FG, CLR_NORM_BG); continue; }
        it  = &p->items[idx];
        sel = (idx == p->cursor);
        ed  = p->editing && sel;
        if (ed)       { pf[0]='>'; pf[1]='*'; pf[2]=' '; pf[3]=0; bg=CLR_EDIT_BG; fg=CLR_EDIT_FG; }
        else if (sel) { pf[0]='>'; pf[1]=' '; pf[2]=' '; pf[3]=0; bg=CLR_SEL_BG;  fg=CLR_SEL_FG;  }
        else          { pf[0]=' '; pf[1]=' '; pf[2]=' '; pf[3]=0; bg=CLR_NORM_BG; fg=CLR_NORM_FG; }
        fill_row(y, fg, bg);
        WcTFT_PrintAt(0, y, pf);
        WcTFT_PrintAt(16, y, it->name);
        if (it->value)   /* 原生 tft180 定宽 6 字符，像素无残留 */
        {
            tft180_set_color(fg, bg);
            tft180_show_string(48, y, ":");
            tft180_show_int16(56, y, *it->value);
        }
    }
}