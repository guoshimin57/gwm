/* *************************************************************************
 *     client.c：實現X client相關功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
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
static void set_default_desktop_mask(Client *c, unsigned int cur_desktop);
static void apply_rules(Client *c);
static bool have_rule(const Rule *r, Client *c);
static Client *get_head_for_add_client(WM *wm, Client *c);
static void add_client_node(Client *head, Client *c);
static void fix_place_type_for_hint(WM *wm, Client *c);
static bool should_float(WM *wm, Client *c);
static bool is_max_state(Client *c);
static void set_default_win_rect(WM *wm, Client *c);
static void set_win_rect_by_attr(Client *c);
static void frame_client(WM *wm, Client *c);
static void del_client_node(Client *c);
static Window get_top_win(WM *wm, Client *c);
static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c);
static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c);
static bool have_same_class_icon_client(WM *wm, Client *c);
static int cmp_map_order(const void *pclient1, const void *pclient2);

void add_client(WM *wm, Window win)
{
    Client *c=new_client(wm, win);

    apply_rules(c);
    add_client_node(get_head_for_add_client(wm, c), c);
    grab_buttons(c->win);
    set_gwm_widget_type(win, CLIENT_WIN);
    XSelectInput(xinfo.display, win, EnterWindowMask|PropertyChangeMask);
    set_cursor(win, NO_OP);
    set_default_win_rect(wm, c);
    save_place_info_of_client(c);
    frame_client(wm, c);
    request_layout_update();
    XMapWindow(xinfo.display, c->frame);
    XMapSubwindows(xinfo.display, c->frame);
    focus_client(wm, wm->cur_desktop, c);
    set_net_wm_allowed_actions(c->win);
}

void set_all_net_client_list(WM *wm)
{
    int n=0;
    Window *wlist=get_client_win_list(wm, &n);

    set_net_client_list(wlist, n);
    free(wlist);

    wlist=get_client_win_list_stacking(wm, &n);
    set_net_client_list_stacking(wlist, n);
    free(wlist);
}

static Client *new_client(WM *wm, Window win)
{
    Client *c=malloc_s(sizeof(Client));
    memset(c, 0, sizeof(Client));
    c->win=win;
    c->map_n=++wm->map_count;
    c->title_text=get_title_text(win, "");
    c->wm_hint=XGetWMHints(xinfo.display, win);
    c->win_type=get_net_wm_win_type(win);
    c->win_state=get_net_wm_state(win);
    c->owner=win_to_client(wm, get_transient_for(c->win));
    c->subgroup_leader=get_subgroup_leader(c);
    c->place_type=TILE_LAYER_MAIN;
    fix_place_type_for_hint(wm, c);
    fix_place_type_for_tile(wm);
    if(!should_hide_frame(c))
        c->border_w=cfg->border_width, c->titlebar_h=get_font_height_by_pad();
    c->class_name="?";
    XGetClassHint(xinfo.display, c->win, &c->class_hint);
    c->image=get_icon_image(c->win, c->wm_hint, c->class_hint.res_name,
        cfg->icon_image_size, cfg->cur_icon_theme);
    set_default_desktop_mask(c, wm->cur_desktop);

    return c;
}

static bool should_hide_frame(Client *c)
{
    return (!c->win_type.none && !c->win_type.normal && !c->win_type.dialog)
        || c->win_state.skip_pager || c->win_state.skip_taskbar;
}

static void set_default_desktop_mask(Client *c, unsigned int cur_desktop)
{
    if(c->win_state.sticky)
        c->desktop_mask=~0U;
    else
    {
        unsigned int desktop;
        if(!get_net_wm_desktop(c->win, &desktop))
            desktop=cur_desktop-1;
        c->desktop_mask = desktop==~0U ? desktop : get_desktop_mask(desktop+1);
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
    const char *pc=r->app_class, *pn=r->app_name, *pt=r->title,
        *class=c->class_hint.res_class, *name=c->class_hint.res_name,
        *title=c->title_text;
    
    return((!pc || !class || !strcmp(class, pc) || !strcmp(pc, "*"))
        && (!pn || !name  || !strcmp(name, pn)  || !strcmp(pn, "*"))
        && (!pt || !title || !strcmp(title, pt) || !strcmp(pt, "*")));
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

static void fix_place_type_for_hint(WM *wm, Client *c)
{
    if(c->owner)                     c->place_type = c->owner->place_type;
    else if(c->win_type.desktop)     c->place_type = DESKTOP_LAYER;
    else if(c->win_state.below)      c->place_type = BELOW_LAYER;
    else if(c->win_type.dock)        c->place_type = DOCK_LAYER;
    else if(c->win_state.above)      c->place_type = ABOVE_LAYER;
    else if(c->win_state.fullscreen) c->place_type = FULLSCREEN_LAYER;
    else if(should_float(wm, c))     c->place_type = FLOAT_LAYER;
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

void fix_place_type_for_tile(WM *wm)
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
    c->x=wm->workarea.x, c->y=wm->workarea.y;
    c->w=wm->workarea.w/4, c->h=wm->workarea.h/4;
    set_win_rect_by_attr(c);
    fix_win_rect_by_state(wm, c);
}


static void set_win_rect_by_attr(Client *c)
{
    XWindowAttributes a;
    if(XGetWindowAttributes(xinfo.display, c->win, &a))
        c->x=a.x, c->y=a.y, c->w=a.width, c->h=a.height;
}

static void frame_client(WM *wm, Client *c)
{
    Rect fr=get_frame_rect(c);
    c->frame=create_widget_win(CLIENT_FRAME, xinfo.root_win, fr.x, fr.y,
        fr.w, fr.h, c->border_w, get_widget_color(CURRENT_BORDER_COLOR), 0);
    XSelectInput(xinfo.display, c->frame, FRAME_EVENT_MASK);
    if(cfg->set_frame_prop)
        copy_prop(c->frame, c->win);
    if(c->titlebar_h)
        create_titlebar(wm, c);
    XAddToSaveSet(xinfo.display, c->win);
    XReparentWindow(xinfo.display, c->win, c->frame, 0, c->titlebar_h);
    
    /* 以下是同時設置窗口前景和背景透明度的非EWMH標準方法：
    unsigned long opacity = (unsigned long)(0xfffffffful);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(xinfo.display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(xinfo.display, c->frame, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&opacity, 1L);
    */
}

void create_titlebar(WM *wm, Client *c)
{
    Rect tr=get_title_area_rect(wm, c);
    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=get_button_rect(c, i);
        c->buttons[i]=create_widget_win(TITLE_BUTTON_BEGIN+i, c->frame, br.x,
            br.y, br.w, br.h, 0, 0, get_widget_color(CURRENT_TITLEBAR_COLOR));
        XSelectInput(xinfo.display, c->buttons[i], BUTTON_EVENT_MASK);
    }
    c->title_area=create_widget_win(TITLE_AREA, c->frame, tr.x, tr.y,
        tr.w, tr.h, 0, 0, get_widget_color(CURRENT_TITLEBAR_COLOR));
    XSelectInput(xinfo.display, c->title_area, TITLE_AREA_EVENT_MASK);
    c->logo=create_widget_win(TITLE_LOGO, c->frame, 0, 0, c->titlebar_h,
        c->titlebar_h, 0, 0, get_widget_color(CURRENT_TITLEBAR_COLOR));
    XSelectInput(xinfo.display, c->logo, BUTTON_EVENT_MASK);
}

Rect get_frame_rect(Client *c)
{
    long bw=c->border_w, bh=c->titlebar_h;
    return (Rect){c->x-bw, c->y-bh-bw, c->w, c->h+bh};
}

Rect get_title_area_rect(WM *wm, Client *c)
{
    int buttons_n[]={[PREVIEW]=1, [STACK]=3, [TILE]=7},
        n=buttons_n[DESKTOP(wm)->cur_layout], size=get_font_height_by_pad();
    return (Rect){size, 0, c->w-cfg->title_button_width*n-size, size};
}

Rect get_button_rect(Client *c, size_t index)
{
    int cw=c->w, w=cfg->title_button_width, h=get_font_height_by_pad();
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

    XReparentWindow(xinfo.display, c->win, xinfo.root_win, c->x, c->y);
    XDestroyWindow(xinfo.display, c->frame);
    if(c->image)
        imlib_context_set_image(c->image), imlib_free_image();
    del_client_node(c);
    fix_place_type_for_tile(wm);
    if(!is_for_quit)
        for(size_t i=1; i<=DESKTOP_N; i++)
            if(is_on_desktop_n(i, c))
                focus_client(wm, i, NULL);

    XFree(c->class_hint.res_class);
    XFree(c->class_hint.res_name);
    XFree(c->wm_hint);
    vfree(c->title_text, c, NULL);

    if(!is_for_quit)
        request_layout_update();
    set_all_net_client_list(wm);
}

static void del_client_node(Client *c)
{
    c->prev->next=c->next;
    c->next->prev=c->prev;
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

    XRestackWindows(xinfo.display, wins, n+1);
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
    {
        if(is_on_cur_desktop(wm, p))
        {
            m=get_top_transient_client(p->subgroup_leader, true);
            if(!m || p==m)
                return p;
            else
                return p->subgroup_leader->next;
        }
    }
    return wm->clients;
}

/* 當c->win所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_prev_client(WM *wm, Client *c)
{
    for(Client *m=NULL, *p=c->prev; p!=c; p=p->prev)
        if(is_on_cur_desktop(wm, p))
            return (m=get_top_transient_client(p->subgroup_leader, true)) ? m : p;
    return wm->clients;
}

bool is_normal_layer(Place_type t)
{
    return t==TILE_LAYER_MAIN || t==TILE_LAYER_SECOND || t==TILE_LAYER_FIXED;
}

void add_subgroup(Client *head, Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *begin=(top ? top : subgroup_leader), *end=subgroup_leader;

    begin->prev=head;
    end->next=head->next;
    head->next=begin;
    end->next->prev=end;
}

void del_subgroup(Client *subgroup_leader)
{
    Client *top=get_top_transient_client(subgroup_leader, false),
           *begin=(top ? top : subgroup_leader), *end=subgroup_leader;

    begin->prev->next=end->next;
    end->next->prev=begin->prev;
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
        if(c->place_type < type)
            return c->prev;
    return head;
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
        if((only_modal && c->win_state.modal) || (!only_modal && c->owner))
            result=c;

    return result;
}

/* 若在調用本函數之前cur_focus_client或prev_focus_client因某些原因（如移動到
 * 其他虛擬桌面、刪除、縮微）而未更新時，則應使用值爲NULL的c來調用本函數。這
 * 樣會自動推斷出合適的規則來取消原聚焦和聚焦新的client。*/
void focus_client(WM *wm, unsigned int desktop_n, Client *c)
{
    update_focus_client_pointer(wm, desktop_n, c);

    Desktop *d=wm->desktop[desktop_n-1];
    Client *pc=d->cur_focus_client, *pp=d->prev_focus_client;

    if(desktop_n == wm->cur_desktop)
    {
        if(pc->win == xinfo.root_win)
            XSetInputFocus(xinfo.display, xinfo.root_win, RevertToPointerRoot, CurrentTime);
        else if(!pc->icon)
        {
            set_input_focus(pc->win, pc->wm_hint);
            if(taskbar->urgency_n[desktop_n-1])
                set_urgency(wm, pc, false);
            if(taskbar->attent_n[desktop_n-1])
                set_attention(wm, pc, false);
        }
    }
    update_client_bg(wm, desktop_n, pc);
    update_client_bg(wm, desktop_n, pp);
    raise_client(wm, pc);
    set_net_active_window(pc->win);
}

static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c)
{
    Desktop *d=wm->desktop[desktop_n-1];
    Client *p=NULL, **pp=&d->prev_focus_client, **pc=&d->cur_focus_client;

    if(!c)  // 當某個client在desktop_n中變得不可見時，即既有可能被刪除了，
    {       // 也可能是被縮微化了，還有可能是移動到其他虛擬桌面了。
        if(!is_map_client(wm, desktop_n, *pc)) // 非當前窗口被非wm手段關閉（如kill）
        {
            p = (*pc)->owner ? (*pc)->owner : *pp;
            if(is_map_client(wm, desktop_n, p))
                *pc=p;
            else if((p=get_prev_map_client(wm, desktop_n, *pp)))
                *pc=p;
            else if((p=get_next_map_client(wm, desktop_n, *pp)))
                *pc=p;
            else
                *pc=wm->clients;
        }

        if(!is_map_client(wm, desktop_n, *pp))
        {
            if(is_map_client(wm, desktop_n, (*pp)->owner))
                *pp=(*pp)->owner;
            else if((p=get_prev_map_client(wm, desktop_n, *pp)))
                *pp=p;
            else if((p=get_next_map_client(wm, desktop_n, *pp)))
                *pp=p;
            else
                *pp=wm->clients;
        }
    }
    else if(c != *pc)
    {
        p=get_top_transient_client(c->subgroup_leader, true);
        *pp=*pc, *pc=(p ? p : c);
    }
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

void update_icon_area(WM *wm)
{
    int x=0, w=0;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(is_on_cur_desktop(wm, c) && c->icon)
        {
            Icon *i=c->icon;
            i->w=taskbar->h;
            if(have_same_class_icon_client(wm, c))
            {
                get_string_size(i->title_text, &w, NULL);
                i->w=MIN(i->w+w, cfg->icon_win_width_max);
                i->show_text=true;
            }
            else
                i->show_text=false;
            i->x=x;
            x+=i->w+cfg->icon_gap;
            XMoveResizeWindow(xinfo.display, i->win, i->x, i->y, i->w, i->h); 
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

bool is_tile_client(WM *wm, Client *c)
{
    return is_on_cur_desktop(wm, c) && !c->owner && !c->icon
        && is_normal_layer(c->place_type);
}

Place_type get_dest_place_type_for_move(WM *wm, Client *c)
{
    return DESKTOP(wm)->cur_layout==TILE && is_tile_client(wm, c) ?
        FLOAT_LAYER : c->place_type;
}

/* 獲取按從早到遲的映射順序排列的客戶窗口列表 */
Window *get_client_win_list(WM *wm, int *n)
{
    int i=0, count=get_clients_n(wm, ANY_PLACE, true, true, true);

    if(count == 0)
        return NULL;

    if(n)
        *n=count;

    Window *wlist=malloc_s(count*sizeof(Window));
    Client **clist=malloc_s(count*sizeof(Client *));

    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        clist[i++]=c;
    qsort(clist, *n, sizeof(Client *), cmp_map_order);

    for(i=0; i<count; i++)
        wlist[i]=clist[i]->win;
    free(clist);

    return wlist;
}

static int cmp_map_order(const void *pclient1, const void *pclient2)
{
    return (*(Client **)pclient1)->map_n - (*(Client **)pclient2)->map_n;
}

/* 獲取按從下到上的疊次序排列的客戶窗口列表 */
Window *get_client_win_list_stacking(WM *wm, int *n)
{
    int count=get_clients_n(wm, ANY_PLACE, true, true, true);
    if(count == 0)
        return NULL;

    if(n)
        *n=count;

    Window *wlist=malloc_s(count*sizeof(Window)), *w=wlist;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev, w++)
        *w=c->win;

    return wlist;
}

void set_attention(WM *wm, Client *c, bool attent)
{
    if(c->win_state.attent == attent) // 避免重復設置
        return;

    c->win_state.attent=attent;
    update_net_wm_state(c->win, c->win_state);

    int incr = attent ? 1 : -1;
    for(unsigned int i=1; i<=DESKTOP_N; i++)
        if(is_on_desktop_n(i, c) && i!=wm->cur_desktop)
            taskbar->attent_n[i-1] += incr;
    update_taskbar_buttons_bg();
}

void set_urgency(WM *wm, Client *c, bool urg)
{
    if(!set_urgency_hint(c->win, c->wm_hint, urg))
        return;

    int incr = (urg ? 1 : -1);
    for(unsigned int i=1; i<=DESKTOP_N; i++)
        if(is_on_desktop_n(i, c) && i!=wm->cur_desktop)
            taskbar->urgency_n[i-1] += incr;
    update_taskbar_buttons_bg();
}

bool is_wm_win(WM *wm, Window win, bool before_wm)
{
    XWindowAttributes a;
    bool status=XGetWindowAttributes(xinfo.display, win, &a);

    if( !status || a.override_redirect
        || !is_on_screen( a.x, a.y, a.width, a.height))
        return false;

    if(!before_wm)
        return !win_to_client(wm, win);

    return is_iconic_state(win) || a.map_state==IsViewable;
}

void restack_win(WM *wm, Window win)
{
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        if(win==wm->top_wins[i])
            return;

    Client *c=win_to_client(wm, win);
    Net_wm_win_type type=get_net_wm_win_type(win);
    Window wins[2]={None, c ? c->frame : win};

    if(type.desktop)
        wins[0]=wm->top_wins[DESKTOP_TOP];
    else if(type.dock)
        wins[0]=wm->top_wins[DOCK_TOP];
    else if(c)
    {
        raise_client(wm, c);
        return;
    }
    else
        wins[0]=wm->top_wins[NORMAL_TOP];
    XRestackWindows(xinfo.display, wins, 2);
}

void update_clients_bg(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        update_client_bg(wm, wm->cur_desktop, c);
}

void update_client_bg(WM *wm, unsigned int desktop_n, Client *c)
{
    if(!c || c==wm->clients)
        return;

    Desktop *d=wm->desktop[desktop_n-1];
    if(c->icon && d->cur_layout!=PREVIEW)
        update_win_bg(c->icon->win, c==d->cur_focus_client ?
            get_widget_color(ENTERED_NORMAL_BUTTON_COLOR) :
            get_widget_color(TASKBAR_COLOR), None);
    else
        update_frame_bg(wm, desktop_n, c);
}

void update_frame_bg(WM *wm, unsigned int desktop_n, Client *c)
{
    bool cur=(c==wm->desktop[desktop_n-1]->cur_focus_client);
    unsigned long color=get_widget_color(cur ? CURRENT_BORDER_COLOR : NORMAL_BORDER_COLOR);

    if(c->border_w)
        XSetWindowBorder(xinfo.display, c->frame, color);
    if(c->titlebar_h)
    {
        color=get_widget_color(cur ? CURRENT_TITLEBAR_COLOR : NORMAL_TITLEBAR_COLOR);
        update_win_bg(c->logo, color, 0);
        update_win_bg(c->title_area, color, 0);
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            update_win_bg(c->buttons[i], color, None);
    }
}

/* 生成帶表頭結點的雙向循環鏈表 */
void create_clients(WM *wm)
{
    Window root, parent, *child=NULL;
    unsigned int n;
    Desktop **d=wm->desktop;

    wm->clients=malloc_s(sizeof(Client));
    memset(wm->clients, 0, sizeof(Client));
    for(size_t i=0; i<DESKTOP_N; i++)
        d[i]->cur_focus_client=d[i]->prev_focus_client=wm->clients;
    wm->clients->win=xinfo.root_win;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(!XQueryTree(xinfo.display, xinfo.root_win, &root, &parent, &child, &n))
        exit_with_msg(_("錯誤：查詢窗口清單失敗！"));
    for(size_t i=0; i<n; i++)
    {
        if(is_wm_win(wm, child[i], true))
            add_client(wm, child[i]);
        restack_win(wm, child[i]);
    }
    XFree(child);
}
