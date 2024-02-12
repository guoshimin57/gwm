/* *************************************************************************
 *     minimax.c：實現窗口最小化、最大化、還原、全屏功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static Client *get_icon_client_head(WM *wm);
static void set_fullscreen(WM *wm, Client *c);

void max_restore_client(WM *wm, Client *c)
{
    if(is_win_state_max(c))
        restore_client(wm, c);
    else
        maximize_client(wm, c, FULL_MAX);
}

void maximize_client(WM *wm, Client *c, Max_way max_way)
{
    if(!is_win_state_max(c))
        save_place_info_of_client(c);
    set_max_rect(wm, c, max_way);
    move_client(wm, c, NULL, get_dest_place_type_for_move(wm, c));
    move_resize_client(wm, c, NULL);
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
    update_net_wm_state(c->win, c->win_state);
}

void set_max_rect(WM *wm, Client *c, Max_way max_way)
{
    int left_x, top_y, max_w, max_h, mid_x, mid_y, half_w, half_h;
    get_max_rect(wm, c, &left_x, &top_y, &max_w, &max_h, &mid_x, &mid_y, &half_w, &half_h);

    bool vmax=(c->h == max_h), hmax=(c->w == max_w), fmax=false;
    switch(max_way)
    {
        case VERT_MAX:  if(hmax) fmax=true; else c->y=top_y, c->h=max_h;  break;
        case HORZ_MAX:  if(vmax) fmax=true; else c->x=left_x, c->w=max_w; break;
        case TOP_MAX:   c->x=left_x, c->y=top_y, c->w=max_w, c->h=half_h; break;
        case BOTTOM_MAX:c->x=left_x, c->y=mid_y, c->w=max_w, c->h=half_h; break;
        case LEFT_MAX:  c->x=left_x, c->y=top_y, c->w=half_w, c->h=max_h; break;
        case RIGHT_MAX: c->x=mid_x, c->y=top_y, c->w=half_w, c->h=max_h;  break;
        case FULL_MAX:  fmax=true; break;
        default:        return;
    }
    if(fmax)
        c->x=left_x, c->y=top_y, c->w=max_w, c->h=max_h;
}

void get_max_rect(WM *wm, Client *c, int *left_x, int *top_y, int *max_w, int *max_h, int *mid_x, int *mid_y, int *half_w, int *half_h)
{
    int bw=c->border_w, th=c->titlebar_h, wx=wm->workarea.x, wy=wm->workarea.y,
        ww=wm->workarea.w, wh=wm->workarea.h;
    *left_x=wx+bw, *top_y=wy+bw+th, *max_w=ww-2*bw, *max_h=wh-th-2*bw,
    *mid_x=*left_x+ww/2, *mid_y=*top_y+wh/2, *half_w=ww/2-2*bw, *half_h=wh/2-th-2*bw;
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
    update_net_wm_state(c->win, c->win_state);
}

bool is_win_state_max(Client *c)
{
    return c->win_state.vmax || c->win_state.hmax || c->win_state.tmax
        || c->win_state.bmax || c->win_state.lmax || c->win_state.rmax;
}

void fix_win_rect_by_state(WM *wm, Client *c)
{
    if(is_win_state_max(c))
    {
        Max_way way;
        if(c->win_state.vmax)   way=VERT_MAX;
        if(c->win_state.hmax)   way=HORZ_MAX;
        if(c->win_state.tmax)   way=TOP_MAX;
        if(c->win_state.bmax)   way=BOTTOM_MAX;
        if(c->win_state.lmax)   way=LEFT_MAX;
        if(c->win_state.rmax)   way=RIGHT_MAX;
        if(c->win_state.vmax && c->win_state.hmax)
            way=FULL_MAX;
        set_max_rect(wm, c, way);
    }
    else if(c->win_state.fullscreen)
        c->x=c->y=0, c->w=xinfo.screen_width, c->h=xinfo.screen_height;
}

void iconify(WM *wm, Client *c)
{
    if(c->win_state.skip_taskbar)
        return;

    move_client_node(wm, c, get_icon_client_head(wm), ANY_PLACE);
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
    {
        create_icon(p);
        p->icon->title_text=get_icon_title_text(p->win, p->title_text);
        update_win_bg(p->icon->win, get_widget_color(TASKBAR_COLOR), None);
        update_icon_area(wm);
        XMapWindow(xinfo.display, p->icon->win);
        XUnmapWindow(xinfo.display, p->frame);
        if(p == DESKTOP(wm)->cur_focus_client)
        {
            focus_client(wm, wm->cur_desktop, NULL);
            update_frame_bg(wm, wm->cur_desktop, p);
        }
        p->win_state.hidden=1;
        update_net_wm_state(p->win, p->win_state);
    }
    request_layout_update();
}

static Client *get_icon_client_head(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->icon)
            return c->prev;
    return wm->clients->prev;
}

void create_icon(Client *c)
{
    Icon *i=c->icon=malloc_s(sizeof(Icon));
    i->x=i->y=0, i->w=i->h=taskbar->h;
    i->title_text=NULL; // 有的窗口映射時未設置圖標標題，故應延後至縮微窗口時再設置title_text
    i->win=create_widget_win(CLIENT_ICON, taskbar->icon_area, 0, 0, i->w, i->h,
        0, 0, get_widget_color(TASKBAR_COLOR));
    XSelectInput(xinfo.display, c->icon->win, ICON_WIN_EVENT_MASK);
}

void deiconify(WM *wm, Client *c)
{
    if(!c)
        return;

    move_client_node(wm, c, NULL, c->place_type);
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
    {
        if(p->icon)
        {
            del_icon(wm, p);
            XMapWindow(xinfo.display, p->frame);
            update_icon_area(wm);
            focus_client(wm, wm->cur_desktop, p);
            p->win_state.hidden=0;
            update_net_wm_state(p->win, p->win_state);
        }
    }
    request_layout_update();
}

void del_icon(WM *wm, Client *c)
{
    if(c->icon)
    {
        XDestroyWindow(xinfo.display, c->icon->win);
        vfree(c->icon->title_text, c->icon, NULL);
        c->icon=NULL;
        update_icon_area(wm);
    }
}

void iconify_all_clients(WM *wm)
{
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c) && !c->icon)
            iconify(wm, c);
}

void deiconify_all_clients(WM *wm)
{
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c) && c->icon)
            deiconify(wm, c);
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
        iconify(wm, c);
    else
        deiconify(wm, c->icon ? c : NULL);
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
    c->x=c->y=0, c->w=xinfo.screen_width, c->h=xinfo.screen_height;
    move_client(wm, c, NULL, FULLSCREEN_LAYER);
    update_net_wm_state(c->win, c->win_state);
}
