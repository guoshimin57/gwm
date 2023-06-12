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

static void apply_rules(WM *wm, Client *c);
static bool have_rule(const Rule *r, Client *c);
static void set_win_rect_by_attr(WM *wm, Client *c);
static void fix_win_pos(WM *wm, Client *c);
static bool fix_win_pos_by_hint(Client *c);
static void fix_win_pos_by_prop(WM *wm, Client *c);
static void fix_win_pos_by_workarea(WM *wm, Client *c);
static void fix_win_size(WM *wm, Client *c);
static void fix_win_size_by_workarea(WM *wm, Client *c);
static void frame_client(WM *wm, Client *c);
static Rect get_button_rect(WM *wm, Client *c, size_t index);
static Rect get_frame_rect(Client *c);
static bool move_client_node(WM *wm, Client *from, Client *to, Area_type type);

void add_client(WM *wm, Window win)
{
    Client *c=malloc_s(sizeof(Client));
    memset(c, 0, sizeof(Client));
    c->win=win;
    c->owner=get_transient_for(wm, win);
    c->title_text=get_text_prop(wm, win, XA_WM_NAME);
    if(!c->title_text)
        c->title_text=copy_string("?");
    c->wm_hint=XGetWMHints(wm->display, win);
    update_size_hint(wm, c);
    apply_rules(wm, c);
    add_client_node(get_area_head(wm, c->area_type), c);
    fix_area_type(wm);
    set_default_win_rect(wm, c);
    create_icon(wm, c);
    if(c->area_type == ICONIFY_AREA)
        iconify(wm, c);
    grab_buttons(wm, c);
    XSelectInput(wm->display, win, EnterWindowMask|PropertyChangeMask);
    XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
    frame_client(wm, c);
    update_layout(wm);
    XMapWindow(wm->display, c->frame);
    XMapSubwindows(wm->display, c->frame);
    if(c->area_type != ICONIFY_AREA)
        focus_client(wm, wm->cur_desktop, c);
    set_all_net_client_list(wm);
}

static void apply_rules(WM *wm, Client *c)
{
    Atom type=get_atom_prop(wm, c->win, wm->ewmh_atom[NET_WM_WINDOW_TYPE]),
         state=get_atom_prop(wm, c->win, wm->ewmh_atom[NET_WM_STATE]);

    Window tw=get_transient_for(wm, c->win);
    c->area_type=DESKTOP(wm)->default_area_type;
    if( (tw && tw!=wm->root_win)
        || type != wm->ewmh_atom[NET_WM_WINDOW_TYPE_NORMAL]
        || state == wm->ewmh_atom[NET_WM_STATE_MODAL])
        c->area_type=FLOATING_AREA;
    c->border_w=wm->cfg->border_width;
    c->titlebar_h=TITLEBAR_HEIGHT(wm);
    c->desktop_mask=get_desktop_mask(wm->cur_desktop);
    c->class_hint.res_class=c->class_hint.res_name=NULL, c->class_name="?";
    if(XGetClassHint(wm->display, c->win, &c->class_hint))
    {
        c->class_name=c->class_hint.res_class;
        for(const Rule *r=wm->cfg->rule; r->app_class; r++)
        {
            if(have_rule(r, c))
            {
                c->area_type=r->area_type;
                if(!r->show_border)
                    c->border_w=0;
                if(!r->show_titlebar)
                    c->titlebar_h=0;
                c->desktop_mask = r->desktop_mask ?
                    r->desktop_mask : get_desktop_mask(wm->cur_desktop);
                if(r->class_alias)
                    c->class_name=r->class_alias;
            }
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

void add_client_node(Client *head, Client *c)
{
    c->prev=head;
    c->next=head->next;
    head->next=c;
    c->next->prev=c;
}

void fix_area_type(WM *wm)
{
    int n=0, m=DESKTOP(wm)->n_main_max;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_on_cur_desktop(wm, c))
        {
            if(c->area_type==MAIN_AREA && ++n>m)
                c->area_type=SECOND_AREA;
            else if(c->area_type==SECOND_AREA && n<m)
                c->area_type=MAIN_AREA, n++;
        }
    }
}

void set_default_win_rect(WM *wm, Client *c)
{
    set_win_rect_by_attr(wm, c);
    fix_win_size(wm, c);
    fix_win_pos(wm, c);
}

static void set_win_rect_by_attr(WM *wm, Client *c)
{
    XWindowAttributes a={.x=wm->workarea.x, .y=wm->workarea.y,
        .width=wm->workarea.w/4, .height=wm->workarea.h/4};
    XGetWindowAttributes(wm->display, c->win, &a);
    c->x=a.x, c->y=a.y, c->w=a.width, c->h=a.height;
}

static void fix_win_pos(WM *wm, Client *c)
{
    if(!fix_win_pos_by_hint(c))
        fix_win_pos_by_prop(wm, c), fix_win_pos_by_workarea(wm, c);
}

static bool fix_win_pos_by_hint(Client *c)
{
    XSizeHints *p=&c->size_hint;
    if((p->flags & USPosition))
        c->x=p->x, c->y=p->y;
    return (p->flags & USPosition);
}

static void fix_win_pos_by_prop(WM *wm, Client *c)
{
    Client *oc;
    // 爲了避免有符號整數與無符號整數之間的運算帶來符號問題
    long ow, oh, w=c->w, h=c->h, wx=wm->workarea.x, wy=wm->workarea.y,
         ww=wm->workarea.w, wh=wm->workarea.h;
    if((oc=win_to_client(wm, get_transient_for(wm, c->win))))
        ow=oc->w, oh=oc->h, c->x=oc->x+(ow-w)/2, c->y=oc->y+(oh-h)/2;
    if( get_atom_prop(wm, c->win, wm->ewmh_atom[NET_WM_WINDOW_TYPE])
        == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DIALOG])
        c->x=wx+(ww-w)/2, c->y=wy+(wh-h)/2;
}

static void fix_win_pos_by_workarea(WM *wm, Client *c)
{
    // 爲了避免有符號整數與無符號整數之間的運算帶來符號問題
    long w=c->w, h=c->h, bw=c->border_w, bh=c->titlebar_h, wx=wm->workarea.x,
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
    c->frame=XCreateSimpleWindow(wm->display, wm->root_win, fr.x, fr.y, fr.w,
        fr.h, c->border_w, WIDGET_COLOR(wm, CURRENT_BORDER), 0);
    XSelectInput(wm->display, c->frame, FRAME_EVENT_MASK);
    if(wm->cfg->set_frame_prop)
        copy_prop(wm, c->frame, c->win);
    if(c->titlebar_h)
        create_titlebar(wm, c);
    XAddToSaveSet(wm->display, c->win);
    XReparentWindow(wm->display, c->win, c->frame, 0, c->titlebar_h);
}

void create_titlebar(WM *wm, Client *c)
{
    Rect tr=get_title_area_rect(wm, c);
    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=get_button_rect(wm, c, i);
        c->buttons[i]=XCreateSimpleWindow(wm->display, c->frame, br.x, br.y,
            br.w, br.h, 0, 0, WIDGET_COLOR(wm, CURRENT_TITLEBAR));
        XSelectInput(wm->display, c->buttons[i], BUTTON_EVENT_MASK);
    }
    c->title_area=XCreateSimpleWindow(wm->display, c->frame, tr.x, tr.y,
        tr.w, tr.h, 0, 0, WIDGET_COLOR(wm, CURRENT_TITLEBAR));
    XSelectInput(wm->display, c->title_area, TITLE_AREA_EVENT_MASK);
    c->logo=XCreateSimpleWindow(wm->display, c->frame, 0, 0,
        c->titlebar_h, c->titlebar_h, 0, 0, WIDGET_COLOR(wm, CURRENT_TITLEBAR));
    XSelectInput(wm->display, c->logo, ExposureMask);
}

static Rect get_frame_rect(Client *c)
{
    long bw=c->border_w, bh=c->titlebar_h;
    return (Rect){c->x-bw, c->y-bh-bw, c->w, c->h+bh};
}

Rect get_title_area_rect(WM *wm, Client *c)
{
    int buttons_n[]={[FULL]=0, [PREVIEW]=1, [STACK]=3, [TILE]=7},
        n=buttons_n[DESKTOP(wm)->cur_layout], size=TITLEBAR_HEIGHT(wm);
    return (Rect){size, 0, c->w-wm->cfg->title_button_width*n, size};
}

static Rect get_button_rect(WM *wm, Client *c, size_t index)
{
    int cw=c->w, w=wm->cfg->title_button_width, h=TITLEBAR_HEIGHT(wm);
    return (Rect){cw-w*(TITLE_BUTTON_N-index), 0, w, h};
}

int get_typed_clients_n(WM *wm, Area_type type)
{
    int n=0;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->area_type==type)
            n++;
    return n;
}

int get_clients_n(WM *wm)
{
    int n=0;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            n++;
    return n;
}

int get_all_clients_n(WM *wm)
{
    int n=0;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
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
    if(c)
    {
        if(is_win_exist(wm, c->win, c->frame))
            XReparentWindow(wm->display, c->win, wm->root_win, c->x, c->y);
        XDestroyWindow(wm->display, c->frame);
        del_icon(wm, c);
        del_client_node(c);
        fix_area_type(wm);
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
}

void del_client_node(Client *c)
{
    c->prev->next=c->next;
    c->next->prev=c->prev;
}

void move_resize_client(WM *wm, Client *c, const Delta_rect *d)
{
    if(d)
        c->x+=d->dx, c->y+=d->dy, c->w+=d->dw, c->h+=d->dh;
    Rect fr=get_frame_rect(c), tr=get_title_area_rect(wm, c);
    XMoveResizeWindow(wm->display, c->win, 0, c->titlebar_h, c->w, c->h);
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
}

void update_frame(WM *wm, unsigned int desktop_n, Client *c)
{
    bool cur=(c==wm->desktop[desktop_n-1]->cur_focus_client);
    if(c->border_w)
        XSetWindowBorder(wm->display, c->frame,
            NC_WIDGET_COLOR(wm, cur, BORDER));
    if(c->titlebar_h)
    {
        update_win_bg(wm, c->logo,
            NC_WIDGET_COLOR(wm, cur, TITLEBAR), None);
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
        update_win_bg(wm, c->title_area,
            NC_WIDGET_COLOR(wm, cur, TITLEBAR), None);
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            update_win_bg(wm, c->buttons[i],
                NC_WIDGET_COLOR(wm, cur, TITLEBAR), None);
    }
}

Client *win_to_iconic_state_client(WM *wm, Window win)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(c->area_type==ICONIFY_AREA && c->icon->win==win)
            return c;
    return NULL;
}

/* 僅在移動窗口、聚焦窗口時才有可能需要提升 */
void raise_client(WM *wm, unsigned int desktop_n)
{
    Client *c=wm->desktop[desktop_n-1]->cur_focus_client;
    if(c != wm->clients)
    {
        Window wins[]={wm->taskbar->win, c->frame};
        if(is_on_desktop_n(desktop_n, c) && c->area_type==FLOATING_AREA)
            XRaiseWindow(wm->display, c->frame);
        else
            XRestackWindows(wm->display, wins, ARRAY_NUM(wins));
        set_all_net_client_list(wm);
    }
}

/* 取得存儲結構意義上的下一個客戶窗口 */
Client *get_next_client(WM *wm, Client *c)
{
    for(Client *p=c->next; p!=wm->clients; p=p->next)
        if(is_on_cur_desktop(wm, p))
            return p;
    return NULL;
}

/* 取得存儲結構意義上的上一個客戶窗口 */
Client *get_prev_client(WM *wm, Client *c)
{
    for(Client *p=c->prev; p!=wm->clients; p=p->prev)
        if(is_on_cur_desktop(wm, p))
            return p;
    return NULL;
}

void move_client(WM *wm, Client *from, Client *to, Area_type type)
{
    if(move_client_node(wm, from, to, type))
    {
        if(from->area_type == ICONIFY_AREA)
            deiconify(wm, from);
        if(type == ICONIFY_AREA)
            iconify(wm, from);
        from->area_type=type;
        fix_area_type(wm);
        raise_client(wm, wm->cur_desktop);
        update_layout(wm);
    }
}

static bool move_client_node(WM *wm, Client *from, Client *to, Area_type type)
{
    Client *head;
    Area_type ft=from->area_type, tt=to->area_type;
    if( from==wm->clients || (from==to && tt==type) || (ft==MAIN_AREA
        && type==SECOND_AREA && !get_typed_clients_n(wm, SECOND_AREA)))
        return false;
    del_client_node(from);
    if(tt == type)
    {
        if((ft==MAIN_AREA && tt==SECOND_AREA)
            || (ft==tt && compare_client_order(wm, from, to)==-1))
            head=to;
        else
            head=to->prev;
    }
    else
    {
        if(ft==MAIN_AREA && type==SECOND_AREA)
            head=to->next;
        else if(from == to)
            head=to->prev;
        else
            head=to;
    }
    add_client_node(head, from);
    return true;
}

void swap_clients(WM *wm, Client *a, Client *b)
{
    if(a != b)
    {
        Client *aprev=a->prev, *bprev=b->prev;
        Area_type atype=a->area_type, btype=b->area_type;

        del_client_node(a);
        add_client_node(compare_client_order(wm, a, b)==-1 ? b : bprev, a);
        if(aprev!=b && bprev!=a) //不相邻
            del_client_node(b), add_client_node(aprev, b);

        a->area_type=btype;
        if(atype!=ICONIFY_AREA && a->area_type==ICONIFY_AREA)
            iconify(wm, a);
        else if(atype==ICONIFY_AREA && a->area_type!=ICONIFY_AREA)
            a->icon->area_type=btype, deiconify(wm, a);

        b->area_type=atype;
        if(btype!=ICONIFY_AREA && b->area_type==ICONIFY_AREA)
            iconify(wm, b);
        else if(btype==ICONIFY_AREA && b->area_type!=ICONIFY_AREA)
            b->icon->area_type=atype, deiconify(wm, b);

        if(atype==ICONIFY_AREA && btype==ICONIFY_AREA)
            update_icon_area(wm);

        raise_client(wm, wm->cur_desktop);
        update_layout(wm);
    }
}

int compare_client_order(WM *wm, Client *c1, Client *c2)
{
    if(c1 == c2)
        return 0;
    for(Client *c=c1; c!=wm->clients; c=c->next)
        if(c == c2)
            return -1;
    return 1;
}

bool is_last_typed_client(WM *wm, Client *c, Area_type type)
{
    for(Client *p=wm->clients->prev; p!=c; p=p->prev)
        if(p->area_type == type)
            return false;
    return true;
}

Client *get_area_head(WM *wm, Area_type type)
{
    Client *head=wm->clients;
    for(Client *c=head->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->area_type==type)
            return c->prev;
    for(Client *c=head->next; c!=wm->clients; c=c->next)
        if(c->area_type == type)
            return c->prev;
    for(Client *c=head->prev; c!=wm->clients; c=c->prev)
        if(c->area_type < type)
            head=c;
    return head;
}
