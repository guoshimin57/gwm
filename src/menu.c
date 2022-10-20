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
    int ox=menu->x, oy=menu->y;
    if(e->type == ButtonPress)
    {
        XButtonEvent *b=&e->xbutton;
        int x=b->x_root-b->x, y=b->y_root-b->y;
        set_menu_pos_for_click(wm, bind, x, y, menu);
    }
    XMoveWindow(wm->display, menu->win, menu->x, menu->y);
    XMapRaised(wm->display, menu->win);
    XMapWindow(wm->display, menu->win);
    menu->x=ox, menu->y=oy;
}

void set_menu_pos_for_click(WM *wm, Window win, int x, int y, Menu *menu)
{
    unsigned int mh=menu->h*menu->row, mw=menu->w*menu->col,
                 sw=wm->screen_width, w, h;

    get_drawable_size(wm, win, &w, &h);

    if(x < 0) // win左邊出屏
        w=x+w, x=0;
    if(x+w > sw) // win右邊出屏
        w=sw-x;

    if(x+mw <= wm->screen_width) // 在win的右邊能顯示完整的菜單
        menu->x=x;
    else if(x+w >= mw) // 在win的左邊能顯示完整的菜單
        menu->x=x+w-mw;
    else if(x+w/2 <= wm->screen_width/2) // win在屏幕的左半部
        menu->x=wm->screen_width-mw;
    else // win在屏幕的右半部
        menu->x=0;

    if(y+h+mh <= wm->screen_height) // 在win下能顯示完整的菜單
        menu->y=y+h;
    else if(y >= mh) // 在win上能顯示完整的菜單
        menu->y=y-mh;
    else if(y+h/2 <= wm->screen_height/2) // win在屏幕的上半部
        menu->y=wm->screen_height-mh;
    else // win在屏幕的下半部
        menu->y=0;
}
