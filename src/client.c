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

#include <X11/Xft/Xft.h>
#include "config.h"
#include "gwm.h"
#include "font.h"
#include "misc.h"
#include "list.h"
#include "image.h"
#include "icccm.h"
#include "prop.h"
#include "grab.h"
#include "rule_cfg.h"
#include "client.h"

static void client_ctor(Client *c, Window win);
static bool has_decoration(const Client *c);
static void create_frame(Client *c);
static void set_default_area(Client *c);
static void set_default_desktop_mask(Client *c);
static void apply_rules(Client *c);
static bool have_rule(const Rule *r, Client *c);
static void client_dtor(Client *c);
static Client *get_first_same_subgroup(const Client *c);
static Client *get_first_same_place_client(Layer layer, Area area);
static Client *get_next_area_client(Layer layer, Area area);
static Client *get_next_layer_client(Layer layer, Area area);
static void set_default_win_rect(Client *c);
static void set_client_rect_by_frame(Client *c);

static Client *clients=NULL;

void init_client_list(void)
{
    clients=Malloc(sizeof(Client));
    list_init(&clients->list);
}

Client *get_clients(void)
{
    return clients;
}

Client *client_new(Window win)
{
    Client *c=Malloc(sizeof(Client));
    client_ctor(c, win);
    list_add(&c->list, &get_head_client(c, ANY_LAYER, ANY_AREA)->list);
    grab_buttons(WIDGET_WIN(c));
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
    c->subgroup_leader = c->owner ? c->owner->subgroup_leader : c;
    c->class_name="?";
    c->image=get_win_icon_image(win);
    set_default_win_rect(c);
    create_frame(c);
    widget_set_draggable(WIDGET(c), true);
    XGetClassHint(xinfo.display, WIDGET_WIN(c), &c->class_hint);
    set_default_layer(c);
    set_default_area(c);
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

static void create_frame(Client *c)
{
    int th=c->decorative ? get_font_height_by_pad() : 0,
        bw=c->decorative ? cfg->border_width : 0;

    c->frame=frame_new(WIDGET(c), WIDGET_X(c), WIDGET_Y(c), WIDGET_W(c), WIDGET_H(c),
        th, bw, c->title_text, c->image);
}

void set_default_layer(Client *c)
{
    if(c->owner)                     c->layer = c->owner->layer;
    else if(c->win_type.desktop)     c->layer = DESKTOP_LAYER;
    else if(c->win_state.below)      c->layer = BELOW_LAYER;
    else if(c->win_state.above)      c->layer = ABOVE_LAYER;
    else if(c->win_type.dock)        c->layer = DOCK_LAYER;
    else if(c->win_state.fullscreen) c->layer = FULLSCREEN_LAYER;
    else if(get_gwm_layout()==TILE
        && !is_win_state_max(c->win_state))
                                     c->layer = TILE_LAYER;
    else                             c->layer = STACK_LAYER;
}

static void set_default_area(Client *c)
{
    c->area = get_gwm_layout()==TILE ? MAIN_AREA : ANY_AREA;
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
    for(const Rule *r=rules; r->app_class; r++)
    {
        if(have_rule(r, c))
        {
            if(r->layer != ANY_LAYER)
                c->layer=r->layer;
            if(r->area != ANY_AREA)
                c->area=r->area;
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

int get_clients_n(Layer layer, Area area, bool count_icon, bool count_trans, bool count_all_desktop)
{
    int n=0;
    clients_for_each(c)
        if( (layer==ANY_LAYER || c->layer==layer)
            && (area==ANY_AREA || c->area==area)
            && (count_icon || !is_iconic_client(c))
            && (count_trans || !c->owner)
            && (count_all_desktop || is_on_cur_desktop(c)))
            n++;
    return n;
}

bool is_iconic_client(const Client *c)
{
    return c->win_state.hidden;
}

Client *win_to_client(Window win)
{
    // 當隱藏標題欄時，標題區和按鈕的窗口ID爲0。故win爲0時，不應視爲找到
    clients_for_each(c)
        if(win==WIDGET_WIN(c) || frame_has_win(c->frame, win))
            return c;
    return NULL;
}

void client_del(Client *c)
{
    list_del(&c->list);
    client_dtor(c);
    widget_del(WIDGET(c));
}

static void client_dtor(Client *c)
{
    vXFree(c->class_hint.res_class, c->class_hint.res_name, c->wm_hint);
    Free(c->title_text);
    frame_del(c->frame), c->frame=NULL;
}

/* 當WIDGET_WIN(c)所在的亞組存在模態窗口時，跳過所有亞組窗口 */
Client *get_next(Client *c)
{
    Client *next=clients_next(c->win_state.modal ? c->subgroup_leader : c);
    return next==clients ? clients_next(clients) : next;
}

/* 當WIDGET_WIN(c)所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_prev(Client *c)
{
    Client *prev=clients_prev(c->win_state.modal ? c->subgroup_leader : c);
    return prev==clients ? clients_prev(clients) : prev;
}

bool is_place_last_client(Client *c)
{
    clients_for_each_reverse(p)
        if(is_on_cur_desktop(p) && p->layer==c->layer && p->area==c->area)
            return p==c;

    return false;
}

// 當c非空時，只使用c；否則只使用layer和area
Client *get_head_client(const Client *c, Layer layer, Area area)
{
    Client *p=NULL;

    if(c)
        layer=c->layer, area=c->area;

    if( (p=get_first_same_subgroup(c))
        || (p=get_first_same_place_client(layer, area))
        || (p=get_next_area_client(layer, area)))
        p=get_next_layer_client(layer, area);

    return p ? list_prev_entry(p, Client, list) : clients;
}

static Client *get_first_same_subgroup(const Client *c)
{
    if(c)
        clients_for_each(p)
            if( is_on_cur_desktop(p)
                && p->subgroup_leader==c->subgroup_leader)
                return p;
    return NULL;
}

static Client *get_first_same_place_client(Layer layer, Area area)
{
    clients_for_each(c)
        if( is_on_cur_desktop(c)
            && c->layer==layer && c->area==area)
            return c;
    return NULL;
}

static Client *get_next_area_client(Layer layer, Area area)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c) && c->layer==layer && c->area<area)
            return c;
    return NULL;
}

static Client *get_next_layer_client(Layer layer, Area area)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c) && c->layer<layer && c->area==area)
            return c;
    return NULL;
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
    c->olayer=c->layer, c->oarea=c->area;
}

void restore_place_info_of_client(Client *c)
{
    WIDGET_X(c)=c->ox, WIDGET_Y(c)=c->oy, WIDGET_W(c)=c->ow, WIDGET_H(c)=c->oh;
    c->layer=c->olayer, c->area=c->oarea;
}

bool is_tiling_client(Client *c)
{
    return !c->owner && !is_iconic_client(c) && c->layer==TILE_LAYER;
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

bool is_new_client(Client *c)
{
    return WIDGET_W(c->frame)<=WIDGET_W(c) && WIDGET_H(c->frame)<=WIDGET_H(c);
}

bool is_on_desktop_n(const Client *c, unsigned int n)
{
    return (c->desktop_mask & get_desktop_mask(n));
}

bool is_on_cur_desktop(const Client *c)
{
    return is_on_desktop_n(c, get_net_current_desktop());
}
