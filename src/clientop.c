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

#include "misc.h"
#include "grab.h"
#include "focus.h"
#include "prop.h"
#include "clientop.h"

static void set_frame_rect_by_client(Client *c);
static bool is_valid_move(Client *from, Client *to, Layer layer, Area area);
static bool is_valid_to_sec_area(Client *c);
static void move_client_node(Client *from, Client *to, Layer layer, Area area);
static int cmp_store_order(Client *c1, Client *c2);
static void set_place_for_subgroup(Client *subgroup_leader, Layer layer, Area area);
static void set_net_wm_state_for_subgroup(Client *subgroup_leader);
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

void remove_client(Client *c)
{
    focus_client(NULL);
    client_del(c);
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

// 對於後3個參數，當to非空時，只使用to；否則只使用layer和area
void move_client(Client *from, Client *to, Layer layer, Area area)
{
    if(to)
        layer=to->layer, area=to->area;

    if(!is_valid_move(from, to, layer, area))
        return;

    move_client_node(from, to, layer, area);
    set_place_for_subgroup(from->subgroup_leader, layer, area);
    set_net_wm_state_for_subgroup(from->subgroup_leader);
    request_layout_update();
}

static bool is_valid_move(Client *from, Client *to, Layer layer, Area area)
{
    return (from && from!=to
        && (!to || from->subgroup_leader!=to->subgroup_leader)
        && (layer!=NORMAL_LAYER || area!=ANY_AREA)
        && (layer==NORMAL_LAYER || area==ANY_AREA)
        && (area!=SECOND_AREA || is_valid_to_sec_area(from)));
}

static bool is_valid_to_sec_area(Client *c)
{
    return c->area!=MAIN_AREA
        || get_clients_n(NORMAL_LAYER, SECOND_AREA, false, false, false);
}

static void move_client_node(Client *from, Client *to, Layer layer, Area area)
{
    Client *head=NULL;
    del_subgroup(from->subgroup_leader);
    if(to)
        head = cmp_store_order(from, to) < 0 ? to : LIST_PREV(Client, to);
    else
    {
        head=get_head_client(NULL, layer, area);
        if(from->area==MAIN_AREA && area==SECOND_AREA)
            head=LIST_NEXT(Client, head);
    }
    add_subgroup(head, from->subgroup_leader);
}

static int cmp_store_order(Client *c1, Client *c2)
{
    if(c1 == c2)
        return 0;
    clients_for_each_from(c1)
        if(c1 == c2)
            return -1;
    return 1;
}

static void set_place_for_subgroup(Client *subgroup_leader, Layer layer, Area area)
{
    subgroup_for_each(c, subgroup_leader)
        c->layer=layer, c->area=area;
}

static void set_net_wm_state_for_subgroup(Client *subgroup_leader)
{
    subgroup_for_each(c, subgroup_leader)
        update_net_wm_state_by_layer(c);
}

void update_net_wm_state_by_layer(Client *c)
{
    Net_wm_state *s=&c->win_state;
    switch(c->layer)
    {
        case FULLSCREEN_LAYER:
            s->vmax=s->hmax=s->tmax=s->bmax=s->lmax=s->rmax=0;
            s->above=s->below=0;
            s->fullscreen=1;
            break;
        case ABOVE_LAYER:
            s->fullscreen=s->below=0;
            s->above=1;
            break;
        case NORMAL_LAYER:
            s->fullscreen=s->above=s->below=0;
            if(get_gwm_layout() == TILE)
                s->vmax=s->hmax=s->tmax=s->bmax=s->lmax=s->rmax=0;
            break;
        case BELOW_LAYER:
            s->fullscreen=s->above=0;
            s->below=1;
            break;
        default: // DOCK_LAYER、DESKTOP_LAYER
            s->vmax=s->hmax=s->tmax=s->bmax=s->lmax=s->rmax=0;
            s->fullscreen=s->above=s->below=0;
            break;
    }
    update_net_wm_state(WIDGET_WIN(c), *s);
}

static void add_subgroup(Client *head, Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *first=(top ? top : subgroup_leader),
           *last=subgroup_leader;

    LIST_BULK_ADD(head, first, last);
}

static void del_subgroup(Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *first=(top ? top : subgroup_leader),
           *last=subgroup_leader;

    LIST_BULK_DEL(first, last);
}

void swap_clients(Client *a, Client *b)
{
    if(a->subgroup_leader == b->subgroup_leader)
        return;

    Client *tmp, *top, *a_begin, *b_begin, *a_prev, *a_leader, *b_leader;

    if(cmp_store_order(a, b) > 0)
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
    move_client(c, NULL, c->layer, c->area);
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
        if(is_on_cur_desktop(c) && !is_iconic_client(c))
            iconify_client(c);
}

void deiconify_all_clients(void)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c) && is_iconic_client(c))
            deiconify_client(c);
}

void maximize_client(Client *c, Max_way max_way)
{
    if(!is_win_state_max(c->win_state))
        save_place_info_of_client(c);
    if(get_gwm_layout()==TILE && c->layer==NORMAL_LAYER)
        move_client(c, NULL, ABOVE_LAYER, ANY_AREA);
    set_max_rect(c, max_way);
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

void set_fullscreen(Client *c)
{
    save_place_info_of_client(c);
    WIDGET_X(c)=WIDGET_Y(c)=0;
    WIDGET_W(c)=xinfo.screen_width, WIDGET_H(c)=xinfo.screen_height;
    move_client(c, NULL, FULLSCREEN_LAYER, ANY_AREA);
}

