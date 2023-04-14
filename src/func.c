/* *************************************************************************
 *     func.c：實現按鍵和按鍵所要綁定的功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static bool is_match_button_release(WM *wm, XEvent *oe, XEvent *ne);
static bool is_valid_click(WM *wm, XEvent *oe, XEvent *ne);
static Delta_rect get_key_delta_rect(Client *c, Direction dir);
static void do_valid_pointer_move_resize(WM *wm, Client *c, Move_info *m, Pointer_act act);
static bool fix_move_resize_delta_rect(WM *wm, Client *c, Delta_rect *d, bool is_move);
static bool is_prefer_move(WM *wm, Client *c, Delta_rect *d, bool is_move);
static bool fix_delta_rect_for_nonprefer_size(Client *c, XSizeHints *hint, Delta_rect *d);
static void fix_dw_by_width_hint(int w, XSizeHints *hint, int *dw);
static void fix_dh_by_height_hint(int h, XSizeHints *hint, int *dh);
static bool fix_delta_rect_for_prefer_size(Client *c, XSizeHints *hint, int dw, int dh, Delta_rect *d);
static void update_hint_win_for_resize(WM *wm, Client *c);
static Delta_rect get_pointer_delta_rect(const Move_info *m, Pointer_act act);

bool is_drag_func(void (*func)(WM *, XEvent *, Func_arg))
{
    return func == pointer_swap_clients
        || func == pointer_move_resize_client
        || func == pointer_change_area
        || func == adjust_layout_ratio;
}

static bool is_grab_root_act(Pointer_act act)
{
    return act==SWAP || act==CHANGE;
}

bool get_valid_click(WM *wm, Pointer_act act, XEvent *oe, XEvent *ne)
{
    if(act==CHOOSE && get_widget_type(wm, oe->xbutton.window)==CLIENT_WIN)
        return true;

    Window win = is_grab_root_act(act) ? wm->root_win : oe->xbutton.window;
    if(act==NO_OP || grab_pointer(wm, win, act))
    {
        XEvent e, *p=(ne ? ne : &e);
        do
        {
            XMaskEvent(wm->display, ROOT_EVENT_MASK|POINTER_MASK, p);
            wm->event_handlers[p->type](wm, p);
        }while(!is_match_button_release(wm, oe, p));
        if(act != NO_OP)
            XUngrabPointer(wm->display, CurrentTime);
        return is_valid_click(wm, oe, p);
    }
    return false;
}

static bool is_match_button_release(WM *wm, XEvent *oe, XEvent *ne)
{
    return (ne->type==ButtonRelease && ne->xbutton.button==oe->xbutton.button);
}

static bool is_valid_click(WM *wm, XEvent *oe, XEvent *ne)
{
    return is_equal_modifier_mask(wm, oe->xbutton.state, ne->xbutton.state)
        && is_pointer_on_win(wm, ne->xbutton.window);
}

void choose_client(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);
    if(c->area_type == ICONIFY_AREA)
        move_client(wm, c, get_area_head(wm, c->icon->area_type), c->icon->area_type);
    if(DESKTOP(wm)->cur_layout == PREVIEW)
        change_layout(wm, e, (Func_arg){.layout=DESKTOP(wm)->prev_layout});
}

void exec(WM *wm, XEvent *e, Func_arg arg)
{
    exec_cmd(wm, arg.cmd);
}

void key_move_resize_client(WM *wm, XEvent *e, Func_arg arg)
{
    Layout lay=DESKTOP(wm)->cur_layout;
    Client *c=CUR_FOC_CLI(wm);
    if(lay==TILE || lay==STACK || (lay==FULL && c->area_type==FLOATING_AREA))
    {
        Direction dir=arg.direction;
        bool is_move = (dir==UP || dir==DOWN || dir==LEFT || dir==RIGHT);
        Delta_rect d=get_key_delta_rect(c, dir);
        if(c->area_type!=FLOATING_AREA && lay==TILE)
            move_client(wm, c, get_area_head(wm, FLOATING_AREA), FLOATING_AREA);
        if(fix_move_resize_delta_rect(wm, c, &d, is_move))
        {
            move_resize_client(wm, c, &d);
            update_hint_win_for_resize(wm, c);
            while(1)
            {
                XEvent ev;
                XMaskEvent(wm->display, ROOT_EVENT_MASK|KeyReleaseMask, &ev);
                if( ev.type==KeyRelease && ev.xkey.state==e->xkey.state
                    && ev.xkey.keycode==e->xkey.keycode)
                {
                    XUnmapWindow(wm->display, wm->hint_win);
                    break;
                }
                else
                    wm->event_handlers[ev.type](wm, &ev);
            }
        }
    }
}

static Delta_rect get_key_delta_rect(Client *c, Direction dir)
{
    int wi=c->size_hint.width_inc, hi=c->size_hint.height_inc;
    Delta_rect dr[] =
    {
        [UP]          = {  0, -hi,   0,   0},
        [DOWN]        = {  0,  hi,   0,   0},
        [LEFT]        = {-wi,   0,   0,   0},
        [RIGHT]       = { wi,   0,   0,   0},
        [LEFT2LEFT]   = {-wi,   0,  wi,   0},
        [LEFT2RIGHT]  = { wi,   0, -wi,   0},
        [RIGHT2LEFT]  = {  0,   0, -wi,   0},
        [RIGHT2RIGHT] = {  0,   0,  wi,   0},
        [UP2UP]       = {  0, -hi,   0,  hi},
        [UP2DOWN]     = {  0,  hi,   0, -hi},
        [DOWN2UP]     = {  0,   0,   0, -hi},
        [DOWN2DOWN]   = {  0,   0,   0,  hi},
    };
    return dr[dir];
}

void quit_wm(WM *wm, XEvent *e, Func_arg arg)
{
    clear_wm(wm);
    exit(EXIT_SUCCESS);
}

void clear_wm(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        XReparentWindow(wm->display, c->win, wm->root_win, c->x, c->y);
        del_client(wm, c, true);
    }
    XDestroyWindow(wm->display, wm->taskbar->win);
    XDestroyWindow(wm->display, wm->cmd_center->win);
    XDestroyWindow(wm->display, wm->run_cmd->win);
    XDestroyWindow(wm->display, wm->hint_win);
    XFreeGC(wm->display, wm->gc);
    XFreeModifiermap(wm->mod_map);
    for(size_t i=0; i<POINTER_ACT_N; i++)
        XFreeCursor(wm->display, wm->cursors[i]);
    XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
    if(wm->run_cmd->xic)
        XDestroyIC(wm->run_cmd->xic);
    if(wm->xim)
        XCloseIM(wm->xim);
    close_fonts(wm);
    XClearWindow(wm->display, wm->root_win);
    XFlush(wm->display);
    XCloseDisplay(wm->display);
    clear_zombies(0);
    free_files(wm->wallpapers);
    for(size_t i=0; i<DESKTOP_N; i++)
        free(wm->desktop[i]);
    vfree(wm->taskbar->status_text, wm->taskbar, wm->cmd_center, wm->run_cmd,
        wm->cfg, NULL);
}

void close_client(WM *wm, XEvent *e, Func_arg arg)
{
    /* 刪除窗口會產生UnmapNotify事件，處理該事件時再刪除框架 */
    Client *c=CUR_FOC_CLI(wm);
    if( c != wm->clients
        && !send_event(wm, wm->icccm_atoms[WM_DELETE_WINDOW], c->win))
        XDestroyWindow(wm->display, c->win);
}

void close_all_clients(WM *wm, XEvent *e, Func_arg arg)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(!send_event(wm, wm->icccm_atoms[WM_DELETE_WINDOW], c->win))
            XDestroyWindow(wm->display, c->win);
}

/* 取得窗口疊次序意義上的下一個客戶窗口 */
void next_client(WM *wm, XEvent *e, Func_arg arg)
{   /* 允許切換至根窗口 */
    Client *c=get_prev_client(wm, CUR_FOC_CLI(wm));
    focus_client(wm, wm->cur_desktop, c ? c : wm->clients);
}

/* 取得窗口疊次序意義上的上一個客戶窗口 */
void prev_client(WM *wm, XEvent *e, Func_arg arg)
{   /* 允許切換至根窗口 */
    Client *c=get_next_client(wm, CUR_FOC_CLI(wm));
    focus_client(wm, wm->cur_desktop, c ? c : wm->clients);
}

void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg)
{
    if(DESKTOP(wm)->cur_layout == TILE)
    {
        int *m=&DESKTOP(wm)->n_main_max;
        *m = *m+arg.desktop_n>=1 ? *m+arg.desktop_n : 1;
        update_layout(wm);
    }
}

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    if(DESKTOP(wm)->cur_layout==TILE && get_typed_clients_n(wm, SECOND_AREA))
    {
        Desktop *d=DESKTOP(wm);
        double mr=d->main_area_ratio+arg.change_ratio, fr=d->fixed_area_ratio;
        int mw=mr*wm->screen_width, sw=wm->screen_width*(1-fr)-mw;
        if(sw>=wm->cfg->resize_inc && mw>=wm->cfg->resize_inc)
        {
            d->main_area_ratio=mr;
            update_layout(wm);
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{ 
    if(DESKTOP(wm)->cur_layout==TILE && get_typed_clients_n(wm, FIXED_AREA))
    {
        Desktop *d=DESKTOP(wm);
        double fr=d->fixed_area_ratio+arg.change_ratio, mr=d->main_area_ratio;
        int mw=wm->screen_width*(mr-arg.change_ratio), fw=wm->screen_width*fr;
        if(mw>=wm->cfg->resize_inc && fw>=wm->cfg->resize_inc)
        {
            d->main_area_ratio-=arg.change_ratio, d->fixed_area_ratio=fr;
            update_layout(wm);
        }
    }
}

void change_area(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);
    Layout l=DESKTOP(wm)->cur_layout;
    Area_type t=arg.area_type==PREV_AREA ? c->icon->area_type : arg.area_type;
    if( c!=wm->clients && (l==TILE || (l==STACK
        && (c->area_type==ICONIFY_AREA || t==ICONIFY_AREA))))
        move_client(wm, c, get_area_head(wm, t), t);
}

void pointer_swap_clients(WM *wm, XEvent *e, Func_arg arg)
{
    XEvent ev;
    Layout layout=DESKTOP(wm)->cur_layout;
    Client *from=CUR_FOC_CLI(wm), *to=NULL, *head=wm->clients;
    if(layout!=TILE || from==head || !get_valid_click(wm, SWAP, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    int x=ev.xbutton.x-wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N;
    if((to=win_to_client(wm, ev.xbutton.subwindow)) == NULL)
        for(Client *c=head->next; c!=head && !to; c=c->next)
            if( c!=from && c->area_type==ICONIFY_AREA
                && x>=c->icon->x && x<c->icon->x+c->icon->w)
                to=c;
    if(to) 
        swap_clients(wm, from, to);
}

void maximize_client(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);
    if(c != wm->clients)
    {
        unsigned int bw=c->border_w, th=c->title_bar_h;
        c->x=bw, c->y=bw+th;
        c->w=wm->screen_width-2*bw;
        c->h=wm->screen_height-2*bw-th-wm->taskbar->h;
        if(DESKTOP(wm)->cur_layout == TILE)
            move_client(wm, c, get_area_head(wm, FLOATING_AREA), FLOATING_AREA);
        move_resize_client(wm, c, NULL);
    }
}

void pointer_move_resize_client(WM *wm, XEvent *e, Func_arg arg)
{
    Layout layout=DESKTOP(wm)->cur_layout;
    Move_info m={e->xbutton.x_root, e->xbutton.y_root, 0, 0};
    Client *c=CUR_FOC_CLI(wm);
    Pointer_act act=(arg.resize ? get_resize_act(c, &m) : MOVE);
    if( (layout==FULL && c->area_type!=FLOATING_AREA) || layout==PREVIEW
        || !grab_pointer(wm, wm->root_win, act))
        return;

    XEvent ev;
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            if(c->area_type!=FLOATING_AREA && layout==TILE)
                move_client(wm, c, get_area_head(wm, FLOATING_AREA), FLOATING_AREA);
            /* 因X事件是異步的，故xmotion.x和ev.xmotion.y可能不是連續變化 */
            m.nx=ev.xmotion.x, m.ny=ev.xmotion.y;
            do_valid_pointer_move_resize(wm, c, &m, act);
        }
        else
            wm->event_handlers[ev.type](wm, &ev);
    }while(!is_match_button_release(wm, e, &ev));
    XUngrabPointer(wm->display, CurrentTime);
    XUnmapWindow(wm->display, wm->hint_win);
}

static void do_valid_pointer_move_resize(WM *wm, Client *c, Move_info *m, Pointer_act act)
{
    Delta_rect d=get_pointer_delta_rect(m, act);
    if(fix_move_resize_delta_rect(wm, c, &d, act==MOVE))
    {
        move_resize_client(wm, c, &d);
        update_hint_win_for_resize(wm, c);
        if(act != MOVE)
        {
            if(d.dw) // dx爲0表示定位器從窗口右邊調整尺寸，非0則表示左邊調整
                m->ox = d.dx ? m->ox-d.dw : m->ox+d.dw;
            if(d.dh) // dy爲0表示定位器從窗口下邊調整尺寸，非0則表示上邊調整
                m->oy = d.dy ? m->oy-d.dh : m->oy+d.dh;
        }
        else
            m->ox=m->nx, m->oy=m->ny;
    }
}

static bool fix_move_resize_delta_rect(WM *wm, Client *c, Delta_rect *d, bool is_move)
{
    if( is_prefer_move(wm, c, d, is_move)
        || fix_delta_rect_for_nonprefer_size(c, &c->size_hint, d))
        return true;

    int dw=d->dw, dh=d->dh;
    fix_dw_by_width_hint(c->w, &c->size_hint, &dw);
    fix_dh_by_height_hint(c->w, &c->size_hint, &dh);
    return fix_delta_rect_for_prefer_size(c, &c->size_hint, dw, dh, d);
}

static bool is_prefer_move(WM *wm, Client *c, Delta_rect *d, bool is_move)
{
    return (is_move && is_on_screen(wm, c->x+d->dx, c->y+d->dy, c->w, c->h));
}

static bool fix_delta_rect_for_nonprefer_size(Client *c, XSizeHints *hint, Delta_rect *d)
{
    if((d->dw || d->dh) && !is_prefer_size(c->w, c->h, hint)) // 首次調整尺寸時才要考慮爲偏好而修正尺寸
    {
        int ox=c->x, oy=c->y, ow=c->w, oh=c->h;
        fix_win_size_by_hint(c);
        d->dx=c->x-ox, d->dy=c->y-oy, d->dw=(int)c->w-ow, d->dh=(int)c->h-oh;
        c->x=ox, c->y=oy, c->w=ow, c->h=oh;
        return true;
    }
    return false;
}

static void fix_dw_by_width_hint(int w, XSizeHints *hint, int *dw)
{
    if(*dw/hint->width_inc)
    {
        int max=MAX(hint->width_inc, hint->min_width);
        *dw=base_n_floor(*dw, hint->width_inc);
        if(w+*dw < max)
            *dw=max-w;
    }
    else
        *dw=0;
}

static void fix_dh_by_height_hint(int h, XSizeHints *hint, int *dh)
{
    if(*dh/hint->height_inc)
    {
        int max=MAX(hint->height_inc, hint->min_height);
        *dh=base_n_floor(*dh, hint->height_inc);
        if(h+*dh < max)
            *dh=max-h;
    }
    else
        *dh=0;
}

static bool fix_delta_rect_for_prefer_size(Client *c, XSizeHints *hint, int dw, int dh, Delta_rect *d)
{
    if((dw || dh) && is_prefer_size(c->w+dw, c->h+dh, hint))
    {
        d->dw=dw, d->dh=dh;
        if(d->dx)
            d->dx=-dw;
        if(d->dy)
            d->dy=-dh;
        return true;
    }
    return false;
}

static void update_hint_win_for_resize(WM *wm, Client *c)
{
    char str[BUFSIZ];
    unsigned int w=get_client_col(c), h=get_client_row(c);
    sprintf(str, "(%d, %d) %ux%u", c->x, c->y, w, h);
    get_string_size(wm, wm->font[HINT_FONT], str, &w, NULL);
    int x=(wm->screen_width-w)/2, y=(wm->screen_height-wm->cfg->hint_win_line_height)/2;
    XMoveResizeWindow(wm->display, wm->hint_win, x, y, w, wm->cfg->hint_win_line_height);
    XMapRaised(wm->display, wm->hint_win);
    String_format f={{0, 0, w, wm->cfg->hint_win_line_height}, CENTER,
        false, 0, wm->text_color[wm->cfg->color_theme][HINT_TEXT_COLOR], HINT_FONT};
    draw_string(wm, wm->hint_win, str, &f);
}

static Delta_rect get_pointer_delta_rect(const Move_info *m, Pointer_act act)
{
    int dx=m->nx-m->ox, dy=m->ny-m->oy;
    Delta_rect dr[] =
    {
        [NO_OP]               = { 0,  0,   0,   0},
        [MOVE]                = {dx, dy,   0,   0},
        [TOP_RESIZE]          = { 0, dy,   0, -dy},
        [BOTTOM_RESIZE]       = { 0,  0,   0,  dy},
        [LEFT_RESIZE]         = {dx,  0, -dx,   0},
        [RIGHT_RESIZE]        = { 0,  0,  dx,   0},
        [TOP_LEFT_RESIZE]     = {dx, dy, -dx, -dy},
        [TOP_RIGHT_RESIZE]    = { 0, dy,  dx, -dy},
        [BOTTOM_LEFT_RESIZE]  = {dx,  0, -dx,  dy},
        [BOTTOM_RIGHT_RESIZE] = { 0,  0,  dx,  dy},
    };
    return dr[act];
}

void pointer_change_area(WM *wm, XEvent *e, Func_arg arg)
{
    XEvent ev;
    Client *from=CUR_FOC_CLI(wm), *to;
    if( DESKTOP(wm)->cur_layout!=TILE || from==wm->clients
        || !get_valid_click(wm, CHANGE, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    Window win=ev.xbutton.window, subw=ev.xbutton.subwindow;
    to=win_to_client(wm, subw);
    if(!to)
        to=win_to_iconic_state_client(wm, subw);
    if(ev.xbutton.x == 0)
        move_client(wm, from, get_area_head(wm, SECOND_AREA), SECOND_AREA);
    else if(ev.xbutton.x == wm->screen_width-1)
        move_client(wm, from, get_area_head(wm, FIXED_AREA), FIXED_AREA);
    else if(ev.xbutton.y == 0)
        maximize_client(wm, NULL, arg);
    else if(subw == wm->taskbar->win)
        move_client(wm, from, get_area_head(wm, ICONIFY_AREA), ICONIFY_AREA);
    else if(win==wm->root_win && subw==None)
        move_client(wm, from, get_area_head(wm, MAIN_AREA), MAIN_AREA);
    else if(to)
        move_client(wm, from, to, to->area_type);
}

void change_layout(WM *wm, XEvent *e, Func_arg arg)
{
    Layout *cl=&DESKTOP(wm)->cur_layout, *pl=&DESKTOP(wm)->prev_layout;
    if(*cl != arg.layout)
    {
        Display *d=wm->display;
        *pl=*cl, *cl=arg.layout;
        if(*pl == PREVIEW)
            for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
                if(is_on_cur_desktop(wm, c) && c->area_type==ICONIFY_AREA)
                    XMapWindow(d, c->icon->win), XUnmapWindow(d, c->frame);
        if(*cl == PREVIEW)
            for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
                if(is_on_cur_desktop(wm, c) && c->area_type==ICONIFY_AREA)
                    XMapWindow(d, c->frame), XUnmapWindow(d, c->icon->win);
        update_layout(wm);
        update_title_bar_layout(wm);
        update_taskbar_buttons(wm);
    }
}

void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    if( DESKTOP(wm)->cur_layout!=TILE
        || !is_layout_adjust_area(wm, e->xbutton.window, e->xbutton.x_root)
        || !grab_pointer(wm, wm->root_win, ADJUST_LAYOUT_RATIO))
        return;
    int ox=e->xbutton.x_root, nx, dx;
    XEvent ev;
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, dx=nx-ox;
            if(abs(dx)>=wm->cfg->resize_inc && change_layout_ratio(wm, ox, nx))
                update_layout(wm), ox=nx;
        }
        else
            wm->event_handlers[ev.type](wm, &ev);
    }while(!is_match_button_release(wm, e, &ev));
    XUngrabPointer(wm->display, CurrentTime);
}

void iconify_all_clients(WM *wm, XEvent *e, Func_arg arg)
{
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c) && c->area_type!=ICONIFY_AREA)
            iconify(wm, c);
}

void deiconify_all_clients(WM *wm, XEvent *e, Func_arg arg)
{
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c) && c->area_type==ICONIFY_AREA)
            deiconify(wm, c);
    update_layout(wm);
}

void change_default_area_type(WM *wm, XEvent *e, Func_arg arg)
{
    DESKTOP(wm)->default_area_type=arg.area_type;
}

void toggle_focus_mode(WM *wm, XEvent *e, Func_arg arg)
{
    wm->cfg->focus_mode = wm->cfg->focus_mode==ENTER_FOCUS ? CLICK_FOCUS : ENTER_FOCUS;
}

void open_cmd_center(WM *wm, XEvent *e, Func_arg arg)
{
    show_menu(wm, e, wm->cmd_center, wm->taskbar->buttons[TASKBAR_BUTTON_INDEX(CMD_CENTER_ITEM)]);
}

void toggle_border_visibility(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);
    c->border_w = c->border_w ? 0 : wm->cfg->border_width;
    XSetWindowBorderWidth(wm->display, c->frame, c->border_w);
    update_layout(wm);
}

void toggle_title_bar_visibility(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);
    c->title_bar_h = c->title_bar_h ? 0 : wm->cfg->title_bar_height;
    if(c->title_bar_h)
    {
        create_title_bar(wm, c);
        XMapSubwindows(wm->display, c->frame);
    }
    else
    {
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            XDestroyWindow(wm->display, c->buttons[i]);
        XDestroyWindow(wm->display, c->title_area);
    }
    update_layout(wm);
}

void focus_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void next_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, wm->cur_desktop<DESKTOP_N ? wm->cur_desktop+1 : 1);
}

void prev_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, wm->cur_desktop>1 ? wm->cur_desktop-1 : DESKTOP_N);
}

void move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    move_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void all_move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_move_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    change_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void all_change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_change_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    attach_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void attach_to_all_desktops(WM *wm, XEvent *e, Func_arg arg)
{
    attach_to_desktop_all(wm);
}

void all_attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_attach_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void enter_and_run_cmd(WM *wm, XEvent *e, Func_arg arg)
{
    show_entry(wm, wm->run_cmd);
}

void change_wallpaper(WM *wm, XEvent *e, Func_arg arg)
{
    srand((unsigned int)time(NULL));
    unsigned long r1=rand(), r2=rand(), color=(r1<<32)|r2;
    Pixmap pixmap=None;
    if(wm->cfg->wallpaper_paths)
    {
        File *f=wm->cur_wallpaper;
        if(f)
        {
            f=wm->cur_wallpaper=(f->next ? f->next : wm->wallpapers->next);
            pixmap=create_pixmap_from_file(wm, wm->root_win, f->name);
        }
    }
    update_win_background(wm, wm->root_win, color, pixmap);
    if(pixmap)
        XFreePixmap(wm->display, pixmap);
}

void print_screen(WM *wm, XEvent *e, Func_arg arg)
{
    print_area(wm, wm->root_win, 0, 0, wm->screen_width, wm->screen_height);
}

void print_win(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);
    if(c != wm->clients)
        print_area(wm, c->frame, 0, 0, c->w, c->h);
}

void switch_color_theme(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cfg->color_theme < COLOR_THEME_N-1)
        wm->cfg->color_theme++;
    else
        wm->cfg->color_theme=0;
    // 以下函數會產生Expose事件，而處理Expose事件時會更新窗口的文字
    // 內容及其顏色，故此處不必更新構件文字顏色。
    update_widget_color(wm);
}
