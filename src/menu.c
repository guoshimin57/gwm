/* *************************************************************************
 *     menu.c：實現菜單功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

Menu *act_center=NULL, *client_menu=NULL; // 操作中心, 客戶窗口菜單

Menu *create_menu(const char *item_text[], int n, int col)
{
    Menu *menu=malloc_s(sizeof(Menu));
    int w=0, maxw=0, sw=xinfo.screen_width, pad=get_font_pad();

    for(int i=0; i<n; i++, maxw = w>maxw ? w : maxw)
        get_string_size(item_text[i], &w, NULL);
    w = ((maxw+2*pad)*col > sw) ? sw/col : maxw+2*pad;

    menu->n=n, menu->col=col, menu->row=(n+col-1)/col;
    menu->x=menu->y=0, menu->w=w, menu->h=get_font_height_by_pad(), menu->pad=pad;
    menu->bg=get_widget_color(MENU_COLOR);
    menu->win=create_widget_win(xinfo.root_win, 0, 0, w*col,
        menu->h*menu->row, 0, 0, menu->bg);
    menu->items=malloc_s(n*sizeof(Window));
    for(int i=0; i<n; i++)
    {
         menu->items[i]=create_widget_win(menu->win, w*(i%col),
             menu->h*(i/col), w, menu->h, 0, 0, menu->bg);
        XSelectInput(xinfo.display, menu->items[i], BUTTON_EVENT_MASK);
    }

    XMapSubwindows(xinfo.display, menu->win);
    return menu;
}

void show_menu(XEvent *e, Menu *menu, Window bind)
{
    if(e->type == ButtonPress)
    {
        XButtonEvent *b=&e->xbutton;
        set_pos_for_click(bind, b->x_root-b->x, &menu->x, &menu->y,
            menu->w*menu->col, menu->h*menu->row);
    }
    XMoveWindow(xinfo.display, menu->win, menu->x, menu->y);
    XMapRaised(xinfo.display, menu->win);
    XMapWindow(xinfo.display, menu->win);
}

void update_menu_bg(Menu *menu, int n)
{
    unsigned long bg=get_widget_color(MENU_COLOR);
    update_win_bg(menu->win, bg, None);
    for(int i=0; i<n; i++)
        update_win_bg(menu->items[i], bg, None);
}

void update_menu_item_fg(Menu *menu, int i)
{
    const char *text=NULL;

    if(menu == act_center)
        text=cfg->act_center_item_text[i];
    else if(menu == client_menu)
        text=cfg->client_menu_item_text[i];
    else
        return;

    Str_fmt fmt={0, 0, menu->w, menu->h, CENTER_LEFT, true, false, 0,
        get_text_color(MENU_TEXT_COLOR)};
    draw_string(menu->items[i], text, &fmt);
}

void destroy_menu(Menu *menu)
{
    XDestroyWindow(xinfo.display, menu->win);
    vfree(menu->items, menu, NULL);
}
