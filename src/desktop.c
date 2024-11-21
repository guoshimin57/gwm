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

#include "config.h"
#include "prop.h"
#include "taskbar.h"
#include "desktop.h"

typedef enum op_type_tag { MOVE_TO_N, CHANGE_TO_N, ATTACH_TO_N, ATTACH_TO_ALL } Op_type;

static void hide_cur_desktop_clients(WM *wm);
static void show_cur_desktop_clients(WM *wm);
static void ready_to_desktop_n(WM *wm, Client *c, unsigned int n, Op_type op);

void init_desktop(WM *wm)
{
    set_net_current_desktop(cfg->default_cur_desktop);
    for(size_t i=0; i<DESKTOP_N; i++)
    {
        Desktop *d=wm->desktop[i]=Malloc(sizeof(Desktop));
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
        return (arg.desktop_n<DESKTOP_N || arg.desktop_n==~0U) ? arg.desktop_n : 0;
    if(e->type == ButtonPress)
        return widget_find(e->xbutton.window)->id-TASKBAR_BUTTON_BEGIN;
    return 1;
}

void focus_desktop_n(WM *wm, unsigned int n)
{
    /* n=~0表示所有虛擬桌面，僅適用於attach_to_all_desktops */
    if(n==~0U || n==get_net_current_desktop())
        return;

    hide_cur_desktop_clients(wm);
    set_net_current_desktop(n);
    show_cur_desktop_clients(wm);
    focus_client(wm, n, CUR_FOC_CLI(wm));
    request_layout_update();
    set_all_net_client_list();
}

static void hide_cur_desktop_clients(WM *wm)
{
    clients_for_each(c)
    {
        if(is_on_cur_desktop(c->desktop_mask))
        {
            if(is_iconic_client(c))
                taskbar_del_client(wm->taskbar, WIDGET_WIN(c));
            else
                widget_hide(WIDGET(c->frame));
        }
    }
}

static void show_cur_desktop_clients(WM *wm)
{
    clients_for_each(c)
    {
        if(is_on_cur_desktop(c->desktop_mask))
        {
            if(is_iconic_client(c))
                taskbar_add_client(wm->taskbar, WIDGET_WIN(c));
            else
                widget_show(WIDGET(c->frame));
        }
    }
}

void move_to_desktop_n(WM *wm, Client *c, unsigned int n)
{
    unsigned int cur_desktop=get_net_current_desktop();
    if(n==~0U || n==cur_desktop || clients_is_head(c))
        return;

    ready_to_desktop_n(wm, c, n, MOVE_TO_N);
    focus_client(wm, cur_desktop, NULL);
    focus_desktop_n(wm, cur_desktop);
}

static void ready_to_desktop_n(WM *wm, Client *c, unsigned int n, Op_type op)
{
    Client *ld=c->subgroup_leader;
    unsigned int mask=get_desktop_mask(n);
    for(Client *p=ld; ld && p->subgroup_leader==ld; p=list_prev_entry(p, Client, list))
    {
        if(op==MOVE_TO_N || op==CHANGE_TO_N)
            p->desktop_mask = mask;
        else if(op == ATTACH_TO_N)
            p->desktop_mask |= mask;
        else
            p->desktop_mask = ~0U;
        focus_client(wm, n, p);
    }
}

void all_move_to_desktop_n(WM *wm, unsigned int n)
{
    if(n == ~0U)
        return;

    Client *pc=CUR_FOC_CLI(wm);
    unsigned int mask=get_desktop_mask(n);
    clients_for_each(c)
        c->desktop_mask=mask;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        focus_client(wm, i, i==n ? pc : get_clients());
    focus_desktop_n(wm, get_net_current_desktop());
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
    if(n==~0U || n==get_net_current_desktop() || clients_is_head(c))
        return;

    ready_to_desktop_n(wm, c, n, ATTACH_TO_N);
    set_all_net_client_list();
}

void attach_to_desktop_all(WM *wm, Client *c)
{
    if(clients_is_head(c))
        return;

    unsigned int cur_desktop=get_net_current_desktop();
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if(i != cur_desktop)
            ready_to_desktop_n(wm, c, i, ATTACH_TO_ALL);
    set_all_net_client_list();
}

void all_attach_to_desktop_n(WM *wm, unsigned int n)
{
    if(n == ~0U)
        return;

    unsigned int mask=get_desktop_mask(n);
    clients_for_each(c)
        c->desktop_mask |= mask;

    unsigned int cur_desktop=get_net_current_desktop();
    if(n == cur_desktop)
        focus_desktop_n(wm, cur_desktop);
    else
        focus_client(wm, n, wm->desktop[n]->cur_focus_client);
    set_all_net_client_list();
}
