/* *************************************************************************
 *     client.c：實現客戶窗口基本功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "gwm.h"
#include "font.h"
#include "misc.h"
#include "list.h"
#include "image.h"
#include "icccm.h"
#include "prop.h"
#include "client.h"

static Client *client_new(Window win);
static void client_ctor(Client *c, Window win);
static bool has_decoration(const Client *c);
static void set_default_desktop_mask(Client *c);
static void apply_rules(Client *c);
static bool have_rule(const Rule *r, Client *c);
static void set_default_place(Client *c);
static void client_dtor(Client *c);
static void set_default_win_rect(Client *c);
static void set_client_rect_by_frame(Client *c);
static bool is_valid_move(Client *from, Client *to, Place type);
static bool is_valid_to_normal_layer_sec(Client *c);
static int cmp_client_store_order(Client *c1, Client *c2);
static void set_place_for_subgroup(Client *subgroup_leader, Place type);
static Client *get_icon_client_head(void);
static bool move_client_node(Client *from, Client *to, Place type);
static void set_frame_rect_by_client(Client *c);
static void add_subgroup(Client *head, Client *subgroup_leader);
static void del_subgroup(Client *subgroup_leader);

static Client *clients=NULL;

// 指向聚焦客戶窗口的函數，目的是爲與與focus模塊解耦
static void (*focus)(Client *)=NULL;

void reg_focus_func(void (*func)(Client *))
{
    focus=func;
}

void init_client_list(void)
{
    clients=Malloc(sizeof(Client));
    list_init(&clients->list);
}

void manage_exsit_clients(void)
{
    unsigned int n;
    Window *child=query_win_list(&n);
    if(!child)
        exit_with_msg(_("錯誤：查詢窗口清單失敗！"));

    for(size_t i=0; i<n; i++)
        if(is_wm_win(child[i], true))
            client_add(child[i]);
    XFree(child);
}

Client *get_clients(void)
{
    return clients;
}

void client_add(Window win)
{
    Client *c=client_new(win);
    list_add(&c->list, &get_head_client(c->place)->list);
    grab_buttons(WIDGET_WIN(c));
    XSelectInput(xinfo.display, win, EnterWindowMask|PropertyChangeMask);
    set_cursor(win, NO_OP);
    request_layout_update();
    widget_show(WIDGET(c->frame));
    focus(c);
    set_net_wm_allowed_actions(WIDGET_WIN(c));
}

static Client *client_new(Window win)
{
    Client *c=Malloc(sizeof(Client));
    client_ctor(c, win);
    return c;
}

static void client_ctor(Client *c, Window win)
{
    widget_ctor(WIDGET(c), NULL, WIDGET_TYPE_CLIENT, CLIENT_WIN, 0, 0, 1, 1);
    WIDGET_WIN(c)=win;
    c->title_text=get_title_text(win, "");
    c->wm_hint=XGetWMHints(xinfo.display, win);
    c->win_type=get_net_wm_win_type(win);
    c->win_state=get_net_wm_state(win);
    c->decorative=has_decoration(c);
    c->owner=win_to_client(get_transient_for(WIDGET_WIN(c)));
    c->subgroup_leader=get_subgroup_leader(c);
    c->class_name="?";
    c->image=get_win_icon_image(win);
    c->frame=frame_new(WIDGET(c), 0, 0, 1, 1,
        c->decorative ? get_font_height_by_pad() : 0,
        c->decorative ? cfg->border_width : 0,
        c->title_text, c->image);
    XGetClassHint(xinfo.display, WIDGET_WIN(c), &c->class_hint);
    set_default_place(c);
    set_default_win_rect(c);
    set_default_desktop_mask(c);
    apply_rules(c);
    save_place_info_of_client(c);
    widget_set_state(WIDGET(c->frame), WIDGET_STATE(c));
}

static bool has_decoration(const Client *c)
{
    return has_motif_decoration(WIDGET_WIN(c))
        && (c->win_type.none || c->win_type.normal || c->win_type.dialog)
        && !c->win_state.skip_pager && !c->win_state.skip_taskbar;
}

Client *get_subgroup_leader(Client *c)
{
    for(; c && c->owner; c=c->owner)
        ;
    return c;
}

static void set_default_place(Client *c)
{
    if(c->owner)                     c->place = c->owner->place;
    else if(c->win_type.desktop)     c->place = DESKTOP_LAYER;
    else if(c->win_state.below)      c->place = BELOW_LAYER;
    else if(c->win_state.above
            || (get_gwm_layout()==TILE && is_win_state_max(c->win_state)))
                                     c->place = ABOVE_LAYER;
    else if(c->win_type.dock)        c->place = DOCK_LAYER;
    else if(c->win_state.fullscreen) c->place = FULLSCREEN_LAYER;
    else                             c->place = MAIN_AREA;  
}

static void set_default_desktop_mask(Client *c)
{
    Window win=WIDGET_WIN(c);
    if(c->win_state.sticky)
        c->desktop_mask=~0U;
    else
    {
        unsigned int old_desktop=get_net_wm_desktop(win),
                     cur_desktop=get_net_current_desktop(),
                     old_mask=get_desktop_mask(old_desktop),
                     cur_mask=get_desktop_mask(cur_desktop);
        c->desktop_mask = old_mask|cur_mask;
    }
}

static void apply_rules(Client *c)
{
    if(!c->class_hint.res_class && !c->class_hint.res_name && !c->title_text)
        return;

    c->class_name=c->class_hint.res_class;
    for(const Rule *r=cfg->rule; r->app_class; r++)
    {
        if(have_rule(r, c))
        {
            if(r->place != ANY_PLACE)
                c->place=r->place;
            if(r->desktop_mask)
                c->desktop_mask=r->desktop_mask;
            if(r->class_alias)
                c->class_name=r->class_alias;
        }
    }
}

static bool have_rule(const Rule *r, Client *c)
{
    const char *pc=r->app_class, *pn=r->app_name, *pt=r->title,
        *class=c->class_hint.res_class, *name=c->class_hint.res_name,
        *title=c->title_text;
    
    return((!pc || !class || !strcmp(class, pc) || !strcmp(pc, "*"))
        && (!pn || !name  || !strcmp(name, pn)  || !strcmp(pn, "*"))
        && (!pt || !title || !strcmp(title, pt) || !strcmp(pt, "*")));
}

int get_clients_n(Place type, bool count_icon, bool count_trans, bool count_all_desktop)
{
    int n=0;
    clients_for_each(c)
        if( (type==ANY_PLACE || c->place==type)
            && (count_icon || !is_iconic_client(c))
            && (count_trans || !c->owner)
            && (count_all_desktop || is_on_cur_desktop(c->desktop_mask)))
            n++;
    return n;
}

bool is_iconic_client(const Client *c)
{
    return WIDGET_WIN(c)!=xinfo.root_win && c->win_state.hidden;
}

Client *win_to_client(Window win)
{
    // 當隱藏標題欄時，標題區和按鈕的窗口ID爲0。故win爲0時，不應視爲找到
    clients_for_each(c)
        if(win==WIDGET_WIN(c) || frame_has_win(c->frame, win))
            return c;
    return NULL;
}

void client_del(Client *c, bool is_for_quit)
{
    list_del(&c->list);
    focus(NULL);
    client_dtor(c);
    widget_del(WIDGET(c));
    if(!is_for_quit)
        request_layout_update();
}

static void client_dtor(Client *c)
{
    vXFree(c->class_hint.res_class, c->class_hint.res_name, c->wm_hint);
    Free(c->title_text);
    frame_del(c->frame), c->frame=NULL;
}

/* 當WIDGET_WIN(c)所在的亞組存在模態窗口時，跳過所有亞組窗口 */
Client *get_next_client(Client *c)
{
    if(!c)
        return NULL;

    Client *ld=c->subgroup_leader;
    list_for_each_entry_continue(Client, c, &clients->list, list)
        if(is_on_cur_desktop(c->desktop_mask) && (!ld || c->subgroup_leader!=ld))
            return c;

    return NULL;
}

/* 當WIDGET_WIN(c)所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_prev_client(Client *c)
{
    if(!c)
        return NULL;

    Client *m=NULL;
    list_for_each_entry_continue_reverse(Client, c, &clients->list, list)
        if(is_on_cur_desktop(c->desktop_mask))
            return (m=get_top_transient_client(c->subgroup_leader, true)) ? m : c;

    return NULL;
}

bool is_normal_layer(Place t)
{
    return t==MAIN_AREA || t==SECOND_AREA || t==FIXED_AREA;
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

bool is_spec_place_last_client(Client *c, Place place)
{
    clients_for_each_reverse(p)
    {
        if(p == c)
            break;
        if(is_on_cur_desktop(WIDGET_WIN(p)) && p->place==place)
            return false;
    }
    return true;
}

Client *get_head_client(Place place)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && c->place==place)
            return list_prev_entry(c, Client, list);

    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && c->place > place)
            return list_prev_entry(c, Client, list);

    return clients;
}

int get_subgroup_n(Client *c)
{
    int n=0;

    subgroup_for_each(p, c->subgroup_leader)
        n++;

    return n;
}

Client *get_top_transient_client(Client *subgroup_leader, bool only_modal)
{
    Client *result=NULL;

    subgroup_for_each(c, subgroup_leader)
        if((only_modal && c->win_state.modal) || (!only_modal && c->owner))
            result=c;

    return result;
}

void client_set_state_unfocused(Client *c, int value)
{
    WIDGET_STATE(c).unfocused=value;
    if(c->frame)
        frame_set_state_unfocused(c->frame, value);
}

static void set_default_win_rect(Client *c)
{
    XWindowAttributes a;
    if(XGetWindowAttributes(xinfo.display, WIDGET_WIN(c), &a))
        WIDGET_X(c)=a.x, WIDGET_Y(c)=a.y,
        WIDGET_W(c)=a.width, WIDGET_H(c)=a.height;
    else
        WIDGET_X(c)=xinfo.screen_width/4, WIDGET_Y(c)=xinfo.screen_height/4,
        WIDGET_W(c)=xinfo.screen_width/2, WIDGET_H(c)=xinfo.screen_height/2;
}

void save_place_info_of_client(Client *c)
{
    c->ox=WIDGET_X(c), c->oy=WIDGET_Y(c), c->ow=WIDGET_W(c), c->oh=WIDGET_H(c);
    c->old_place=c->place;
}

void restore_place_info_of_client(Client *c)
{
    WIDGET_X(c)=c->ox, WIDGET_Y(c)=c->oy, WIDGET_W(c)=c->ow, WIDGET_H(c)=c->oh;
    c->place=c->old_place;
}

bool is_tile_client(Client *c)
{
    return is_on_cur_desktop(c->desktop_mask) && !c->owner && !is_iconic_client(c)
        && is_normal_layer(c->place);
}

bool is_tiled_client(Client *c)
{
    return get_gwm_layout()==TILE && is_tile_client(c);
}

void set_state_attent(Client *c, bool attent)
{
    if(c->win_state.attent == attent) // 避免重復設置
        return;

    c->win_state.attent=attent;
    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

bool is_wm_win(Window win, bool before_wm)
{
    XWindowAttributes a;
    bool status=XGetWindowAttributes(xinfo.display, win, &a);

    if( !status || a.override_redirect
        || !is_on_screen( a.x, a.y, a.width, a.height))
        return false;

    if(!before_wm)
        return !win_to_client(win);

    return is_iconic_state(win) || a.map_state==IsViewable;
}

void update_clients_bg(void)
{
    clients_for_each(c)
        update_client_bg(c);
}

void update_client_bg(Client *c)
{
    if(!c || c==clients)
        return;

    if(is_iconic_client(c))
        c->win_state.focused=1, update_net_wm_state(WIDGET_WIN(c), c->win_state);
    else if(c->frame)
        frame_update_bg(c->frame);
}

void set_client_rect_by_outline(Client *c, int x, int y, int w, int h)
{
    int bw=WIDGET_BORDER_W(c->frame);
    widget_set_rect(WIDGET(c->frame), x, y, w-2*bw, h-2*bw);
    set_client_rect_by_frame(c);
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

static void set_client_rect_by_frame(Client *c)
{
    int bw=WIDGET_BORDER_W(c->frame),
        bh=frame_get_titlebar_height(c->frame),
        x=WIDGET_X(c->frame)+bw,
        y=WIDGET_Y(c->frame)+bh+bw,
        w=WIDGET_W(c->frame),
        h=WIDGET_H(c->frame)-bh;
    widget_set_rect(WIDGET(c), x, y, w, h);
}

bool is_exist_client(Client *c)
{
    clients_for_each(p)
        if(p == c)
            return true;
    return false;
}

Client *get_new_client(void)
{
    clients_for_each(c)
        if(is_new_client(c))
            return c;
    return NULL;
}

bool is_new_client(Client *c)
{
    return WIDGET_W(c->frame)==1 && WIDGET_H(c->frame)==1;
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
        head=get_head_client(type);
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
    for(Client *ld=subgroup_leader, *c=ld; ld && c->subgroup_leader==ld; c=list_prev_entry(c, Client, list))
        c->place=type;
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
    a_prev=list_prev_entry(a_begin, Client, list);

    top=get_top_transient_client(b_leader, false);
    b_begin=(top ? top : b_leader);

    del_subgroup(a_leader);
    add_subgroup(b_leader, a_leader);
    if(list_next_entry(a_leader, Client, list) != b_begin) //不相邻
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

    move_client_node(c, get_icon_client_head(), ANY_PLACE);

    Client *ld=c->subgroup_leader;
    for(Client *p=ld; ld && p->subgroup_leader==ld; p=list_prev_entry(p, Client, list))
    {
        p->win_state.hidden=1;
        update_net_wm_state(WIDGET_WIN(p), p->win_state);
        widget_hide(WIDGET(p->frame));
        if(WIDGET_WIN(p) == get_net_active_window())
        {
            focus(NULL);
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

void deiconify_client(Client *c)
{
    if(!c)
        return;

    move_client_node(c, NULL, c->place);
    Client *ld=c->subgroup_leader;
    for(Client *p=ld; ld && p->subgroup_leader==ld; p=list_prev_entry(p, Client, list))
    {
        if(is_iconic_client(p))
        {
            p->win_state.hidden=0;
            update_net_wm_state(WIDGET_WIN(p), p->win_state);
            widget_show(WIDGET(p->frame));
            focus(p);
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
