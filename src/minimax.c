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
#include "focus.h"
#include "clientop.h"

static void maximize(Max_way max_way);
static void maximize_client(Client *c, Max_way max_way);
static void set_max_rect(Client *c, Max_way max_way);
static Rect get_vert_max_rect(const Rect *workarea, const Client *c);
static Rect get_horz_max_rect(const Rect *workarea, const Client *c);
static Rect get_top_max_rect(const Rect *workarea);
static Rect get_bottom_max_rect(const Rect *workarea);
static Rect get_left_max_rect(const Rect *workarea);
static Rect get_right_max_rect(const Rect *workarea);
static void set_fullscreen(Client *c);

void minimize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    iconify_client(get_cur_focus_client()); 
}

void deiconify(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    deiconify_client(get_cur_focus_client()); 
}

void max_restore(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();

    if(is_win_state_max(c->win_state))
        restore_client(c);
    else
        maximize_client(c, FULL_MAX);
}

void vert_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(VERT_MAX);
}

void horz_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(HORZ_MAX);
}

void top_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(TOP_MAX);
}

void bottom_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(BOTTOM_MAX);
}

void left_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(LEFT_MAX);
}

void right_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(RIGHT_MAX);
}

void full_maximize(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize(FULL_MAX);
}

static void maximize(Max_way max_way)
{
    maximize_client(get_cur_focus_client(), max_way);
}

static void maximize_client(Client *c, Max_way max_way)
{
    if(!is_win_state_max(c->win_state))
        save_place_info_of_client(c);
    set_max_rect(c, max_way);
    if(is_tiled_client(c))
        move_client(c, NULL, ABOVE_LAYER);
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

static void set_max_rect(Client *c, Max_way max_way)
{
    int bw=WIDGET_BORDER_W(c->frame),
        w=WIDGET_W(c->frame)+2*bw, h=WIDGET_H(c->frame)+2*bw;
    Rect wr=get_net_workarea(), maxr=wr;
    switch(max_way)
    {
        case VERT_MAX:   if(w != wr.h) maxr=get_vert_max_rect(&wr, c); break;
        case HORZ_MAX:   if(h != wr.w) maxr=get_horz_max_rect(&wr, c); break;
        case TOP_MAX:    maxr=get_top_max_rect(&wr); break;
        case BOTTOM_MAX: maxr=get_bottom_max_rect(&wr); break;
        case LEFT_MAX:   maxr=get_left_max_rect(&wr); break;
        case RIGHT_MAX:  maxr=get_right_max_rect(&wr); break;
        case FULL_MAX:   break;
        default:         return;
    }
    set_client_rect_by_outline(c, maxr.x, maxr.y, maxr.w, maxr.h);
}

static Rect get_vert_max_rect(const Rect *workarea, const Client *c)
{
    Rect r=widget_get_outline(WIDGET(c->frame));
    return (Rect){r.x, workarea->y, r.w, workarea->h};
}

static Rect get_horz_max_rect(const Rect *workarea, const Client *c)
{
    Rect r=widget_get_outline(WIDGET(c->frame));
    return (Rect){workarea->x, r.y, workarea->w, r.h};
}

static Rect get_top_max_rect(const Rect *workarea)
{
    return (Rect){workarea->x, workarea->y, workarea->w, workarea->h/2};
}

static Rect get_bottom_max_rect(const Rect *workarea)
{
    return (Rect){workarea->x, workarea->y+workarea->h/2, workarea->w, workarea->h/2};
}

static Rect get_left_max_rect(const Rect *workarea)
{
    return (Rect){workarea->x, workarea->y, workarea->w/2, workarea->h};
}

static Rect get_right_max_rect(const Rect *workarea)
{
    return (Rect){workarea->x+workarea->w/2, workarea->y, workarea->w/2, workarea->h};
}

void change_net_wm_state_for_vmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, vmax))
        maximize_client(c, VERT_MAX);
    else
        restore_client(c);
}

void change_net_wm_state_for_hmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hmax))
        maximize_client(c, HORZ_MAX);
    else
        restore_client(c);
}

void change_net_wm_state_for_tmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, tmax))
        maximize_client(c, TOP_MAX);
    else
        restore_client(c);
}

void change_net_wm_state_for_bmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, bmax))
        maximize_client(c, BOTTOM_MAX);
    else
        restore_client(c);
}

void change_net_wm_state_for_lmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, lmax))
        maximize_client(c, LEFT_MAX);
    else
        restore_client(c);
}

void change_net_wm_state_for_rmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, rmax))
        maximize_client(c, RIGHT_MAX);
    else
        restore_client(c);
}

void change_net_wm_state_for_hidden(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hidden))
        iconify_client(c);
    else
        deiconify_client(is_iconic_client(c) ? c : NULL);
}

void change_net_wm_state_for_fullscreen(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, fullscreen))
        set_fullscreen(c);
    else
        restore_client(c);
}

static void set_fullscreen(Client *c)
{
    save_place_info_of_client(c);
    WIDGET_X(c)=WIDGET_Y(c)=0;
    WIDGET_W(c)=xinfo.screen_width, WIDGET_H(c)=xinfo.screen_height;
    move_client(c, NULL, FULLSCREEN_LAYER);
    c->win_state.fullscreen=1;
    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

void show_desktop(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool show=false;

    toggle_showing_desktop_mode(show=!show);
}

void toggle_showing_desktop_mode(bool show)
{
    if(show)
        iconify_all_clients();
    else
        deiconify_all_clients();
    set_net_showing_desktop(show);
}
