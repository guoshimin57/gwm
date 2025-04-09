/* *************************************************************************
 *     clientop.c：實現客戶窗口操作的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "grab.h"
#include "focus.h"
#include "prop.h"
#include "clientop.h"

static void set_frame_rect_by_client(Client *c);
static bool is_valid_move(Client *from, Client *to, Place type);
static bool is_valid_to_normal_layer_sec(Client *c);
static int cmp_client_store_order(Client *c1, Client *c2);
static void set_place_for_subgroup(Client *subgroup_leader, Place type);
static bool move_client_node(Client *from, Client *to, Place type);
static void add_subgroup(Client *head, Client *subgroup_leader);
static void del_subgroup(Client *subgroup_leader);
static void set_max_rect(Client *c, Max_way max_way);
static Rect get_vert_max_rect(const Rect *workarea, const Client *c);
static Rect get_horz_max_rect(const Rect *workarea, const Client *c);
static Rect get_top_max_rect(const Rect *workarea);
static Rect get_bottom_max_rect(const Rect *workarea);
static Rect get_left_max_rect(const Rect *workarea);
static Rect get_right_max_rect(const Rect *workarea);

void manage_exsit_clients(void)
{
    unsigned int n;
    Window *child=query_win_list(&n);
    if(!child)
        exit_with_msg(_("錯誤：查詢窗口清單失敗！"));

    for(size_t i=0; i<n; i++)
        if(is_wm_win(child[i], true))
            add_client(child[i]);
    XFree(child);
}

void add_client(Window win)
{
    Client *c=client_new(win);
    XSelectInput(xinfo.display, win, EnterWindowMask|PropertyChangeMask);
    set_cursor(win, NO_OP);
    request_layout_update();
    widget_show(WIDGET(c->frame));
    set_net_wm_allowed_actions(WIDGET_WIN(c));
    focus_client(c);
}

void remove_client(Client *c, bool is_for_quit)
{
    focus_client(NULL);
    client_del(c);
    if(!is_for_quit)
        request_layout_update();
}

void move_resize_client(Client *c, const Delta_rect *d)
{
    if(d)
        WIDGET_X(c)+=d->dx, WIDGET_Y(c)+=d->dy, WIDGET_W(c)+=d->dw, WIDGET_H(c)+=d->dh;
    set_frame_rect_by_client(c);
    frame_move_resize(c->frame, WIDGET_X(c->frame), WIDGET_Y(c->frame), WIDGET_W(c->frame), WIDGET_H(c->frame));

    int bh=frame_get_titlebar_height(c->frame);
    XMoveResizeWindow(xinfo.display, WIDGET_WIN(c), 0, bh, WIDGET_W(c), WIDGET_H(c));
}

static void set_frame_rect_by_client(Client *c)
{
    int bw=WIDGET_BORDER_W(c->frame),
        bh=frame_get_titlebar_height(c->frame),
        x=WIDGET_X(c)-bw,
        y=WIDGET_Y(c)-bh-bw,
        w=WIDGET_W(c),
        h=WIDGET_H(c)+bh;
    widget_set_rect(WIDGET(c->frame), x, y, w, h);
}

void move_client(Client *from, Client *to, Place type)
{
    if(move_client_node(from, to, type))
    {
        set_place_for_subgroup(from->subgroup_leader,
            to ? to->place : type);
        request_layout_update();
    }
}

static bool move_client_node(Client *from, Client *to, Place type)
{
    if(!is_valid_move(from, to, type))
        return false;

    Client *head=NULL;
    del_subgroup(from->subgroup_leader);
    if(to)
        head = cmp_client_store_order(from, to) < 0 ? to : list_prev_entry(to, Client, list);
    else
    {
        head=get_head_client(NULL, type);
        if(from->place==MAIN_AREA && type==SECOND_AREA)
            head=list_next_entry(head, Client, list);
    }
    add_subgroup(head, from->subgroup_leader);
    return true;
}

static bool is_valid_move(Client *from, Client *to, Place type)
{
    Place t = to ? to->place : type;

    return from
        && (!to || from->subgroup_leader!=to->subgroup_leader)
        && (t!=SECOND_AREA || is_valid_to_normal_layer_sec(from))
        && (get_gwm_layout()==TILE || !is_normal_layer(t));
}

static bool is_valid_to_normal_layer_sec(Client *c)
{
    return c->place!=MAIN_AREA
        || get_clients_n(SECOND_AREA, false, false, false);
}

static int cmp_client_store_order(Client *c1, Client *c2)
{
    if(c1 == c2)
        return 0;
    clients_for_each_from(c1)
        if(c1 == c2)
            return -1;
    return 1;
}

static void set_place_for_subgroup(Client *subgroup_leader, Place type)
{
    subgroup_for_each(c, subgroup_leader)
        c->place=type;
}

static void add_subgroup(Client *head, Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *first=(top ? top : subgroup_leader),
           *last=subgroup_leader;

    list_bulk_add(&head->list, &first->list, &last->list);
}

static void del_subgroup(Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *first=(top ? top : subgroup_leader),
           *last=subgroup_leader;

    list_bulk_del(&first->list, &last->list);
}

void swap_clients(Client *a, Client *b)
{
    if(a->subgroup_leader == b->subgroup_leader)
        return;

    Client *tmp, *top, *a_begin, *b_begin, *a_prev, *a_leader, *b_leader;

    if(cmp_client_store_order(a, b) > 0)
        tmp=a, a=b, b=tmp;

    a_leader=a->subgroup_leader, b_leader=b->subgroup_leader;

    top=get_top_transient_client(a_leader, false);
    a_begin=(top ? top : a_leader);
    a_prev=clients_prev(a_begin);

    top=get_top_transient_client(b_leader, false);
    b_begin=(top ? top : b_leader);

    del_subgroup(a_leader);
    add_subgroup(b_leader, a_leader);
    if(clients_next(a_leader) != b_begin) //不相邻
        del_subgroup(b_leader), add_subgroup(a_prev, b_leader);

    request_layout_update();
}

void restore_client(Client *c)
{
    restore_place_info_of_client(c);
    move_client(c, NULL, c->place);
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

void iconify_client(Client *c)
{
    if(c->win_state.skip_taskbar)
        return;

    subgroup_for_each(p, c->subgroup_leader)
    {
        p->win_state.hidden=1;
        update_net_wm_state(WIDGET_WIN(p), p->win_state);
        widget_hide(WIDGET(p->frame));
        if(WIDGET_WIN(p) == get_net_active_window())
        {
            focus_client(NULL);
            frame_update_bg(p->frame);
        }
    }
    request_layout_update();
}

void deiconify_client(Client *c)
{
    if(!c)
        return;

    subgroup_for_each(p, c->subgroup_leader)
    {
        if(is_iconic_client(p))
        {
            p->win_state.hidden=0;
            update_net_wm_state(WIDGET_WIN(p), p->win_state);
            widget_show(WIDGET(p->frame));
            focus_client(p);
        }
    }
    request_layout_update();
}

void iconify_all_clients(void)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask) && !is_iconic_client(c))
            iconify_client(c);
}

void deiconify_all_clients(void)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            deiconify_client(c);
}

void maximize_client(Client *c, Max_way max_way)
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

void toggle_showing_desktop_mode(bool show)
{
    if(show)
        iconify_all_clients();
    else
        deiconify_all_clients();
    set_net_showing_desktop(show);
}

void toggle_shade_mode(Client *c, bool shade)
{
    if(!c->decorative)
        return;

    int x=WIDGET_X(c->frame), y=WIDGET_Y(c->frame),
        w=WIDGET_W(c->frame), h=WIDGET_H(c->frame), ch=WIDGET_H(c);
    if(shade)
        frame_move_resize(c->frame, x, y, w, h-ch);
    else
        frame_move_resize(c->frame, x, y, w, h+ch);
    c->win_state.shaded=shade;
}
