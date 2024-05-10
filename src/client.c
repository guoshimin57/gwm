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
static Client *get_head_for_add_client(Client *list, Client *c);
static void add_client_node(Client *list, Client *c);
static void set_default_place_type(Client *c);
static void set_default_win_rect(Client *c);
static void set_win_rect_by_attr(Client *c);
static void frame_client(Client *c);
static void unframe_client(Client *c);
static void del_client_node(Client *c);
static Window get_top_win(WM *wm, Client *c);
static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c);
static bool is_map_client(Client *list, unsigned int desktop_n, Client *c);
static Client *get_prev_map_client(Client *list, unsigned int desktop_n, Client *c);
static Client *get_next_map_client(Client *list, unsigned int desktop_n, Client *c);
static int cmp_map_order(const void *pclient1, const void *pclient2);

static long map_count=0; // 所有客戶窗口的累計映射次數

void add_client(WM *wm, Window win)
{
    Client *c=new_client(wm, win);

    apply_rules(c);
    add_client_node(get_head_for_add_client(wm->clients, c), c);
    grab_buttons(WIDGET_WIN(c));
    XSelectInput(xinfo.display, win, EnterWindowMask|PropertyChangeMask);
    set_cursor(win, NO_OP);
    set_default_win_rect(c);
    save_place_info_of_client(c);
    frame_client(c);
    request_layout_update();
    show_widget(WIDGET(c->frame));
    focus_client(wm, wm->cur_desktop, c);
    set_net_wm_allowed_actions(WIDGET_WIN(c));
}

void set_all_net_client_list(Client *list)
{
    int n=0;
    Window *wlist=get_client_win_list(list, &n);

    set_net_client_list(wlist, n);
    free_s(wlist);

    wlist=get_client_win_list_stacking(list, &n);
    set_net_client_list_stacking(wlist, n);
    free_s(wlist);
}

static Client *new_client(WM *wm, Window win)
{
    Client *c=malloc_s(sizeof(Client));
    memset(c, 0, sizeof(Client));
    Widget_state state={.current=1};
    init_widget(WIDGET(c), CLIENT_WIN, UNUSED_TYPE, state, xinfo.root_win, 0, 0, 1, 1);
    WIDGET_WIN(c)=win;
    c->map_n=++map_count;
    c->title_text=get_title_text(win, "");
    c->wm_hint=XGetWMHints(xinfo.display, win);
    c->win_type=get_net_wm_win_type(win);
    c->win_state=get_net_wm_state(win);
    c->owner=win_to_client(wm->clients, get_transient_for(WIDGET_WIN(c)));
    c->subgroup_leader=get_subgroup_leader(c);
    set_default_place_type(c);
    if(!should_hide_frame(c))
        c->border_w=cfg->border_width, c->titlebar_h=get_font_height_by_pad();
    c->class_name="?";
    XGetClassHint(xinfo.display, WIDGET_WIN(c), &c->class_hint);
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
        if(!get_net_wm_desktop(WIDGET_WIN(c), &desktop))
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

static Client *get_head_for_add_client(Client *list, Client *c)
{
    Client *top=NULL, *head=NULL;
    if(c->owner)
    {
        top=get_top_transient_client(c->subgroup_leader, false);
        head = top ? top->prev : c->owner->prev;
    }
    else
        head=get_head_client(list, c->place_type);
    return head;
}

static void add_client_node(Client *list, Client *c)
{
    c->prev=list;
    c->next=list->next;
    list->next=c;
    c->next->prev=c;
}

static void set_default_place_type(Client *c)
{
    if(c->owner)                     c->place_type = c->owner->place_type;
    else if(c->win_type.desktop)     c->place_type = DESKTOP_LAYER;
    else if(c->win_state.below)      c->place_type = BELOW_LAYER;
    else if(c->win_type.dock)        c->place_type = DOCK_LAYER;
    else if(c->win_state.above)      c->place_type = ABOVE_LAYER;
    else if(c->win_state.fullscreen) c->place_type = FULLSCREEN_LAYER;
    else if(is_win_state_max(c))     c->place_type = FLOAT_LAYER;
    else                             c->place_type = TILE_LAYER_MAIN;  
}

void set_win_rect_by_frame(Client *c, const Rect *frame)
{
    WIDGET_X(c)=frame->x+c->border_w;
    WIDGET_Y(c)=frame->y+c->titlebar_h+c->border_w;
    WIDGET_W(c)=frame->w-2*c->border_w;
    WIDGET_H(c)=frame->h-c->titlebar_h-2*c->border_w;
}

static void set_default_win_rect(Client *c)
{
    WIDGET_W(c)=xinfo.screen_width/4;
    WIDGET_H(c)=xinfo.screen_height/4;
    WIDGET_X(c)=(xinfo.screen_width-WIDGET_W(c))/2;
    WIDGET_Y(c)=(xinfo.screen_height-WIDGET_H(c))/2;
    set_win_rect_by_attr(c);
}


static void set_win_rect_by_attr(Client *c)
{
    XWindowAttributes a;
    if(XGetWindowAttributes(xinfo.display, WIDGET_WIN(c), &a))
        WIDGET_X(c)=a.x, WIDGET_Y(c)=a.y, WIDGET_W(c)=a.width, WIDGET_H(c)=a.height;
}

static void frame_client(Client *c)
{
    Rect fr=get_frame_rect(c);
    c->frame=malloc_s(sizeof(Frame));
    init_widget(WIDGET(c->frame), CLIENT_FRAME, UNUSED_TYPE, WIDGET_STATE(c),
        xinfo.root_win, fr.x, fr.y, fr.w, fr.h);
    set_widget_border_width(WIDGET(c->frame), cfg->border_width);
    set_widget_border_color(WIDGET(c->frame),
        get_widget_color(WIDGET_STATE(c->frame)));
    XSelectInput(xinfo.display, c->frame->base.win, FRAME_EVENT_MASK);
    if(cfg->set_frame_prop)
        copy_prop(c->frame->base.win, WIDGET_WIN(c));
    if(c->titlebar_h)
        create_titlebar(c);
    XAddToSaveSet(xinfo.display, WIDGET_WIN(c));
    XReparentWindow(xinfo.display, WIDGET_WIN(c), c->frame->base.win, 0, c->titlebar_h);
    show_widget(WIDGET(c->frame));
    
    /* 以下是同時設置窗口前景和背景透明度的非EWMH標準方法：
    unsigned long opacity = (unsigned long)(0xfffffffful);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(xinfo.display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(xinfo.display, c->frame, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&opacity, 1L);
    */
}

static void unframe_client(Client *c)
{
    if(c->titlebar_h)
    {
        destroy_button(c->frame->logo);
        destroy_widget(c->frame->title_area);
        for(int i=0; i<TITLE_BUTTON_N; i++)
            destroy_button(c->frame->buttons[i]);
    }
    destroy_widget(WIDGET(c->frame));
    XReparentWindow(xinfo.display, WIDGET_WIN(c), xinfo.root_win, WIDGET_X(c), WIDGET_Y(c));
}

void create_titlebar(Client *c)
{
    Rect tr=get_title_area_rect(c);

    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=get_button_rect(c, i);
        c->frame->buttons[i]=create_button(TITLE_BUTTON_BEGIN+i, WIDGET_STATE(c),
            WIDGET_WIN(c->frame), br.x, br.y, br.w, br.h, cfg->title_button_text[i]);
        set_widget_tooltip(WIDGET(c->frame->buttons[i]), cfg->tooltip[TITLE_BUTTON_BEGIN+i]);
    }

    c->frame->title_area=create_widget(TITLE_AREA, UNUSED_TYPE, WIDGET_STATE(c),
        WIDGET_WIN(c->frame), tr.x, tr.y, tr.w, tr.w);
    set_widget_tooltip(WIDGET(c->frame->title_area), c->title_text);
    c->frame->logo=create_button(TITLE_LOGO, WIDGET_STATE(c), WIDGET_WIN(c->frame),
        0, 0, c->titlebar_h, c->titlebar_h, NULL);
    set_widget_tooltip(WIDGET(c->frame->logo), cfg->tooltip[TITLE_LOGO]);
    Imlib_Image image=get_icon_image(WIDGET_WIN(c), c->class_hint.res_name,
        cfg->icon_image_size, cfg->cur_icon_theme);
    set_button_icon(BUTTON(c->frame->logo), image, c->class_hint.res_name, NULL);
}

Rect get_frame_rect(Client *c)
{
    long bw=c->border_w, bh=c->titlebar_h;
    return (Rect){WIDGET_X(c)-bw, WIDGET_Y(c)-bh-bw, WIDGET_W(c), WIDGET_H(c)+bh};
}

Rect get_title_area_rect(Client *c)
{
    int layout=cfg->default_layout;
    get_gwm_current_layout(&layout);
    int buttons_n[]={[PREVIEW]=1, [STACK]=3, [TILE]=7},
        n=buttons_n[layout], size=get_font_height_by_pad();
    return (Rect){size, 0, WIDGET_W(c)-cfg->title_button_width*n-size, size};
}

Rect get_button_rect(Client *c, size_t index)
{
    int cw=WIDGET_W(c), w=cfg->title_button_width, h=get_font_height_by_pad();
    return (Rect){cw-w*(TITLE_BUTTON_N-index), 0, w, h};
}

int get_clients_n(Client *list, Place_type type, bool count_icon, bool count_trans, bool count_all_desktop)
{
    int n=0;
    for(Client *c=list->next; c!=list; c=c->next)
        if( (type==ANY_PLACE || c->place_type==type)
            && (count_icon || !is_iconic_client(c))
            && (count_trans || !c->owner)
            && (count_all_desktop || is_on_cur_desktop(c->desktop_mask)))
            n++;
    return n;
}

bool is_iconic_client(Client *c)
{
    return WIDGET_WIN(c)!=xinfo.root_win && c->win_state.hidden;
}

Client *win_to_client(Client *list, Window win)
{
    // 當隱藏標題欄時，標題區和按鈕的窗口ID爲0。故win爲0時，不應視爲找到
    for(Client *c=list->next; win && c!=list; c=c->next)
    {
        if(win==WIDGET_WIN(c) || win==c->frame->base.win)
            return c;
        if(c->titlebar_h)
        {
            if(win==WIDGET_WIN(c->frame->logo) || win==WIDGET_WIN(c->frame->title_area))
                return c;
            for(size_t i=0; i<TITLE_BUTTON_N; i++)
                if(win == WIDGET_WIN(c->frame->buttons[i]))
                    return c;
        }
    }
        
    return NULL;
}

void del_client(WM *wm, Client *c, bool is_for_quit)
{
    if(!c)
        return;

    unframe_client(c);
    if(c->image)
        imlib_context_set_image(c->image), imlib_free_image();
    del_client_node(c);
    if(!is_for_quit)
        for(size_t i=1; i<=DESKTOP_N; i++)
            if(is_on_desktop_n(i, c->desktop_mask))
                focus_client(wm, i, NULL);

    XFree(c->class_hint.res_class);
    XFree(c->class_hint.res_name);
    XFree(c->wm_hint);
    vfree(c->title_text, c, NULL);

    if(!is_for_quit)
        request_layout_update();
    set_all_net_client_list(wm->clients);
}

static void del_client_node(Client *c)
{
    c->prev->next=c->next;
    c->next->prev=c->prev;
}

/* 僅在移動窗口、聚焦窗口時或窗口類型、狀態發生變化才有可能需要提升 */
void raise_client(WM *wm, Client *c)
{
    int n=get_subgroup_n(c), i=n;
    Window wins[n+1];

    wins[0]=get_top_win(wm, c);
    for(Client *ld=c->subgroup_leader, *p=ld; ld && p->subgroup_leader==ld; p=p->prev)
        wins[i--]=WIDGET_WIN(p->frame);

    XRestackWindows(xinfo.display, wins, n+1);
    set_all_net_client_list(wm->clients);
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

/* 當WIDGET_WIN(c)所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_next_client(Client *list, Client *c)
{
    for(Client *m=NULL, *p=c->next; p!=list; p=p->next)
    {
        if(is_on_cur_desktop(p->desktop_mask))
        {
            m=get_top_transient_client(p->subgroup_leader, true);
            if(!m || p==m)
                return p;
            else
                return p->subgroup_leader->next;
        }
    }
    return list;
}

/* 當WIDGET_WIN(c)所在的亞組存在模態窗口時，跳過非模態窗口 */
Client *get_prev_client(Client *list, Client *c)
{
    for(Client *m=NULL, *p=c->prev; p!=c; p=p->prev)
        if(is_on_cur_desktop(p->desktop_mask))
            return (m=get_top_transient_client(p->subgroup_leader, true)) ? m : p;
    return list;
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

bool is_last_typed_client(Client *list, Client *c, Place_type type)
{
    for(Client *p=list->prev; p!=c; p=p->prev)
        if(is_on_cur_desktop(p->desktop_mask) && p->place_type==type)
            return false;
    return true;
}

Client *get_head_client(Client *list, Place_type type)
{
    for(Client *c=list->next; c!=list; c=c->next)
        if(is_on_cur_desktop(c->desktop_mask) && c->place_type==type)
            return c->prev;
    for(Client *c=list->next; c!=list; c=c->next)
        if(c->place_type == type)
            return c->prev;
    for(Client *c=list->prev; c!=list; c=c->prev)
        if(c->place_type < type)
            return c->prev;
    return list;
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

    if(pc!=wm->clients && pc->titlebar_h)
    {
        WIDGET_STATE(pc).current=WIDGET_STATE(pc->frame->logo).current=WIDGET_STATE(pc->frame->title_area).current=1;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            WIDGET_STATE(pc->frame->buttons[i]).current=1;
    }
    if(pp!=wm->clients && pp!=pc && pp->titlebar_h)
    {
        WIDGET_STATE(pp).current=WIDGET_STATE(pp->frame->logo).current=WIDGET_STATE(pp->frame->title_area).current=0;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            WIDGET_STATE(pp->frame->buttons[i]).current=0;
    }
    if(desktop_n == wm->cur_desktop)
    {
        if(WIDGET_WIN(pc) == xinfo.root_win)
            XSetInputFocus(xinfo.display, xinfo.root_win, RevertToPointerRoot, CurrentTime);
        else if(!is_iconic_client(pc))
            set_input_focus(WIDGET_WIN(pc), pc->wm_hint);
    }

    if(pc != wm->clients)
        WIDGET_STATE(pc).current=WIDGET_STATE(pc->frame).current=1;
    update_client_bg(wm, desktop_n, pc);

    if(pp != wm->clients)
        WIDGET_STATE(pp).current=WIDGET_STATE(pp->frame).current=0;
    update_client_bg(wm, desktop_n, pp);

    raise_client(wm, pc);
    set_net_active_window(WIDGET_WIN(pc));
}

static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c)
{
    Desktop *d=wm->desktop[desktop_n-1];
    Client *p=NULL, **pp=&d->prev_focus_client, **pc=&d->cur_focus_client;

    if(!c)  // 當某個client在desktop_n中變得不可見時，即既有可能被刪除了，
    {       // 也可能是被縮微化了，還有可能是移動到其他虛擬桌面了。
        if(!is_map_client(wm->clients, desktop_n, *pc)) // 非當前窗口被非wm手段關閉（如kill）
        {
            p = (*pc)->owner ? (*pc)->owner : *pp;
            if(is_map_client(wm->clients, desktop_n, p))
                *pc=p;
            else if((p=get_prev_map_client(wm->clients, desktop_n, *pp)))
                *pc=p;
            else if((p=get_next_map_client(wm->clients, desktop_n, *pp)))
                *pc=p;
            else
                *pc=wm->clients;
        }

        if(!is_map_client(wm->clients, desktop_n, *pp))
        {
            if(is_map_client(wm->clients, desktop_n, (*pp)->owner))
                *pp=(*pp)->owner;
            else if((p=get_prev_map_client(wm->clients, desktop_n, *pp)))
                *pp=p;
            else if((p=get_next_map_client(wm->clients, desktop_n, *pp)))
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

static bool is_map_client(Client *list, unsigned int desktop_n, Client *c)
{
    if(c && !is_iconic_client(c) && is_on_desktop_n(desktop_n, c->desktop_mask))
        for(Client *p=list->next; p!=list; p=p->next)
            if(p == c)
                return true;
    return false;
}

/* 取得存儲結構意義上的上一個處於映射狀態的客戶窗口 */
static Client *get_prev_map_client(Client *list, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->prev; p!=list; p=p->prev)
        if(!is_iconic_client(p) && is_on_desktop_n(desktop_n, p->desktop_mask))
            return p;
    return NULL;
}

/* 取得存儲結構意義上的下一個處於映射狀態的客戶窗口 */
static Client *get_next_map_client(Client *list, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->next; p!=list; p=p->next)
        if(!is_iconic_client(p) && is_on_desktop_n(desktop_n, p->desktop_mask))
            return p;
    return NULL;
}

void save_place_info_of_client(Client *c)
{
    c->ox=WIDGET_X(c), c->oy=WIDGET_Y(c), c->ow=WIDGET_W(c), c->oh=WIDGET_H(c);
    c->old_place_type=c->place_type;
}

void save_place_info_of_clients(Client *list)
{
    for(Client *c=list->prev; c!=list; c=c->prev)
        if(is_on_cur_desktop(c->desktop_mask))
            save_place_info_of_client(c);
}

void restore_place_info_of_client(Client *c)
{
    WIDGET_X(c)=c->ox, WIDGET_Y(c)=c->oy, WIDGET_W(c)=c->ow, WIDGET_H(c)=c->oh;
    c->place_type=c->old_place_type;
}

void restore_place_info_of_clients(Client *list)
{
    for(Client *c=list->next; c!=list; c=c->next)
        if(is_on_cur_desktop(c->desktop_mask))
            restore_place_info_of_client(c);
}

bool is_tile_client(Client *c)
{
    return is_on_cur_desktop(c->desktop_mask) && !c->owner && !is_iconic_client(c)
        && is_normal_layer(c->place_type);
}

/* 獲取當前桌面按從早到遲的映射順序排列的客戶窗口列表 */
Window *get_client_win_list(Client *list, int *n)
{
    int i=0, count=get_clients_n(list, ANY_PLACE, true, true, false);

    if(count == 0)
        return NULL;

    if(n)
        *n=count;

    Window *wlist=malloc_s(count*sizeof(Window));
    Client **clist=malloc_s(count*sizeof(Client *));

    for(Client *c=list->prev; c!=list; c=c->prev)
        if(is_on_cur_desktop(c->desktop_mask))
            clist[i++]=c;
    qsort(clist, *n, sizeof(Client *), cmp_map_order);

    for(i=0; i<count; i++)
        wlist[i]=WIDGET_WIN(clist[i]);
    free_s(clist);

    return wlist;
}

static int cmp_map_order(const void *pclient1, const void *pclient2)
{
    return (*(Client **)pclient1)->map_n - (*(Client **)pclient2)->map_n;
}

/* 獲取當前桌面按從下到上的疊次序排列的客戶窗口列表 */
Window *get_client_win_list_stacking(Client *list, int *n)
{
    int count=get_clients_n(list, ANY_PLACE, true, true, false);
    if(count == 0)
        return NULL;

    if(n)
        *n=count;

    Window *wlist=malloc_s(count*sizeof(Window)), *w=wlist;
    for(Client *c=list->prev; c!=list; c=c->prev, w++)
        if(is_on_cur_desktop(c->desktop_mask))
            *w=WIDGET_WIN(c);

    return wlist;
}

void set_state_attent(Client *c, bool attent)
{
    if(c->win_state.attent == attent) // 避免重復設置
        return;

    c->win_state.attent=attent;
    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

bool is_wm_win(Client *list, Window win, bool before_wm)
{
    XWindowAttributes a;
    bool status=XGetWindowAttributes(xinfo.display, win, &a);

    if( !status || a.override_redirect
        || !is_on_screen( a.x, a.y, a.width, a.height))
        return false;

    if(!before_wm)
        return !win_to_client(list, win);

    return is_iconic_state(win) || a.map_state==IsViewable;
}

void restack_win(WM *wm, Window win)
{
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        if(win==wm->top_wins[i])
            return;

    Client *c=win_to_client(wm->clients, win);
    Net_wm_win_type type=get_net_wm_win_type(win);
    Window wins[2]={None, c ? WIDGET_WIN(c->frame) : win};

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
    if(is_iconic_client(c) && d->cur_layout!=PREVIEW)
        c->win_state.focused=1, update_net_wm_state(WIDGET_WIN(c), c->win_state);
    else
        update_frame_bg(c);
}

void update_frame_bg(Client *c)
{
    if(c->border_w)
        set_widget_border_color(WIDGET(c->frame),
            get_widget_color(WIDGET_STATE(c->frame)));
    if(c->titlebar_h)
    {
        update_widget_bg(WIDGET(c->frame->logo));
        update_widget_bg(WIDGET(c->frame->title_area));
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            update_widget_bg(WIDGET(c->frame->buttons[i]));
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
    WIDGET_WIN(wm->clients)=xinfo.root_win;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(!XQueryTree(xinfo.display, xinfo.root_win, &root, &parent, &child, &n))
        exit_with_msg(_("錯誤：查詢窗口清單失敗！"));
    for(size_t i=0; i<n; i++)
    {
        if(is_wm_win(wm->clients, child[i], true))
            add_client(wm, child[i]);
        restack_win(wm, child[i]);
    }
    XFree(child);
}
