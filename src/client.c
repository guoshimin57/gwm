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
#include "config.h"
#include "client.h"
#include "drawable.h"
#include "desktop.h"
#include "font.h"
#include "grab.h"
#include "hint.h"
#include "icon.h"
#include "layout.h"
#include "misc.h"

static void apply_rules(WM *wm, Client *c);
static bool have_rule(Rule *r, Client *c);
void set_win_rect_by_attr(WM *wm, Client *c);
static void fix_win_pos(WM *wm, Client *c);
static bool fix_win_pos_by_hint(Client *c);
static void fix_win_pos_by_prop(WM *wm, Client *c);
static void fix_win_pos_by_screen(WM *wm, Client *c);
static void fix_win_size(WM *wm, Client *c);
static void fix_win_size_by_screen(WM *wm, Client *c);
static void frame_client(WM *wm, Client *c);
static Rect get_button_rect(Client *c, size_t index);
static Rect get_frame_rect(Client *c);
static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c);
static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c);
static void update_client_look(WM *wm, unsigned int desktop_n, Client *c);
static bool move_client_node(WM *wm, Client *from, Client *to, Area_type type);

void add_client(WM *wm, Window win)
{
    Client *c=malloc_s(sizeof(Client));
    memset(c, 0, sizeof(Client));
    c->win=win;
    c->owner=get_transient_for(wm, win);
    c->title_text=get_text_prop(wm, win, XA_WM_NAME);
    c->wm_hint=XGetWMHints(wm->display, win);
    update_size_hint(wm, c);
    apply_rules(wm, c);
    add_client_node(get_area_head(wm, c->area_type), c);
    fix_area_type(wm);
    set_default_win_rect(wm, c);
    frame_client(wm, c);
    if(c->area_type == ICONIFY_AREA)
        iconify(wm, c);
    else
        focus_client(wm, wm->cur_desktop, c);
    grab_buttons(wm, c, BUTTONBIND, ARRAY_NUM(BUTTONBIND));
    XSelectInput(wm->display, win, EnterWindowMask|PropertyChangeMask);
    XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
}

static void apply_rules(WM *wm, Client *c)
{
    Atom type=get_atom_prop(wm, c->win, wm->ewmh_atom[_NET_WM_WINDOW_TYPE]),
         state=get_atom_prop(wm, c->win, wm->ewmh_atom[_NET_WM_STATE]);

    Window tw=get_transient_for(wm, c->win);
    c->area_type=DESKTOP(wm).default_area_type;
    if( (tw && tw!=wm->root_win)
        || type != wm->ewmh_atom[_NET_WM_WINDOW_TYPE_NORMAL]
        || state == wm->ewmh_atom[_NET_WM_STATE_MODAL])
        c->area_type=FLOATING_AREA;
    c->border_w=BORDER_WIDTH;
    c->title_bar_h=TITLE_BAR_HEIGHT;
    c->desktop_mask=get_desktop_mask(wm->cur_desktop);
    c->class_hint.res_class=c->class_hint.res_name=NULL, c->class_name="?";
    if(XGetClassHint(wm->display, c->win, &c->class_hint))
    {
        Rule *r=RULE;
        c->class_name=c->class_hint.res_class;
        for(size_t i=0; i<ARRAY_NUM(RULE); i++, r++)
        {
            if(have_rule(r, c))
            {
                c->area_type=r->area_type;
                c->border_w=r->border_w;
                c->title_bar_h=r->title_bar_h;
                c->desktop_mask = r->desktop_mask ?
                    r->desktop_mask : get_desktop_mask(wm->cur_desktop);
                if(r->class_alias)
                    c->class_name=r->class_alias;
            }
        }
    }
}

static bool have_rule(Rule *r, Client *c)
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
    int n=0, m=DESKTOP(wm).n_main_max;
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

void set_win_rect_by_attr(WM *wm, Client *c)
{
    XWindowAttributes a={0, 0, wm->screen_width/4, wm->screen_height/4};
    XGetWindowAttributes(wm->display, c->win, &a);
    c->x=a.x, c->y=a.y, c->w=a.width, c->h=a.height;
}

static void fix_win_pos(WM *wm, Client *c)
{
    if(!fix_win_pos_by_hint(c))
        fix_win_pos_by_prop(wm, c), fix_win_pos_by_screen(wm, c);
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
    int w=c->w, h=c->h, bh=c->title_bar_h, sw=wm->screen_width, sh=wm->screen_height;
    if((oc=win_to_client(wm, get_transient_for(wm, c->win))))
        c->x=oc->x+(oc->w-w)/2, c->y=oc->y+(oc->h-h)/2;
    if( get_atom_prop(wm, c->win, wm->ewmh_atom[_NET_WM_WINDOW_TYPE])
        == wm->ewmh_atom[_NET_WM_WINDOW_TYPE_DIALOG])
        c->x=(sw-w)/2, c->y=(sh-h-bh)/2+bh;
}

static void fix_win_pos_by_screen(WM *wm, Client *c)
{
    // 爲了避免有符號整數與無符號整數之間的運算帶來符號問題
    int w=c->w, h=c->h, bw=c->border_w, bh=c->title_bar_h,
        sw=wm->screen_width, sh=wm->screen_height, th=wm->taskbar.h;
    if(c->x >= sw-w-bw)
        c->x=sw-w-bw;
    if(c->x < bw)
        c->x=bw;
    if(c->y >= sh-th-bw-h)
        c->y=sh-th-bw-h;
    if(c->y < bw+bh)
        c->y=bw+bh;
}

static void fix_win_size(WM *wm, Client *c)
{
    fix_win_size_by_hint(c);
    fix_win_size_by_screen(wm, c);
}

static void fix_win_size_by_screen(WM *wm, Client *c)
{
    unsigned int sw=wm->screen_width, sh=wm->screen_height, th=wm->taskbar.h;
    if(c->w+2*c->border_w > sw)
        c->w=sw-2*c->border_w;
    if(c->h+c->title_bar_h+2*c->border_w > sh-th)
        c->h=sh-th-c->title_bar_h-2*c->border_w;
}

static void frame_client(WM *wm, Client *c)
{
    Rect fr=get_frame_rect(c);
    c->frame=XCreateSimpleWindow(wm->display, wm->root_win, fr.x, fr.y, fr.w,
        fr.h, c->border_w, wm->widget_color[CURRENT_BORDER_COLOR].pixel, 0);
    XSelectInput(wm->display, c->frame, FRAME_EVENT_MASK);
#if SET_FRAME_PROP
    copy_prop(wm, c->frame, c->win);
#endif
    if(c->title_bar_h)
        create_title_bar(wm, c);
    XAddToSaveSet(wm->display, c->win);
    XReparentWindow(wm->display, c->win, c->frame, 0, c->title_bar_h);
    XMapWindow(wm->display, c->frame);
    XMapSubwindows(wm->display, c->frame);
}

void create_title_bar(WM *wm, Client *c)
{
    unsigned long bc=wm->widget_color[CURRENT_TITLE_BUTTON_COLOR].pixel,
                  ac=wm->widget_color[CURRENT_TITLE_AREA_COLOR].pixel;
    Rect tr=get_title_area_rect(wm, c);
    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=get_button_rect(c, i);
        c->buttons[i]=XCreateSimpleWindow(wm->display, c->frame,
            br.x, br.y, br.w, br.h, 0, 0, bc);
        XSelectInput(wm->display, c->buttons[i], BUTTON_EVENT_MASK);
    }
    c->title_area=XCreateSimpleWindow(wm->display, c->frame,
        tr.x, tr.y, tr.w, tr.h, 0, 0, ac);
    XSelectInput(wm->display, c->title_area, TITLE_AREA_EVENT_MASK);
}

static Rect get_frame_rect(Client *c)
{
    return (Rect){c->x-c->border_w, c->y-c->title_bar_h-c->border_w,
        c->w, c->h+c->title_bar_h};
}

Rect get_title_area_rect(WM *wm, Client *c)
{
    int buttons_n[]={[FULL]=0, [PREVIEW]=1, [STACK]=3, [TILE]=7},
        n=buttons_n[DESKTOP(wm).cur_layout];
    return (Rect){0, 0, c->w-TITLE_BUTTON_WIDTH*n, c->title_bar_h};
}

static Rect get_button_rect(Client *c, size_t index)
{
    return (Rect){c->w-TITLE_BUTTON_WIDTH*(TITLE_BUTTON_N-index),
        (c->title_bar_h-TITLE_BUTTON_HEIGHT)/2,
        TITLE_BUTTON_WIDTH, TITLE_BUTTON_HEIGHT};
}

unsigned int get_typed_clients_n(WM *wm, Area_type type)
{
    unsigned int n=0;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->area_type==type)
            n++;
    return n;
}

Client *win_to_client(WM *wm, Window win)
{
    // 當隱藏標題欄時，標題區和按鈕的窗口ID爲0。故win爲0時，不應視爲找到
    for(Client *c=wm->clients->next; win && c!=wm->clients; c=c->next)
    {
        if(win==c->win || win==c->frame || win==c->title_area)
            return c;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            if(win == c->buttons[i])
                return c;
    }
        
    return NULL;
}

void del_client(WM *wm, Client *c, bool change_focus)
{
    if(c)
    {
        if(is_win_exist(wm, c->win, c->frame))
            XReparentWindow(wm->display, c->win, wm->root_win, c->x, c->y);
        XDestroyWindow(wm->display, c->frame);
        if(c->area_type == ICONIFY_AREA)
            del_icon(wm, c);
        del_client_node(c);
        fix_area_type(wm);
        if(change_focus)
            for(size_t i=1; i<=DESKTOP_N; i++)
                if(is_on_desktop_n(i, c))
                    focus_client(wm, i, NULL);
#if USE_IMAGE_ICON
        if(c->image)
        {
            imlib_context_set_image(c->image);
            imlib_free_image();
        }
#endif
        XFree(c->class_hint.res_class);
        XFree(c->class_hint.res_name);
        XFree(c->wm_hint);
        free(c->title_text);
        free(c);
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
    XMoveResizeWindow(wm->display, c->win,
        0, c->title_bar_h, c->w, c->h);
    if(c->title_bar_h)
    {
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
        {
            Rect br=get_button_rect(c, i);
            XMoveWindow(wm->display, c->buttons[i], br.x, br.y);
        }
        XResizeWindow(wm->display, c->title_area, tr.w, tr.h);
    }
    XMoveResizeWindow(wm->display, c->frame, fr.x, fr.y, fr.w, fr.h);
}

void update_frame(WM *wm, unsigned int desktop_n, Client *c)
{
    bool flag=(c==wm->desktop[desktop_n-1].cur_focus_client);
    if(c->border_w)
        XSetWindowBorder(wm->display, c->frame, flag ?
            wm->widget_color[CURRENT_BORDER_COLOR].pixel :
            wm->widget_color[NORMAL_BORDER_COLOR].pixel);
    if(c->title_bar_h)
    {
        update_win_background(wm, c->title_area, flag ?
            wm->widget_color[CURRENT_TITLE_AREA_COLOR].pixel :
            wm->widget_color[NORMAL_TITLE_AREA_COLOR].pixel, None);
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            update_win_background(wm, c->buttons[i], flag ?
                wm->widget_color[CURRENT_TITLE_BUTTON_COLOR].pixel :
                wm->widget_color[NORMAL_TITLE_BUTTON_COLOR].pixel, None);
    }
}

Client *win_to_iconic_state_client(WM *wm, Window win)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(c->area_type==ICONIFY_AREA && c->icon->win==win)
            return c;
    return NULL;
}

/* 若在調用本函數之前cur_focus_client或prev_focus_client因某些原因
 * （如移動到其他虛擬桌面）而未更新時，則應使用值爲NULL的c來調用本
 * 函數。這樣會自動推斷出合適的規則來取消原聚焦和聚焦新的client。*/
void focus_client(WM *wm, unsigned int desktop_n, Client *c)
{
    update_focus_client_pointer(wm, desktop_n, c);

    Desktop *d=wm->desktop+desktop_n-1;
    Client *pc=d->cur_focus_client;

    if(desktop_n == wm->cur_desktop)
    {
        if(pc->win == wm->root_win)
            XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
        else if(pc->area_type != ICONIFY_AREA)
            set_input_focus(wm, pc->wm_hint, pc->win);
    }
    update_client_look(wm, desktop_n, pc);
    update_client_look(wm, desktop_n, d->prev_focus_client);
    if(pc->area_type!=ICONIFY_AREA || d->cur_layout==PREVIEW)
        raise_client(wm, desktop_n);
}

static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c)
{
    Desktop *d=wm->desktop+desktop_n-1;
    Client *p=NULL, **pp=&d->prev_focus_client, **pc=&d->cur_focus_client;
 
    if(!c)  // 當某個client在desktop_n中變得不可見時，即既有可能被刪除了，
    {       // 也可能是被縮微化了，還有可能是移動到其他虛擬桌面了。
        if(!is_map_client(wm, desktop_n, *pc))
            p=win_to_client(wm, (*pc)->owner);
        if(!is_map_client(wm, desktop_n, p))
            p=*pp;
        if(!is_map_client(wm, desktop_n, p))
            p=get_prev_map_client(wm, desktop_n, *pp);
        if(!p)
            p=get_next_map_client(wm, desktop_n, *pp);
        *pc = p ? p : wm->clients;
        if(!is_map_client(wm, desktop_n, *pp) || *pp==*pc)
            p=win_to_client(wm, (*pp)->owner);
        if(!is_map_client(wm, desktop_n, p))
            p=get_prev_map_client(wm, desktop_n, *pp);
        if(!p)
            p=get_next_map_client(wm, desktop_n, *pp);
        *pp = p ? p : wm->clients;
    }
    else if(c != *pc)
        *pp=*pc, *pc=c;
}

static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    if(c && c->area_type!=ICONIFY_AREA && is_on_desktop_n(desktop_n, c))
        for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
            if(p == c)
                return true;
    return false;
}

/* 取得存儲結構意義上的下一個處於映射狀態的客戶窗口 */
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->next; p!=wm->clients; p=p->next)
        if(p->area_type!=ICONIFY_AREA && is_on_desktop_n(desktop_n, p))
            return p;
    return NULL;
}

/* 取得存儲結構意義上的上一個處於映射狀態的客戶窗口 */
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->prev; p!=wm->clients; p=p->prev)
        if(p->area_type!=ICONIFY_AREA && is_on_desktop_n(desktop_n, p))
            return p;
    return NULL;
}

static void update_client_look(WM *wm, unsigned int desktop_n, Client *c)
{
    if(c && c!=wm->clients)
    {
        Desktop *d=wm->desktop+desktop_n-1;
        if(c->area_type==ICONIFY_AREA && d->cur_layout!=PREVIEW)
            update_win_background(wm, c->icon->win, c==d->cur_focus_client ?
                wm->widget_color[ENTERED_NORMAL_BUTTON_COLOR].pixel :
                wm->widget_color[ICON_AREA_COLOR].pixel, None);
        else
            update_frame(wm, desktop_n,  c);
    }
}

/* 僅在移動窗口、聚焦窗口時才有可能需要提升 */
void raise_client(WM *wm, unsigned int desktop_n)
{
    Client *c=wm->desktop[desktop_n-1].cur_focus_client;
    if(c != wm->clients)
    {
        Window wins[]={wm->taskbar.win, c->frame};
        if(is_on_desktop_n(desktop_n, c) && c->area_type==FLOATING_AREA)
            XRaiseWindow(wm->display, c->frame);
        else
            XRestackWindows(wm->display, wins, ARRAY_NUM(wins));
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
