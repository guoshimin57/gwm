/* *************************************************************************
 *     desktop.c：實現虛擬桌面功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
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
static void hide_client(WM *wm, const Client *c);
static void show_cur_desktop_clients(WM *wm);
static void show_client(WM *wm, const Client *c);
static void fix_cur_desktop_clients_bg(WM *wm);
static void fix_client_bg(WM *wm, Client *c);
static void ready_to_desktop_n(WM *wm, Client *c, unsigned int n, Op_type op);

void init_desktop(WM *wm)
{
    set_net_current_desktop(cfg->default_cur_desktop);
    for(size_t i=0; i<DESKTOP_N; i++)
    {
        Desktop *d=wm->desktop[i]=Malloc(sizeof(Desktop));
        d->n_main_max=cfg->default_n_main_max;
        d->cur_layout=d->prev_layout=cfg->default_layout;
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
    request_layout_update();
    fix_cur_desktop_clients_bg(wm);
    show_cur_desktop_clients(wm);
    focus_client(wm, is_exist_client(CUR_FOC_CLI(wm)) ? CUR_FOC_CLI(wm) : NULL);
    set_all_net_client_list();
}

static void hide_cur_desktop_clients(WM *wm)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            hide_client(wm, c);
}

static void hide_client(WM *wm, const Client *c)
{
    if(is_iconic_client(c))
        taskbar_del_client(wm->taskbar, WIDGET_WIN(c));
    else
        widget_hide(WIDGET(c->frame));
}

static void show_cur_desktop_clients(WM *wm)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            show_client(wm, c);
}

static void show_client(WM *wm, const Client *c)
{
    if(is_iconic_client(c))
        taskbar_add_client(wm->taskbar, WIDGET_WIN(c));
    else
        widget_show(WIDGET(c->frame));
}

static void fix_cur_desktop_clients_bg(WM *wm)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            fix_client_bg(wm, c);
}

static void fix_client_bg(WM *wm, Client *c)
{
    Client *fc=CUR_FOC_CLI(wm);
    if(WIDGET_STATE(c).unfocused && c==fc)
    {
        client_set_state_unfocused(c, 0);
        update_client_bg(c);
    }
    else if(WIDGET_STATE(c).unfocused==0 && c!=fc)
    {
        client_set_state_unfocused(c, 1);
        update_client_bg(c);
    }
}

static void ready_to_desktop_n(WM *wm, Client *c, unsigned int n, Op_type op)
{
    Client *ld=c->subgroup_leader;
    unsigned int mask=get_desktop_mask(n);

    for(Client *p=ld; ld && p->subgroup_leader==ld; p=list_prev_entry(p, Client, list))
    {
        switch(op)
        {
            case MOVE_TO_N:
                p->desktop_mask=mask;
                fix_client_bg(wm, c);
                hide_client(wm, p);
                break;
            case CHANGE_TO_N:
                p->desktop_mask=mask; break;
            case ATTACH_TO_N:
                p->desktop_mask |= mask; break;
            case ATTACH_TO_ALL:
                p->desktop_mask=~0U; break;
        }
    }
}

void move_to_desktop_n(WM *wm, unsigned int n)
{
    Client *c=CUR_FOC_CLI(wm);
    if(n==~0U || n==get_net_current_desktop() || clients_is_head(c))
        return;

    ready_to_desktop_n(wm, c, n, MOVE_TO_N);
    focus_client(wm, NULL);
    request_layout_update();
}

void all_move_to_desktop_n(WM *wm, unsigned int n)
{
    if(n == ~0U)
        return;

    clients_for_each(c)
        if(!is_on_desktop_n(n, c->desktop_mask))
            ready_to_desktop_n(wm, c, n, MOVE_TO_N);

    Client *head=get_clients();
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if(i != n)
            wm->desktop[i]->prev_focus_client=wm->desktop[i]->cur_focus_client=head;

    if(n == get_net_current_desktop())
        request_layout_update();
}

void change_to_desktop_n(WM *wm, unsigned int n)
{
    Client *c=CUR_FOC_CLI(wm);
    if(n==~0U || n==get_net_current_desktop() || clients_is_head(c))
        return;

    ready_to_desktop_n(wm, c, n, CHANGE_TO_N);
    focus_client(wm, NULL);
    wm->desktop[n]->cur_focus_client=c;
    focus_desktop_n(wm, n);
}

void all_change_to_desktop_n(WM *wm, unsigned int n)
{
    if(n == ~0U)
        return;

    all_move_to_desktop_n(wm, n);
    focus_desktop_n(wm, n);
}

void attach_to_desktop_n(WM *wm, unsigned int n)
{
    Client *c=CUR_FOC_CLI(wm);
    if(n==~0U || n==get_net_current_desktop() || clients_is_head(c))
        return;

    ready_to_desktop_n(wm, c, n, ATTACH_TO_N);
}

void attach_to_desktop_all(WM *wm)
{
    Client *c=CUR_FOC_CLI(wm);
    if(clients_is_head(c))
        return;

    unsigned int cur_desktop=get_net_current_desktop();
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if(i != cur_desktop)
            ready_to_desktop_n(wm, c, i, ATTACH_TO_ALL);
}

void all_attach_to_desktop_n(unsigned int n)
{
    if(n == ~0U)
        return;

    unsigned int mask=get_desktop_mask(n);
    clients_for_each(c)
        c->desktop_mask |= mask;
}
