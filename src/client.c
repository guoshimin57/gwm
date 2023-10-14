/* *************************************************************************
 *     client.c：實現X client相關功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static Client *new_client(WM *wm, Window win);
static bool should_hide_frame(Client *c);
static void apply_rules(WM *wm, Client *c);
static void set_default_desktop_mask(WM *wm, Client *c);
static void set_default_place_type(WM *wm, Client *c);
static bool should_float(WM *wm, Client *c);
static bool is_max_state(Client *c);
static bool have_rule(const Rule *r, Client *c);
static Client *get_head_for_add_client(WM *wm, Client *c);
static void add_client_node(Client *head, Client *c);
static void set_default_win_rect(WM *wm, Client *c);
static void set_win_rect_by_attr(WM *wm, Client *c);
static bool fix_win_pos_by_hint(Client *c);
static void fix_win_pos_by_prop(WM *wm, Client *c);
static void set_transient_win_pos(Client *c);
static void fix_win_pos_by_workarea(WM *wm, Client *c);
static void fix_win_size(WM *wm, Client *c);
static void fix_win_size_by_workarea(WM *wm, Client *c);
static void frame_client(WM *wm, Client *c);
static Rect get_button_rect(WM *wm, Client *c, size_t index);
static void del_client_node(Client *c);
static Rect get_frame_rect(Client *c);
static Window get_top_win(WM *wm, Client *c);
static bool is_valid_move(WM *wm, Client *from, Client *to, Place_type type);
static bool is_valid_to_normal_layer_sec(WM *wm, Client *c);
static bool move_client_node(WM *wm, Client *from, Client *to, Place_type type);
static void add_subgroup(Client *head, Client *subgroup_leader);
static void del_subgroup(Client *subgroup_leader);
static void set_place_type_for_subgroup(Client *subgroup_leader, Place_type type);
static int cmp_client_store_order(WM *wm, Client *c1, Client *c2);
static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c);
static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_icon_client_head(WM *wm);
static bool have_same_class_icon_client(WM *wm, Client *c);
static void get_max_rect(WM *wm, Client *c, int *left_x, int *top_y, int *max_w, int *max_h, int *mid_x, int *mid_y, int *half_w, int *half_h);

void add_client(WM *wm, Window win)
{
    Client *c=new_client(wm, win);

    apply_rules(wm, c);
    add_client_node(get_head_for_add_client(wm, c), c);
    fix_place_type(wm);
    c->old_place_type=c->place_type;
    set_default_win_rect(wm, c);
    set_icon_image(wm, c);
    grab_buttons(wm, c);
    XSelectInput(wm->display, win, EnterWindowMask|PropertyChangeMask);
    XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
    frame_client(wm, c);
    update_layout(wm);
    XMapWindow(wm->display, c->frame);
    XMapSubwindows(wm->display, c->frame);
    focus_client(wm, wm->cur_desktop, c);
    set_all_net_client_list(wm);
}

static Client *new_client(WM *wm, Window win)
{
    Client *c=malloc_s(sizeof(Client));
    memset(c, 0, sizeof(Client));
    c->win=win;
    c->map_n=++wm->map_count;
    c->title_text=get_title_text(wm, win, "");
    c->wm_hint=XGetWMHints(wm->display, win);
    c->win_type=get_net_wm_win_type(wm, win);
    c->win_state=get_net_wm_state(wm, win);
    c->owner=win_to_client(wm, get_transient_for(wm, c->win));
    c->subgroup_leader=get_subgroup_leader(c);
    if(!should_hide_frame(c))
        c->border_w=wm->cfg->border_width, c->titlebar_h=TITLEBAR_HEIGHT(wm);
    c->class_hint.res_class=c->class_hint.res_name=NULL, c->class_name="?";
    update_size_hint(wm, c);
    set_default_place_type(wm, c);
    set_default_desktop_mask(wm, c);

    return c;
}

static bool should_hide_frame(Client *c)
{
    Net_wm_win_type t=c->win_type;
    
    return t.desktop || t.dock || t.menu || t.dropdown_menu || t.popup_menu
        || t.splash || t.tooltip || t.notification || t.combo || t.dnd; 
}

static void set_default_place_type(WM *wm, Client *c)
{
    if(c->owner)                     c->place_type = c->owner->place_type;
    else if(c->win_type.desktop)     c->place_type = DESKTOP_LAYER;
    else if(c->win_state.below)      c->place_type = BELOW_LAYER;
    else if(c->win_type.dock)        c->place_type = DOCK_LAYER;
    else if(c->win_state.above)      c->place_type = ABOVE_LAYER;
    else if(c->win_state.fullscreen) c->place_type = FULLSCREEN_LAYER;
    else if(should_float(wm, c))     c->place_type = FLOAT_LAYER;
    else                             c->place_type = TILE_LAYER_MAIN;
}

static bool should_float(WM *wm, Client *c)
{
    Layout layout=DESKTOP(wm)->cur_layout;
    return layout==STACK || (layout==TILE && is_max_state(c));
}

static bool is_max_state(Client *c)
{
    Net_wm_state *s=&c->win_state;
    return s->vmax || s->hmax || s->tmax || s->bmax || s->lmax || s->rmax;
}

static void set_default_desktop_mask(WM *wm, Client *c)
{
    if(c->win_state.sticky)
        c->desktop_mask=~0U;
    else
    {
        unsigned int desktop;
        unsigned char *p=get_prop(wm, c->win, wm->ewmh_atom[NET_WM_DESKTOP], NULL);

        desktop = p ? *(unsigned long *)p : wm->cur_desktop-1;
        XFree(p);
        c->desktop_mask = desktop==~0U ? desktop : get_desktop_mask(desktop+1);
    }
}

static void apply_rules(WM *wm, Client *c)
{
    if(!XGetClassHint(wm->display, c->win, &c->class_hint))
        return;

    c->class_name=c->class_hint.res_class;
    for(const Rule *r=wm->cfg->rule; r->app_class; r++)
    {
        if(have_rule(r, c))
        {
            c->place_type=r->place_type;
            if(!r->show_border)
                c->border_w=0;
            if(!r->show_titlebar)
                c->titlebar_h=0;
            if(r->desktop_mask)
                c->desktop_mask=r->desktop_mask;
            if(r->class_alias)
                c->class_name=r->class_alias;
        }
    }
}

static bool have_rule(const Rule *r, Client *c)
{
    const char *pc=r->app_class, *pn=r->app_name,
        *class=c->class_hint.res_class, *name=c->class_hint.res_name;
    return ((pc && ((class && strstr(class, pc)) || strcmp(pc, "*")==0))
        || ((pn && ((name && strstr(name, pn)) || strcmp(pc, "*")==0))));
}

static Client *get_head_for_add_client(WM *wm, Client *c)
{
    Client *top=NULL, *head=NULL;
    if(c->owner)
    {
        top=get_top_transient_client(c->subgroup_leader, false);
        head = top ? top->prev : c->owner->prev;
    }
    else
        head=get_head_client(wm, c->place_type);
    return head;
}

static void add_client_node(Client *head, Client *c)
{
    c->prev=head;
    c->next=head->next;
    head->next=c;
    c->next->prev=c;
}

void fix_place_type(WM *wm)
{
    int n=0, m=DESKTOP(wm)->n_main_max;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_on_cur_desktop(wm, c) && !c->icon && !c->owner)
        {
            if(c->place_type==TILE_LAYER_MAIN && ++n>m)
                c->place_type=TILE_LAYER_SECOND;
            else if(c->place_type==TILE_LAYER_SECOND && n<m)
                c->place_type=TILE_LAYER_MAIN, n++;
        }
    }
}

void set_win_rect_by_frame(Client *c, const Rect *frame)
{
    c->x=frame->x+c->border_w, c->y=frame->y+c->titlebar_h+c->border_w;
    c->w=frame->w-2*c->border_w, c->h=frame->h-c->titlebar_h-2*c->border_w;
}

static void set_default_win_rect(WM *wm, Client *c)
{
    int bw=c->border_w, th=c->titlebar_h, wx=wm->workarea.x, wy=wm->workarea.y,
        ww=wm->workarea.w, wh=wm->workarea.h;

    if(c->win_state.vmax)
        c->x=wx+bw, c->w=ww-2*bw;
    if(c->win_state.hmax)
        c->y=wy+bw+th, c->h=wh-th-2*bw;
    if(c->win_state.fullscreen)
        c->x=c->y=0, c->w=wm->screen_width, c->h=wm->screen_height;

    if(!is_max_state(c) && !c->win_state.fullscreen)
    {
        set_win_rect_by_attr(wm, c);
        fix_win_size(wm, c);
        fix_win_pos(wm, c);
    }

    save_place_info_of_client(c);
}

static void set_win_rect_by_attr(WM *wm, Client *c)
{
    XWindowAttributes a={.x=wm->workarea.x, .y=wm->workarea.y,
        .width=wm->workarea.w/4, .height=wm->workarea.h/4};
    XGetWindowAttributes(wm->display, c->win, &a);
    c->x=a.x, c->y=a.y, c->w=a.width, c->h=a.height;
}

void fix_win_pos(WM *wm, Client *c)
{
    if(!fix_win_pos_by_hint(c))
        fix_win_pos_by_prop(wm, c), fix_win_pos_by_workarea(wm, c);
}

static bool fix_win_pos_by_hint(Client *c)
{
    XSizeHints *p=&c->size_hint;
    if((p->flags & USPosition) || (p->flags & PPosition))
        c->x=p->x+c->border_w, c->y=p->y+c->border_w+c->titlebar_h;
    return (p->flags & USPosition) || (p->flags & PPosition);
}

static void fix_win_pos_by_prop(WM *wm, Client *c)
{
    if(c->owner)
        set_transient_win_pos(c);
    else if(c->win_type.dialog)
        c->x=wm->workarea.x+(wm->workarea.w-c->w)/2,
        c->y=wm->workarea.y+(wm->workarea.h-c->h)/2;
}

static void set_transient_win_pos(Client *c)
{
    c->x=c->owner->x+(c->owner->w-c->w)/2;
    c->y=c->owner->y+(c->owner->h-c->h)/2;
}

static void fix_win_pos_by_workarea(WM *wm, Client *c)
{
    int w=c->w, h=c->h, bw=c->border_w, bh=c->titlebar_h, wx=wm->workarea.x,
         wy=wm->workarea.y, ww=wm->workarea.w, wh=wm->workarea.h;
    if(c->x >= wx+ww-w-bw) // 窗口在工作區右邊出界
        c->x=wx+ww-w-bw;
    if(c->x < wx+bw) // 窗口在工作區左邊出界
        c->x=wx+bw;
    if(c->y >= wy+wh-bw-h) // 窗口在工作區下邊出界
        c->y=wy+wh-bw-h;
    if(c->y < wy+bw+bh) // 窗口在工作區上邊出界
        c->y=wy+bw+bh;
}

static void fix_win_size(WM *wm, Client *c)
{
    fix_win_size_by_hint(c);
    fix_win_size_by_workarea(wm, c);
}

static void fix_win_size_by_workarea(WM *wm, Client *c)
{
    long ww=wm->workarea.w, wh=wm->workarea.h, bh=c->titlebar_h, bw=c->border_w;
    if(c->w+2*bw > ww)
        c->w=ww-2*bw;
    if(c->h+bh+2*bw > wh)
        c->h=wh-bh-2*bw;
}

static void frame_client(WM *wm, Client *c)
{
    Rect fr=get_frame_rect(c);
    c->frame=create_widget_win(wm, wm->root_win, fr.x, fr.y, fr.w, fr.h,
        c->border_w, WIDGET_COLOR(wm, CURRENT_BORDER), 0);
    XSelectInput(wm->display, c->frame, FRAME_EVENT_MASK);
    if(wm->cfg->set_frame_prop)
        copy_prop(wm, c->frame, c->win);
    if(c->titlebar_h)
        create_titlebar(wm, c);
    XAddToSaveSet(wm->display, c->win);
    XReparentWindow(wm->display, c->win, c->frame, 0, c->titlebar_h);
    
    /* 以下是同時設置窗口前景和背景透明度的非EWMH標準方法：
    unsigned long opacity = (unsigned long)(0xfffffffful);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(wm->display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(wm->display, c->frame, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&opacity, 1L);
    */
}

void create_titlebar(WM *wm, Client *c)
{
    Rect tr=get_title_area_rect(wm, c);
    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=get_button_rect(wm, c, i);
        c->buttons[i]=create_widget_win(wm, c->frame, br.x, br.y,
            br.w, br.h, 0, 0, WIDGET_COLOR(wm, CURRENT_TITLEBAR));
        XSelectInput(wm->display, c->buttons[i], BUTTON_EVENT_MASK);
    }
    c->title_area=create_widget_win(wm, c->frame, tr.x, tr.y,
        tr.w, tr.h, 0, 0, WIDGET_COLOR(wm, CURRENT_TITLEBAR));
    XSelectInput(wm->display, c->title_area, TITLE_AREA_EVENT_MASK);
    c->logo=create_widget_win(wm, c->frame, 0, 0, c->titlebar_h,
        c->titlebar_h, 0, 0, WIDGET_COLOR(wm, CURRENT_TITLEBAR));
    XSelectInput(wm->display, c->logo, BUTTON_EVENT_MASK);
}

static Rect get_frame_rect(Client *c)
{
    long bw=c->border_w, bh=c->titlebar_h;
    return (Rect){c->x-bw, c->y-bh-bw, c->w, c->h+bh};
}

Rect get_title_area_rect(WM *wm, Client *c)
{
    int buttons_n[]={[FULL]=c->owner ? 1 : 0, [PREVIEW]=1, [STACK]=3, [TILE]=7},
        n=buttons_n[DESKTOP(wm)->cur_layout], size=TITLEBAR_HEIGHT(wm);
    return (Rect){size, 0, c->w-wm->cfg->title_button_width*n-size, size};
}

static Rect get_button_rect(WM *wm, Client *c, size_t index)
{
    int cw=c->w, w=wm->cfg->title_button_width, h=TITLEBAR_HEIGHT(wm);
    return (Rect){cw-w*(TITLE_BUTTON_N-index), 0, w, h};
}

int get_clients_n(WM *wm, Place_type type, bool count_icon, bool count_trans, bool count_all_desktop)
{
    int n=0;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if( (type==ANY_PLACE || c->place_type==type)
            && (count_icon || !c->icon)
            && (count_trans || !c->owner)
            && (count_all_desktop || is_on_cur_desktop(wm, c)))
            n++;
    return n;
}

Client *win_to_client(WM *wm, Window win)
{
    // 當隱藏標題欄時，標題區和按鈕的窗口ID爲0。故win爲0時，不應視爲找到
    for(Client *c=wm->clients->next; win && c!=wm->clients; c=c->next)
    {
        if(win==c->win || win==c->frame || win==c->logo || win==c->title_area)
            return c;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            if(win == c->buttons[i])
                return c;
    }
        
    return NULL;
}

void del_client(WM *wm, Client *c, bool is_for_quit)
{
    if(!c)
        return;

    if(is_win_exist(wm, c->win, c->frame))
        XReparentWindow(wm->display, c->win, wm->root_win, c->x, c->y);
    XDestroyWindow(wm->display, c->frame);
    if(c->image)
        imlib_context_set_image(c->image), imlib_free_image();
    del_client_node(c);
    fix_place_type(wm);
    if(!is_for_quit)
        for(size_t i=1; i<=DESKTOP_N; i++)
            if(is_on_desktop_n(i, c))
                focus_client(wm, i, NULL);

    XFree(c->class_hint.res_class);
    XFree(c->class_hint.res_name);
    XFree(c->wm_hint);
    vfree(c->title_text, c, NULL);

    if(!is_for_quit)
        update_layout(wm);
    set_all_net_client_list(wm);
}

static void del_client_node(Client *c)
{
    c->prev->next=c->next;
    c->next->prev=c->prev;
}

void move_resize_client(WM *wm, Client *c, const Delta_rect *d)
{
    if(d)
        c->x+=d->dx, c->y+=d->dy, c->w+=d->dw, c->h+=d->dh;
    Rect fr=get_frame_rect(c), tr=get_title_area_rect(wm, c);
    if(c->titlebar_h)
    {
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
        {
            Rect br=get_button_rect(wm, c, i);
            XMoveWindow(wm->display, c->buttons[i], br.x, br.y);
        }
        XResizeWindow(wm->display, c->title_area, tr.w, tr.h);
    }
    XMoveResizeWindow(wm->display, c->frame, fr.x, fr.y, fr.w, fr.h);
    XMoveResizeWindow(wm->display, c->win, 0, c->titlebar_h, c->w, c->h);
}

Client *win_to_iconic_state_client(WM *wm, Window win)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(c->icon && c->icon->win==win)
            return c;
    return NULL;
}

/* 僅在移動窗口、聚焦窗口時或窗口類型、狀態發生變化才有可能需要提升 */
void raise_client(WM *wm, Client *c)
{
    int n=get_subgroup_n(c), i=n;
    Window wins[n+1];

    wins[0]=get_top_win(wm, c);
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
        wins[i--]=p->frame;

    XRestackWindows(wm->display, wins, n+1);
    set_all_net_client_list(wm);
}

static Window get_top_win(WM *wm, Client *c)
{
    size_t index[]=
    {
        [FULLSCREEN_LAYER]=FULLSCREEN_TOP,
        [ABOVE_LAYER]=ABOVE_TOP,
        [DOCK_LAYER]=DOCK_TOP,
        [FLOAT_LAYER]=FLOAT_TOP,
        [TILE_LAYER_MAIN]=NORMAL_TOP,
        [TILE_LAYER_SECOND]=NORMAL_TOP,
        [TILE_LAYER_FIXED]=NORMAL_TOP,
        [BELOW_LAYER]=BELOW_TOP,
        [DESKTOP_LAYER]=DESKTOP_TOP,
    };
    return wm->top_wins[index[c->place_type]];
}

/* 當c->win所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_next_client(WM *wm, Client *c)
{
    for(Client *m=NULL, *p=c->next; p!=wm->clients; p=p->next)
        if(is_on_cur_desktop(wm, p))
            return (m=get_top_transient_client(p->subgroup_leader, true)) ? m : p;
    return NULL;
}

/* 當c->win所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_prev_client(WM *wm, Client *c)
{
    for(Client *m=NULL, *p=c->prev; p!=wm->clients; p=p->prev)
        if(is_on_cur_desktop(wm, p))
            return (m=get_top_transient_client(p->subgroup_leader, true)) ? m : p;
    return NULL;
}

void move_client(WM *wm, Client *from, Client *to, Place_type type)
{
    if(move_client_node(wm, from, to, type))
    {
        set_place_type_for_subgroup(from->subgroup_leader,
            to ? to->place_type : type);
        fix_place_type(wm);
        raise_client(wm, from);
        update_layout(wm);
    }
}

bool is_normal_layer(Place_type t)
{
    return t==TILE_LAYER_MAIN || t==TILE_LAYER_SECOND || t==TILE_LAYER_FIXED;
}

static bool is_valid_move(WM *wm, Client *from, Client *to, Place_type type)
{
    Layout l=DESKTOP(wm)->cur_layout;
    Place_type t = to ? to->place_type : type;

    return from != wm->clients
        && (!to || from->subgroup_leader!=to->subgroup_leader)
        && (t!=TILE_LAYER_SECOND || is_valid_to_normal_layer_sec(wm, from))
        && (l==TILE || !is_normal_layer(t));
}

static bool is_valid_to_normal_layer_sec(WM *wm, Client *c)
{
    return c->place_type!=TILE_LAYER_MAIN
        || get_clients_n(wm, TILE_LAYER_SECOND, false, false, false);
}

static bool move_client_node(WM *wm, Client *from, Client *to, Place_type type)
{
    if(!is_valid_move(wm, from, to, type))
        return false;

    Client *head=NULL;
    del_subgroup(from->subgroup_leader);
    if(to)
        head = cmp_client_store_order(wm, from, to) < 0 ? to : to->prev;
    else
    {
        head=get_head_client(wm, type);
        if(from->place_type==TILE_LAYER_MAIN && type==TILE_LAYER_SECOND)
            head=head->next;
    }
    add_subgroup(head, from->subgroup_leader);
    return true;
}

static void add_subgroup(Client *head, Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *begin=(top ? top : subgroup_leader), *end=subgroup_leader;

    begin->prev=head;
    end->next=head->next;
    head->next=begin;
    end->next->prev=end;
}

static void del_subgroup(Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *begin=(top ? top : subgroup_leader), *end=subgroup_leader;

    begin->prev->next=end->next;
    end->next->prev=begin->prev;
}

static void set_place_type_for_subgroup(Client *subgroup_leader, Place_type type)
{
    for(Client *ld=subgroup_leader, *c=ld; ld && c->subgroup_leader==ld; c=c->prev)
        c->place_type=type;
}

void swap_clients(WM *wm, Client *a, Client *b)
{
    if(a->subgroup_leader == b->subgroup_leader)
        return;

    Client *tmp, *top, *a_begin, *b_begin, *a_prev, *a_leader, *b_leader, *oa=a;

    if(cmp_client_store_order(wm, a, b) > 0)
        tmp=a, a=b, b=tmp;

    a_leader=a->subgroup_leader, b_leader=b->subgroup_leader;

    top=get_top_transient_client(a_leader, false);
    a_begin=(top ? top : a_leader);
    a_prev=a_begin->prev;

    top=get_top_transient_client(b_leader, false);
    b_begin=(top ? top : b_leader);

    del_subgroup(a_leader);
    add_subgroup(b_leader, a_leader);
    if(a_leader->next != b_begin) //不相邻
        del_subgroup(b_leader), add_subgroup(a_prev, b_leader);

    raise_client(wm, oa);
    update_layout(wm);
}

static int cmp_client_store_order(WM *wm, Client *c1, Client *c2)
{
    if(c1 == c2)
        return 0;
    for(Client *c=c1; c!=wm->clients; c=c->next)
        if(c == c2)
            return -1;
    return 1;
}

bool is_last_typed_client(WM *wm, Client *c, Place_type type)
{
    for(Client *p=wm->clients->prev; p!=c; p=p->prev)
        if(is_on_cur_desktop(wm, p) && p->place_type==type)
            return false;
    return true;
}

Client *get_head_client(WM *wm, Place_type type)
{
    Client *head=wm->clients;
    for(Client *c=head->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->place_type==type)
            return c->prev;
    for(Client *c=head->next; c!=wm->clients; c=c->next)
        if(c->place_type == type)
            return c->prev;
    for(Client *c=head->prev; c!=wm->clients; c=c->prev)
        if(c->place_type > type)
            return c->prev;
    return head->prev;
}

int get_subgroup_n(Client *c)
{
    int n=0;
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
        n++;
    return n;
}

Client *get_subgroup_leader(Client *c)
{
    for(; c && c->owner; c=c->owner)
        ;
    return c;
}

Client *get_top_transient_client(Client *subgroup_leader, bool only_modal)
{
    Client *result=NULL, *ld=subgroup_leader;

    for(Client *c=ld; ld && c->subgroup_leader==ld; c=c->prev)
        if((only_modal && c->win_state.modal) || c->owner)
            result=c;

    return result;
}

/* 若在調用本函數之前cur_focus_client或prev_focus_client因某些原因（如移動到
 * 其他虛擬桌面、刪除、縮微）而未更新時，則應使用值爲NULL的c來調用本函數。這
 * 樣會自動推斷出合適的規則來取消原聚焦和聚焦新的client。*/
void focus_client(WM *wm, unsigned int desktop_n, Client *c)
{
    if(have_urgency(wm, desktop_n))
        set_urgency(wm, c, false);
    if(have_attention(wm, desktop_n))
        set_attention(wm, c, false);
    update_focus_client_pointer(wm, desktop_n, c);

    Desktop *d=wm->desktop[desktop_n-1];
    Client *pc=d->cur_focus_client;

    if(desktop_n == wm->cur_desktop)
    {
        if(pc->win == wm->root_win)
            XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
        else if(!pc->icon)
            set_input_focus(wm, pc->wm_hint, pc->win);
    }
    update_client_bg(wm, desktop_n, pc);
    update_client_bg(wm, desktop_n, d->prev_focus_client);
    raise_client(wm, pc);
    set_net_active_window(wm);
}

static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c)
{
    Desktop *d=wm->desktop[desktop_n-1];
    Client *p=NULL, **pp=&d->prev_focus_client, **pc=&d->cur_focus_client;

    if(!c)  // 當某個client在desktop_n中變得不可見時，即既有可能被刪除了，
    {       // 也可能是被縮微化了，還有可能是移動到其他虛擬桌面了。
        if(is_map_client(wm, desktop_n, *pc)) // 非當前窗口被非wm手段關閉（如kill）
            return;
        p = (*pc)->owner ? (*pc)->owner : *pp;
        if(is_map_client(wm, desktop_n, p))
            *pc=p;
        else if((p=get_prev_map_client(wm, desktop_n, *pp)))
            *pc=p;
        else if((p=get_next_map_client(wm, desktop_n, *pp)))
            *pc=p;
        else
            *pc=wm->clients;

        if(is_map_client(wm, desktop_n, *pp))
           return;
        else if(is_map_client(wm, desktop_n, (*pp)->owner))
            *pp=(*pp)->owner;
        else if((p=get_prev_map_client(wm, desktop_n, *pp)))
            *pp=p;
        else if((p=get_next_map_client(wm, desktop_n, *pp)))
            *pp=p;
        else
            *pp=wm->clients;
    }
    else if(c != *pc)
        *pp=*pc, *pc=((p=get_top_transient_client(c->subgroup_leader, true)) ? p : c);
}

static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    if(c && !c->icon && is_on_desktop_n(desktop_n, c))
        for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
            if(p == c)
                return true;
    return false;
}

/* 取得存儲結構意義上的上一個處於映射狀態的客戶窗口 */
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->prev; p!=wm->clients; p=p->prev)
        if(!p->icon && is_on_desktop_n(desktop_n, p))
            return p;
    return NULL;
}

/* 取得存儲結構意義上的下一個處於映射狀態的客戶窗口 */
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->next; p!=wm->clients; p=p->next)
        if(!p->icon && is_on_desktop_n(desktop_n, p))
            return p;
    return NULL;
}

bool is_on_desktop_n(unsigned int n, Client *c)
{
    return (c->desktop_mask & get_desktop_mask(n));
}

bool is_on_cur_desktop(WM *wm, Client *c)
{
    return (c->desktop_mask & get_desktop_mask(wm->cur_desktop));
}

unsigned int get_desktop_mask(unsigned int desktop_n)
{
    return 1<<(desktop_n-1);
}

void iconify(WM *wm, Client *c)
{
    if(c->win_state.skip_taskbar)
        return;

    move_client_node(wm, c, get_icon_client_head(wm), ANY_PLACE);
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
    {
        create_icon(wm, p);
        p->icon->title_text=get_icon_title_text(wm, p->win, p->title_text);
        update_win_bg(wm, p->icon->win, WIDGET_COLOR(wm, TASKBAR), None);
        update_icon_area(wm);
        XMapWindow(wm->display, p->icon->win);
        XUnmapWindow(wm->display, p->frame);
        if(p == DESKTOP(wm)->cur_focus_client)
        {
            focus_client(wm, wm->cur_desktop, NULL);
            update_frame_bg(wm, wm->cur_desktop, p);
        }
        p->win_state.hidden=1;
        update_net_wm_state(wm, p);
    }
    update_layout(wm);
}

static Client *get_icon_client_head(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->icon)
            return c->prev;
    return wm->clients->prev;
}

void create_icon(WM *wm, Client *c)
{
    Icon *i=c->icon=malloc_s(sizeof(Icon));
    i->x=i->y=0, i->w=i->h=wm->taskbar->h;
    i->title_text=NULL; // 有的窗口映射時未設置圖標標題，故應延後至縮微窗口時再設置title_text
    i->win=create_widget_win(wm, wm->taskbar->icon_area, 0, 0,
        i->w, i->h, 0, 0, WIDGET_COLOR(wm, TASKBAR));
    XSelectInput(wm->display, c->icon->win, ICON_WIN_EVENT_MASK);
}

void update_icon_area(WM *wm)
{
    int x=0, w=0;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(is_on_cur_desktop(wm, c) && c->icon)
        {
            Icon *i=c->icon;
            i->w=wm->taskbar->h;
            if(have_same_class_icon_client(wm, c))
            {
                get_string_size(wm, wm->font[TITLEBAR_FONT], i->title_text, &w, NULL);
                i->w=MIN(i->w+w, wm->cfg->icon_win_width_max);
                i->show_text=true;
            }
            else
                i->show_text=false;
            i->x=x;
            x+=i->w+wm->cfg->icon_gap;
            XMoveResizeWindow(wm->display, i->win, i->x, i->y, i->w, i->h); 
        }
    }
}

static bool have_same_class_icon_client(WM *wm, Client *c)
{
    for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
        if( p!=c && is_on_cur_desktop(wm, p) && p->icon
            && !strcmp(c->class_name, p->class_name))
            return true;
    return false;
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
            XMapWindow(wm->display, p->frame);
            update_icon_area(wm);
            focus_client(wm, wm->cur_desktop, p);
            p->win_state.hidden=0;
            update_net_wm_state(wm, p);
        }
    }
    update_layout(wm);
}

void del_icon(WM *wm, Client *c)
{
    if(c->icon)
    {
        XDestroyWindow(wm->display, c->icon->win);
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

void update_win_state_for_move_resize(WM *wm, Client *c)
{
    int x=c->x, y=c->y, w=c->w, h=c->h, left_x, top_y, max_w, max_h,
        mid_x, mid_y, half_w, half_h;
    bool update=false;
    Net_wm_state *s=&c->win_state;

    get_max_rect(wm, c, &left_x, &top_y, &max_w, &max_h, &mid_x, &mid_y, &half_w, &half_h);
    if(s->vmax && (y!=top_y || h!=max_h))
        s->vmax=0, update=true;

    if(s->hmax && (x!=left_x || w!=max_w))
        s->hmax=0, update=true;

    if( s->tmax && (x!=left_x || y!=top_y || w!=max_w || h!=half_h))
        s->tmax=0, update=true;

    if( s->bmax && (x!=left_x || y!=mid_y || w!=max_w || h!=half_h))
        s->bmax=0, update=true;

    if( s->lmax && (x!=left_x || y!=top_y || w!=half_w || h!=max_h))
        s->lmax=0, update=true;

    if( s->rmax && (x!=mid_x || y!=top_y || w!=half_w || h!=max_h))
        s->rmax=0, update=true;

    if(update)
        update_net_wm_state(wm, c);
}

static void get_max_rect(WM *wm, Client *c, int *left_x, int *top_y, int *max_w, int *max_h, int *mid_x, int *mid_y, int *half_w, int *half_h)
{
    int bw=c->border_w, th=c->titlebar_h, wx=wm->workarea.x, wy=wm->workarea.y,
        ww=wm->workarea.w, wh=wm->workarea.h;
    *left_x=wx+bw, *top_y=wy+bw+th, *max_w=ww-2*bw, *max_h=wh-th-2*bw,
    *mid_x=*left_x+ww/2, *mid_y=*top_y+wh/2, *half_w=ww/2-2*bw, *half_h=wh/2-th-2*bw;
}

void update_net_wm_state(WM *wm, Client *c)
{
    Atom *a=wm->ewmh_atom;
    unsigned long n=0, val[17]={0}; // 目前EWMH規範中NET_WM_STATE共有13種狀態，GWM自定義4種狀態

    if(c->win_state.modal)          val[n++]=a[NET_WM_STATE_MODAL];
    if(c->win_state.sticky)         val[n++]=a[NET_WM_STATE_STICKY];
    if(c->win_state.vmax)           val[n++]=a[NET_WM_STATE_MAXIMIZED_VERT];
    if(c->win_state.hmax)           val[n++]=a[NET_WM_STATE_MAXIMIZED_HORZ];
    if(c->win_state.tmax)           val[n++]=a[GWM_WM_STATE_MAXIMIZED_TOP];
    if(c->win_state.bmax)           val[n++]=a[GWM_WM_STATE_MAXIMIZED_BOTTOM];
    if(c->win_state.lmax)           val[n++]=a[GWM_WM_STATE_MAXIMIZED_LEFT];
    if(c->win_state.rmax)           val[n++]=a[GWM_WM_STATE_MAXIMIZED_RIGHT];
    if(c->win_state.shaded)         val[n++]=a[NET_WM_STATE_SHADED];
    if(c->win_state.skip_taskbar)   val[n++]=a[NET_WM_STATE_SKIP_TASKBAR];
    if(c->win_state.skip_pager)     val[n++]=a[NET_WM_STATE_SKIP_PAGER];
    if(c->win_state.hidden)         val[n++]=a[NET_WM_STATE_HIDDEN];
    if(c->win_state.fullscreen)     val[n++]=a[NET_WM_STATE_FULLSCREEN];
    if(c->win_state.above)          val[n++]=a[NET_WM_STATE_ABOVE];
    if(c->win_state.below)          val[n++]=a[NET_WM_STATE_BELOW];
    if(c->win_state.attent)         val[n++]=a[NET_WM_STATE_DEMANDS_ATTENTION];
    if(c->win_state.focused)        val[n++]=a[NET_WM_STATE_FOCUSED];
    XChangeProperty(wm->display, c->win, a[NET_WM_STATE], XA_ATOM, 32,
        PropModeReplace, (unsigned char *)val, n);
}

void save_place_info_of_client(Client *c)
{
    c->ox=c->x, c->oy=c->y, c->ow=c->w, c->oh=c->h;
    c->old_place_type=c->place_type;
}

void save_place_info_of_clients(WM *wm)
{
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c))
            save_place_info_of_client(c);
}

void restore_place_info_of_client(Client *c)
{
    c->x=c->ox, c->y=c->oy, c->w=c->ow, c->h=c->oh;
    c->place_type=c->old_place_type;
}

void restore_place_info_of_clients(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            restore_place_info_of_client(c);
}

void restore_client(WM *wm, Client *c)
{
    restore_place_info_of_client(c);
    move_client(wm, c, NULL, c->place_type);
}

bool is_tile_client(WM *wm, Client *c)
{
    return is_on_cur_desktop(wm, c) && !c->owner && !c->icon
        && is_normal_layer(c->place_type);
}

void max_client(WM *wm, Client *c, Max_way max_way)
{
    int left_x, top_y, max_w, max_h, mid_x, mid_y, half_w, half_h;

    if(!is_win_state_max(c))
        save_place_info_of_client(c);
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

    Place_type type=get_dest_place_type_for_move(wm, c);
    move_client(wm, c, NULL, type);
    move_resize_client(wm, c, NULL);
}

Place_type get_dest_place_type_for_move(WM *wm, Client *c)
{
    return DESKTOP(wm)->cur_layout==TILE && is_tile_client(wm, c) ?
        FLOAT_LAYER : c->place_type;
}

bool is_win_state_max(Client *c)
{
    return c->win_state.vmax || c->win_state.hmax || c->win_state.tmax
        || c->win_state.bmax || c->win_state.lmax || c->win_state.rmax;
}
