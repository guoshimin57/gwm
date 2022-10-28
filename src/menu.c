/* *************************************************************************
 *     menu.c：實現菜單功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "menu.h"
#include "misc.h"

void create_menu(WM *wm, Menu *menu, unsigned int n, unsigned int col, unsigned int w, unsigned int h, unsigned long bg)
{
    menu->n=n, menu->col=col, menu->row=(n+col-1)/col;
    menu->w=w, menu->h=h, menu->bg=bg;
    menu->win=XCreateSimpleWindow(wm->display, wm->root_win,
        0, 0, w*col, h*menu->row, 0, 0, bg);
    set_override_redirect(wm, menu->win);

    menu->items=malloc_s(n*sizeof(Window));
    for(size_t i=0; i<n; i++)
    {
        menu->items[i]=XCreateSimpleWindow(wm->display, menu->win,
            w*(i%col), h*(i/col), w, h, 0, 0, bg);
        XSelectInput(wm->display, menu->items[i], BUTTON_EVENT_MASK);
    }

    XMapSubwindows(wm->display, menu->win);
}

void show_menu(WM *wm, XEvent *e, Menu *menu, Window bind)
{
    if(e->type == ButtonPress)
    {
        XButtonEvent *b=&e->xbutton;
        int x=b->x_root-b->x, y=b->y_root-b->y;
        set_pos_for_click(wm, bind, x, y, &menu->x, &menu->y,
            menu->w*menu->col, menu->h*menu->row);
    }
    XMoveWindow(wm->display, menu->win, menu->x, menu->y);
    XMapRaised(wm->display, menu->win);
    XMapWindow(wm->display, menu->win);
}
