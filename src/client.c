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
#include "desktop.h"

static Client *client_new(Window win);
static void client_ctor(Client *c, Window win);
static bool has_decoration(const Client *c);
static void set_default_desktop_mask(Client *c);
static void apply_rules(Client *c);
static bool have_rule(const Rule *r, Client *c);
static void set_default_place_type(Client *c);
static void client_dtor(Client *c);
static void set_default_win_rect(Client *c);

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
    list_add(&c->list, &get_head_client(c->place_type)->list);
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
    set_default_place_type(c);
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

static void set_default_place_type(Client *c)
{
    if(c->owner)                     c->place_type = c->owner->place_type;
    else if(c->win_type.desktop)     c->place_type = DESKTOP_LAYER;
    else if(c->win_state.below)      c->place_type = BELOW_LAYER;
    else if(c->win_type.dock)        c->place_type = DOCK_LAYER;
    else if(c->win_state.above)      c->place_type = ABOVE_LAYER;
    else if(c->win_state.fullscreen) c->place_type = FULLSCREEN_LAYER;
    else if(is_win_state_max(c->win_state)) c->place_type = FLOAT_LAYER;
    else                             c->place_type = TILE_LAYER_MAIN;  
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
            if(r->place_type != ANY_PLACE)
                c->place_type=r->place_type;
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

int get_clients_n(Place_type type, bool count_icon, bool count_trans, bool count_all_desktop)
{
    int n=0;
    clients_for_each(c)
        if( (type==ANY_PLACE || c->place_type==type)
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

bool is_normal_layer(Place_type t)
{
    return t==TILE_LAYER_MAIN || t==TILE_LAYER_SECOND || t==TILE_LAYER_FIXED;
}

void add_subgroup(Client *head, Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *first=(top ? top : subgroup_leader),
           *last=subgroup_leader;

    list_bulk_add(&head->list, &first->list, &last->list);
}

void del_subgroup(Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *first=(top ? top : subgroup_leader),
           *last=subgroup_leader;

    list_bulk_del(&first->list, &last->list);
}

bool is_last_typed_client(Client *c, Place_type type)
{
    clients_for_each_reverse(p)
    {
        if(p == c)
            break;
        if(is_on_cur_desktop(WIDGET_WIN(p)) && p->place_type==type)
            return false;
    }
    return true;
}

Client *get_head_client(Place_type type)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && c->place_type==type)
            return list_prev_entry(c, Client, list);

    clients_for_each(c)
        if(c->place_type == type)
            return list_prev_entry(c, Client, list);

    clients_for_each(c)
        if(c->place_type > type)
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
    c->old_place_type=c->place_type;
}

void save_place_info_of_clients(void)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask))
            save_place_info_of_client(c);
}

void restore_place_info_of_client(Client *c)
{
    WIDGET_X(c)=c->ox, WIDGET_Y(c)=c->oy, WIDGET_W(c)=c->ow, WIDGET_H(c)=c->oh;
    c->place_type=c->old_place_type;
}

void restore_place_info_of_clients(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            restore_place_info_of_client(c);
}

bool is_tile_client(Client *c)
{
    return is_on_cur_desktop(c->desktop_mask) && !c->owner && !is_iconic_client(c)
        && is_normal_layer(c->place_type);
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

void move_resize_client(Client *c, const Delta_rect *d)
{
    if(d)
        WIDGET_X(c)+=d->dx, WIDGET_Y(c)+=d->dy, WIDGET_W(c)+=d->dw, WIDGET_H(c)+=d->dh;
    set_frame_rect_by_client(c);
    frame_move_resize(c->frame, WIDGET_X(c->frame), WIDGET_Y(c->frame), WIDGET_W(c->frame), WIDGET_H(c->frame));

    int bh=frame_get_titlebar_height(c->frame);
    XMoveResizeWindow(xinfo.display, WIDGET_WIN(c), 0, bh, WIDGET_W(c), WIDGET_H(c));
}

void set_client_rect_by_outline(Client *c, int x, int y, int w, int h)
{
    int bw=WIDGET_BORDER_W(c->frame);
    widget_set_rect(WIDGET(c->frame), x, y, w-2*bw, h-2*bw);
    set_client_rect_by_frame(c);
}

void set_frame_rect_by_client(Client *c)
{
    int bw=WIDGET_BORDER_W(c->frame),
        bh=frame_get_titlebar_height(c->frame),
        x=WIDGET_X(c)-bw,
        y=WIDGET_Y(c)-bh-bw,
        w=WIDGET_W(c),
        h=WIDGET_H(c)+bh;
    widget_set_rect(WIDGET(c->frame), x, y, w, h);
}

void set_client_rect_by_frame(Client *c)
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
