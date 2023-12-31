/* *************************************************************************
 *     desktop.c：實現虛擬桌面功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

typedef enum op_type_tag { MOVE_TO_N, CHANGE_TO_N, ATTACH_TO_N, ATTACH_TO_ALL } Op_type;

static void ready_to_desktop_n(WM *wm, Client *c, unsigned int n, Op_type op);

void init_desktop(WM *wm)
{
    wm->cur_desktop=cfg->default_cur_desktop;
    for(size_t i=0; i<DESKTOP_N; i++)
    {
        Desktop *d=wm->desktop[i]=malloc_s(sizeof(Desktop));
        d->n_main_max=cfg->default_n_main_max;
        d->cur_layout=d->prev_layout=cfg->default_layout;
        d->default_place_type=TILE_LAYER_MAIN;
        d->main_area_ratio=cfg->default_main_area_ratio;
        d->fixed_area_ratio=cfg->default_fixed_area_ratio;
    }
}

unsigned int get_desktop_n(XEvent *e, Func_arg arg)
{
    if(e->type == KeyPress)
        return (arg.n>=0 && arg.n<=DESKTOP_N) ? arg.n : 1;
    if(e->type == ButtonPress)
        return WIDGET_INDEX(get_widget_type(e->xbutton.window), TASKBAR_BUTTON)+1;
    return 1;
}

void focus_desktop_n(WM *wm, unsigned int n)
{
    if(n == 0) // n=0表示所有虛擬桌面，僅適用於attach_to_all_desktops
        return;

    wm->cur_desktop=n;
    set_net_current_desktop(wm->cur_desktop-1);

    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_on_cur_desktop(wm, c))
        {
            if(c->icon)
                XMapWindow(xinfo.display, c->icon->win);
            else
                XMapWindow(xinfo.display, c->frame);
        }
        else
        {
            if(c->icon)
                XUnmapWindow(xinfo.display, c->icon->win);
            else
                XUnmapWindow(xinfo.display, c->frame);
        }
    }

    focus_client(wm, wm->cur_desktop, CUR_FOC_CLI(wm));
    request_layout_update();
    update_icon_area(wm);
    update_taskbar_buttons_bg();
    set_all_net_client_list(wm);
}

void move_to_desktop_n(WM *wm, Client *c, unsigned int n)
{
    if(!n || n==wm->cur_desktop || c==wm->clients)
        return;

    ready_to_desktop_n(wm, c, n, MOVE_TO_N);
    focus_client(wm, wm->cur_desktop, NULL);
    focus_desktop_n(wm, wm->cur_desktop);
}

static void ready_to_desktop_n(WM *wm, Client *c, unsigned int n, Op_type op)
{
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
    {
        if(op==MOVE_TO_N || op==CHANGE_TO_N)
            p->desktop_mask = get_desktop_mask(n);
        else if(op == ATTACH_TO_N)
            p->desktop_mask |= get_desktop_mask(n);
        else
            p->desktop_mask = ~0;
        focus_client(wm, n, p);
    }
}

void all_move_to_desktop_n(WM *wm, unsigned int n)
{
    if(!n)
        return;

    Client *pc=CUR_FOC_CLI(wm);
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        c->desktop_mask=get_desktop_mask(n);
    for(unsigned int i=1; i<=DESKTOP_N; i++)
        focus_client(wm, i, i==n ? pc : wm->clients);
    focus_desktop_n(wm, wm->cur_desktop);
}

void change_to_desktop_n(WM *wm, Client *c, unsigned int n)
{
    move_to_desktop_n(wm, c, n);
    focus_desktop_n(wm, n);
}

void all_change_to_desktop_n(WM *wm, unsigned int n)
{
    all_move_to_desktop_n(wm, n);
    focus_desktop_n(wm, n);
}

void attach_to_desktop_n(WM *wm, Client *c, unsigned int n)
{
    if(!n || n==wm->cur_desktop || c==wm->clients)
        return;

    ready_to_desktop_n(wm, c, n, ATTACH_TO_N);
    set_all_net_client_list(wm);
}

void attach_to_desktop_all(WM *wm, Client *c)
{
    if(c == wm->clients)
        return;

    for(unsigned int i=1; i<=DESKTOP_N; i++)
        if(i != wm->cur_desktop)
            ready_to_desktop_n(wm, c, i, ATTACH_TO_ALL);
    set_all_net_client_list(wm);
}

void all_attach_to_desktop_n(WM *wm, unsigned int n)
{
    if(!n)
        return;

    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        c->desktop_mask |= get_desktop_mask(n);
    if(n == wm->cur_desktop)
        focus_desktop_n(wm, wm->cur_desktop);
    else
        focus_client(wm, n, wm->desktop[n-1]->cur_focus_client);
    set_all_net_client_list(wm);
}
