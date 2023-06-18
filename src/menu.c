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

Menu *create_menu(WM *wm, int n, int col, int w, int h, int pad, unsigned long bg)
{
    Menu *menu=malloc_s(sizeof(Menu));
    menu->n=n, menu->col=col, menu->row=(n+col-1)/col;
    menu->w=w, menu->h=h, menu->pad=pad, menu->bg=bg;
    menu->win=XCreateSimpleWindow(wm->display, wm->root_win,
        0, 0, w*col+2*pad, h*menu->row+2*pad, 0, 0, bg);
    set_override_redirect(wm, menu->win);

    menu->items=malloc_s(n*sizeof(Window));
    for(int i=0; i<n; i++)
    {
        menu->items[i]=XCreateSimpleWindow(wm->display, menu->win,
            pad+w*(i%col), pad+h*(i/col), w, h, 0, 0, bg);
        XSelectInput(wm->display, menu->items[i], BUTTON_EVENT_MASK);
    }

    XMapSubwindows(wm->display, menu->win);
    return menu;
}

void show_menu(WM *wm, XEvent *e, Menu *menu, Window bind)
{
    if(e->type == ButtonPress)
    {
        XButtonEvent *b=&e->xbutton;
        set_pos_for_click(wm, bind, b->x_root-b->x, &menu->x, &menu->y,
            menu->w*menu->col+2*menu->pad, menu->h*menu->row+2*menu->pad);
    }
    XMoveWindow(wm->display, menu->win, menu->x, menu->y);
    XMapRaised(wm->display, menu->win);
    XMapWindow(wm->display, menu->win);
}

void update_menu_item_text(WM *wm, Window win)
{
    const char *text=NULL;
    int h=MENU_ITEM_HEIGHT(wm), w=wm->cfg->menu_item_width;
    String_format f={{0, 0, w, h}, CENTER_LEFT, true, true, false, 0,
        TEXT_COLOR(wm, MENU), MENU_FONT};
    Widget_type t=get_widget_type(wm, win);

    if(IS_WIDGET_CLASS(t, ACT_CENTER_ITEM))
        text=wm->cfg->act_center_item_text[WIDGET_INDEX(t, ACT_CENTER_ITEM)];
    else
        text=wm->cfg->client_menu_item_text[WIDGET_INDEX(t, CLIENT_MENU_ITEM)];
    draw_string(wm, win, text, &f);
}
