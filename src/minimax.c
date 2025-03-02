/* *************************************************************************
 *     minimax.c：實現窗口最小化、最大化、還原、全屏功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "prop.h"
#include "minimax.h"
#include "client.h"

static void maximize_client(WM *wm, Client *c, Max_way way);
static void set_max_rect(WM *wm, Client *c, Max_way max_way);
static Rect get_vert_max_rect(const WM *wm, const Client *c);
static Rect get_horz_max_rect(const WM *wm, const Client *c);
static Rect get_top_max_rect(const WM *wm);
static Rect get_bottom_max_rect(const WM *wm);
static Rect get_left_max_rect(const WM *wm);
static Rect get_right_max_rect(const WM *wm);
static Client *get_icon_client_head(void);
static void set_fullscreen(WM *wm, Client *c);

void minimize(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    iconify_client(wm, CUR_FOC_CLI(wm)); 
}

void deiconify(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    deiconify_client(wm, CUR_FOC_CLI(wm)); 
}

void max_restore(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);

    if(is_win_state_max(c->win_state))
        restore_client(wm, c);
    else
        maximize_client(wm, c, FULL_MAX);
}

void maximize(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    maximize_client(wm, CUR_FOC_CLI(wm), arg.max_way);
}

static void maximize_client(WM *wm, Client *c, Max_way max_way)
{
    if(!is_win_state_max(c->win_state))
        save_place_info_of_client(c);
    set_max_rect(wm, c, max_way);
    if(is_tiled_client(c))
        move_client(wm, c, NULL, FLOAT_LAYER);
    move_resize_client(c, NULL);
    switch(max_way)
    {
        case VERT_MAX:   c->win_state.vmax=1; break;
        case HORZ_MAX:   c->win_state.hmax=1; break;
        case TOP_MAX:    c->win_state.tmax=1; break;
        case BOTTOM_MAX: c->win_state.bmax=1; break;
        case LEFT_MAX:   c->win_state.lmax=1; break;
        case RIGHT_MAX:  c->win_state.rmax=1; break;
        case FULL_MAX:   c->win_state.vmax=c->win_state.hmax=1; break;
    }
    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

static void set_max_rect(WM *wm, Client *c, Max_way max_way)
{
    int bw=WIDGET_BORDER_W(c->frame),
        w=WIDGET_W(c->frame)+2*bw, h=WIDGET_H(c->frame)+2*bw;
    Rect r=wm->workarea;
    switch(max_way)
    {
        case VERT_MAX:   if(w != r.h) r=get_vert_max_rect(wm, c); break;
        case HORZ_MAX:   if(h != r.w) r=get_horz_max_rect(wm, c); break;
        case TOP_MAX:    r=get_top_max_rect(wm); break;
        case BOTTOM_MAX: r=get_bottom_max_rect(wm); break;
        case LEFT_MAX:   r=get_left_max_rect(wm); break;
        case RIGHT_MAX:  r=get_right_max_rect(wm); break;
        case FULL_MAX:   break;
        default:         return;
    }
    set_client_rect_by_outline(c, r.x, r.y, r.w, r.h);
}

static Rect get_vert_max_rect(const WM *wm, const Client *c)
{
    Rect r=widget_get_outline(WIDGET(c->frame)), w=wm->workarea;
    return (Rect){r.x, w.y, r.w, w.h};
}

static Rect get_horz_max_rect(const WM *wm, const Client *c)
{
    Rect r=widget_get_outline(WIDGET(c->frame)), w=wm->workarea;
    return (Rect){w.x, r.y, w.w, r.h};
}

static Rect get_top_max_rect(const WM *wm)
{
    Rect w=wm->workarea;
    return (Rect){w.x, w.y, w.w, w.h/2};
}

static Rect get_bottom_max_rect(const WM *wm)
{
    Rect w=wm->workarea;
    return (Rect){w.x, w.y+w.h/2, w.w, w.h/2};
}

static Rect get_left_max_rect(const WM *wm)
{
    Rect w=wm->workarea;
    return (Rect){w.x, w.y, w.w/2, w.h};
}

static Rect get_right_max_rect(const WM *wm)
{
    Rect w=wm->workarea;
    return (Rect){w.x+w.w/2, w.y, w.w/2, w.h};
}

void restore_client(WM *wm, Client *c)
{
    restore_place_info_of_client(c);
    move_client(wm, c, NULL, c->place_type);
    if(c->win_state.vmax)
        c->win_state.vmax=0;
    if(c->win_state.hmax)
        c->win_state.hmax=0;
    if(c->win_state.tmax)
        c->win_state.tmax=0;
    if(c->win_state.bmax)
        c->win_state.bmax=0;
    if(c->win_state.lmax)
        c->win_state.lmax=0;
    if(c->win_state.rmax)
        c->win_state.rmax=0;
    if(c->win_state.fullscreen)
        c->win_state.fullscreen=0;

    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

void iconify_client(WM *wm, Client *c)
{
    if(c->win_state.skip_taskbar)
        return;

    move_client_node(wm, c, get_icon_client_head(), ANY_PLACE);

    Client *ld=c->subgroup_leader;
    for(Client *p=ld; ld && p->subgroup_leader==ld; p=list_prev_entry(p, Client, list))
    {
        p->win_state.hidden=1;
        update_net_wm_state(WIDGET_WIN(p), p->win_state);
        widget_hide(WIDGET(p->frame));
        if(p == CUR_FOC_CLI(wm))
        {
            focus_client(wm, NULL);
            frame_update_bg(p->frame);
        }
    }

    request_layout_update();
}

static Client *get_icon_client_head(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            return clients_prev(c);;
    return clients_last();
}

void deiconify_client(WM *wm, Client *c)
{
    if(!c)
        return;

    move_client_node(wm, c, NULL, c->place_type);
    Client *ld=c->subgroup_leader;
    for(Client *p=ld; ld && p->subgroup_leader==ld; p=list_prev_entry(p, Client, list))
    {
        if(is_iconic_client(p))
        {
            p->win_state.hidden=0;
            update_net_wm_state(WIDGET_WIN(p), p->win_state);
            widget_show(WIDGET(p->frame));
            focus_client(wm, p);
        }
    }
    request_layout_update();
}

void iconify_all_clients(WM *wm)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask) && !is_iconic_client(c))
            iconify_client(wm, c);
}

void deiconify_all_clients(WM *wm)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            deiconify_client(wm, c);
}

void change_net_wm_state_for_vmax(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, vmax))
        maximize_client(wm, c, VERT_MAX);
    else
        restore_client(wm, c);
}

void change_net_wm_state_for_hmax(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hmax))
        maximize_client(wm, c, HORZ_MAX);
    else
        restore_client(wm, c);
}

void change_net_wm_state_for_tmax(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, tmax))
        maximize_client(wm, c, TOP_MAX);
    else
        restore_client(wm, c);
}

void change_net_wm_state_for_bmax(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, bmax))
        maximize_client(wm, c, BOTTOM_MAX);
    else
        restore_client(wm, c);
}

void change_net_wm_state_for_lmax(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, lmax))
        maximize_client(wm, c, LEFT_MAX);
    else
        restore_client(wm, c);
}

void change_net_wm_state_for_rmax(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, rmax))
        maximize_client(wm, c, RIGHT_MAX);
    else
        restore_client(wm, c);
}

void change_net_wm_state_for_hidden(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hidden))
        iconify_client(wm, c);
    else
        deiconify_client(wm, is_iconic_client(c) ? c : NULL);
}

void change_net_wm_state_for_fullscreen(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, fullscreen))
        set_fullscreen(wm, c);
    else
        restore_client(wm, c);
}

static void set_fullscreen(WM *wm, Client *c)
{
    save_place_info_of_client(c);
    WIDGET_X(c)=WIDGET_Y(c)=0;
    WIDGET_W(c)=xinfo.screen_width, WIDGET_H(c)=xinfo.screen_height;
    move_client(wm, c, NULL, FULLSCREEN_LAYER);
    c->win_state.fullscreen=1;
    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}
