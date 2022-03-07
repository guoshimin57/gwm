/* *************************************************************************
 *     desktop.c：實現虛擬桌面功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "desktop.h"

void next_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, wm->cur_desktop<DESKTOP_N ? wm->cur_desktop+1 : 1);
}

void prev_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, wm->cur_desktop>1 ? wm->cur_desktop-1 : DESKTOP_N);
}

unsigned int get_desktop_n(WM *wm, XEvent *e, Func_arg arg)
{
    if(e->type == KeyPress)
        return (arg.n>=0 && arg.n<=DESKTOP_N) ? arg.n : 1;
    else if(e->type == ButtonPress)
        return TASKBAR_BUTTON_INDEX(get_widget_type(wm, e->xbutton.window))+1;
    else
        return 1;
}

void focus_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void focus_desktop_n(WM *wm, unsigned int n)
{
    if(n == 0)
        return;

    wm->cur_desktop=n;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_on_cur_desktop(wm, c))
        {
            if(c->area_type == ICONIFY_AREA)
                XMapWindow(wm->display, c->icon->win);
            else
                XMapWindow(wm->display, c->frame);
        }
        else
        {
            if(c->area_type == ICONIFY_AREA)
                XUnmapWindow(wm->display, c->icon->win);
            else
                XUnmapWindow(wm->display, c->frame);
        }
    }

    focus_client(wm, wm->cur_desktop, DESKTOP(wm).cur_focus_client);
    update_layout(wm);
    update_icon_area(wm);
    update_taskbar_buttons(wm);
}

bool is_on_cur_desktop(WM *wm, Client *c)
{
    return (c->desktop_mask & get_desktop_mask(wm->cur_desktop));
}

bool is_on_desktop_n(unsigned int desktop_n, Client *c)
{
    return (c->desktop_mask & get_desktop_mask(desktop_n));
}

unsigned int get_desktop_mask(unsigned int desktop_n)
{
    return 1<<(desktop_n-1);
}

void move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    unsigned int n=get_desktop_n(wm, e, arg);
    Client *pc=DESKTOP(wm).cur_focus_client, *pp=DESKTOP(wm).prev_focus_client;
    if(n && n!=wm->cur_desktop && pc!=wm->clients)
    {
        pc->desktop_mask=get_desktop_mask(n);
        focus_client(wm, n, pc);
        focus_client(wm, wm->cur_desktop, pp);
        focus_desktop_n(wm, wm->cur_desktop);
    }
}

void all_move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    unsigned int n=get_desktop_n(wm, e, arg);
    if(n)
    {
        Client *pc=DESKTOP(wm).cur_focus_client;
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            c->desktop_mask=get_desktop_mask(n);
        for(unsigned int i=1; i<=DESKTOP_N; i++)
            focus_client(wm, i, i==n ? pc : wm->clients);
        focus_desktop_n(wm, wm->cur_desktop);
    }
}

void change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    move_to_desktop(wm, e, arg);
    focus_desktop(wm, e, arg);
}

void all_change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_move_to_desktop(wm, e, arg);
    focus_desktop(wm, e, arg);
}

void attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    unsigned int n=get_desktop_n(wm, e, arg);
    Client *c=DESKTOP(wm).cur_focus_client;
    if(n && n!=wm->cur_desktop && c!=wm->clients)
    {
        c->desktop_mask |= get_desktop_mask(n);
        focus_client(wm, n, c);
    }
}

void attach_to_all_desktops(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=DESKTOP(wm).cur_focus_client;
    if(c != wm->clients)
    {
        c->desktop_mask=~0;
        for(unsigned int i=1; i<=DESKTOP_N; i++)
            if(i != wm->cur_desktop)
                focus_client(wm, i, c);
    }
}

void all_attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    unsigned int n=get_desktop_n(wm, e, arg);
    if(n)
    {
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            c->desktop_mask |= get_desktop_mask(n);
        if(n == wm->cur_desktop)
            focus_desktop_n(wm, wm->cur_desktop);
        else
            focus_client(wm, n, wm->desktop[n-1].cur_focus_client);
    }
}
