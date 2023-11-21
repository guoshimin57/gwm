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

static bool is_match_button_release(XEvent *oe, XEvent *ne);
static bool is_valid_click(XEvent *oe, XEvent *ne);
static Delta_rect get_key_delta_rect(Client *c, Direction dir);
static void do_valid_pointer_move_resize(WM *wm, Client *c, Move_info *m, Pointer_act act);
static bool fix_move_resize_delta_rect(Client *c, Delta_rect *d, bool is_move);
static bool is_prefer_move(Client *c, Delta_rect *d, bool is_move);
static bool fix_delta_rect_for_nonprefer_size(Client *c, XSizeHints *hint, Delta_rect *d);
static void fix_dw_by_width_hint(int w, XSizeHints *hint, int *dw);
static void fix_dh_by_height_hint(int h, XSizeHints *hint, int *dh);
static bool fix_delta_rect_for_prefer_size(Client *c, XSizeHints *hint, int dw, int dh, Delta_rect *d);
static void update_hint_win_for_move_resize(Client *c);
static Delta_rect get_pointer_delta_rect(const Move_info *m, Pointer_act act);

bool is_drag_func(void (*func)(WM *, XEvent *, Func_arg))
{
    return func == pointer_swap_clients
        || func == pointer_move_resize_client
        || func == pointer_change_place
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

    Window win = is_grab_root_act(act) ? xinfo.root_win : oe->xbutton.window;
    if(act!=NO_OP && !grab_pointer(wm, win, act))
        return false;

    XEvent e, *p=(ne ? ne : &e);
    do
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, p);
        wm->event_handlers[p->type](wm, p);
    }while(!is_match_button_release(oe, p));
    if(act != NO_OP)
        XUngrabPointer(xinfo.display, CurrentTime);
    return is_valid_click(oe, p);
}

static bool is_match_button_release(XEvent *oe, XEvent *ne)
{
    return (ne->type==ButtonRelease && ne->xbutton.button==oe->xbutton.button);
}

static bool is_valid_click(XEvent *oe, XEvent *ne)
{
    return is_equal_modifier_mask(oe->xbutton.state, ne->xbutton.state)
        && is_pointer_on_win(ne->xbutton.window);
}

void choose_client(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);

    if(c->icon)
        deiconify(wm, c);

    if(DESKTOP(wm)->cur_layout == PREVIEW)
        arg.layout=DESKTOP(wm)->prev_layout, change_layout(wm, e, arg);
}

void exec(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e);
    exec_cmd(arg.cmd);
}

void key_move_resize_client(WM *wm, XEvent *e, Func_arg arg)
{
    if(DESKTOP(wm)->cur_layout == PREVIEW)
        return;

    Client *c=CUR_FOC_CLI(wm);
    Direction dir=arg.direction;
    bool is_move = (dir==UP || dir==DOWN || dir==LEFT || dir==RIGHT);
    Delta_rect d=get_key_delta_rect(c, dir);
    Place_type type=get_dest_place_type_for_move(wm, c);

    if(c->place_type!=FLOAT_LAYER && type==FLOAT_LAYER)
        move_client(wm, c, NULL, type);
    if(fix_move_resize_delta_rect(c, &d, is_move))
    {
        move_resize_client(wm, c, &d);
        update_hint_win_for_move_resize(c);
        while(1)
        {
            XEvent ev;
            XMaskEvent(xinfo.display, ROOT_EVENT_MASK|KeyReleaseMask, &ev);
            if( ev.type==KeyRelease && ev.xkey.state==e->xkey.state
                && ev.xkey.keycode==e->xkey.keycode)
            {
                XUnmapWindow(xinfo.display, xinfo.hint_win);
                break;
            }
            else
                wm->event_handlers[ev.type](wm, &ev);
        }
    }
    update_win_state_for_move_resize(wm, c);
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
    UNUSED(e), UNUSED(arg);
    clear_wm(wm);
    exit(EXIT_SUCCESS);
}

void clear_wm(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        XReparentWindow(xinfo.display, c->win, xinfo.root_win, c->x, c->y);
        del_client(wm, c, true);
    }
    XDestroyWindow(xinfo.display, wm->taskbar->win);
    XDestroyWindow(xinfo.display, wm->act_center->win);
    XDestroyWindow(xinfo.display, wm->client_menu->win);
    XDestroyWindow(xinfo.display, xinfo.hint_win);
    XDestroyWindow(xinfo.display, wm->wm_check_win);
    destroy_entry(wm->run_cmd);
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        XDestroyWindow(xinfo.display, wm->top_wins[i]);
    XFreeGC(xinfo.display, wm->gc);
    XFreeModifiermap(xinfo.mod_map);
    for(size_t i=0; i<POINTER_ACT_N; i++)
        XFreeCursor(xinfo.display, wm->cursors[i]);
    XSetInputFocus(xinfo.display, xinfo.root_win, RevertToPointerRoot, CurrentTime);
    if(xinfo.xim)
        XCloseIM(xinfo.xim);
    close_font(wm);
    XClearWindow(xinfo.display, xinfo.root_win);
    XFlush(xinfo.display);
    XCloseDisplay(xinfo.display);
    clear_zombies(0);
    free_strings(wm->wallpapers);
    for(size_t i=0; i<DESKTOP_N; i++)
        free(wm->desktop[i]);
    vfree(wm->taskbar->status_text, wm->taskbar, wm->act_center, wm->run_cmd,
        cfg, NULL);
}

void close_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    /* 刪除窗口會產生UnmapNotify事件，處理該事件時再刪除框架 */
    Client *c=CUR_FOC_CLI(wm);
    if(c != wm->clients)
        close_win(c->win);
}

void close_all_clients(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            close_win(c->win);
}

/* 取得存儲次序上在當前客戶之前的客戶（或其亞組長）。因使用頭插法存儲客戶，
 * 故若兩者同放置類型的話，該客戶在時間上比當前客戶出現得遲(next) */
void next_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_client(wm, wm->cur_desktop, get_prev_client(wm, CUR_FOC_CLI(wm)));
}

/* 取得存儲次序上在當前客戶（或其亞組長）之後的客戶。因使用頭插法存儲客戶，
 * 故若兩者同放置類型的話，該客戶在時間上比當前客戶出現得早(prev) */
void prev_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_client(wm, wm->cur_desktop, get_next_client(wm, CUR_FOC_CLI(wm)));
}

void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
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
    UNUSED(e), UNUSED(arg);
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm, TILE_LAYER_SECOND, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double mr=d->main_area_ratio+arg.change_ratio, fr=d->fixed_area_ratio;
        long mw=mr*wm->workarea.w, sw=wm->workarea.w*(1-fr)-mw;
        if(sw>=cfg->resize_inc && mw>=cfg->resize_inc)
        {
            d->main_area_ratio=mr;
            update_layout(wm);
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{ 
    UNUSED(e), UNUSED(arg);
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm, TILE_LAYER_FIXED, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double fr=d->fixed_area_ratio+arg.change_ratio, mr=d->main_area_ratio;
        long mw=wm->workarea.w*(mr-arg.change_ratio), fw=wm->workarea.w*fr;
        if(mw>=cfg->resize_inc && fw>=cfg->resize_inc)
        {
            d->main_area_ratio-=arg.change_ratio, d->fixed_area_ratio=fr;
            update_layout(wm);
        }
    }
}

void change_place(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Client *c=CUR_FOC_CLI(wm);

    move_client(wm, c, NULL, arg.place_type);
    update_win_state_for_move_resize(wm, c);
}

void pointer_swap_clients(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
    XEvent ev;
    Layout layout=DESKTOP(wm)->cur_layout;
    Client *from=CUR_FOC_CLI(wm), *to=NULL, *head=wm->clients;
    if(layout!=TILE || from==head || !get_valid_click(wm, SWAP, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    if((to=win_to_client(wm, ev.xbutton.subwindow)))
        swap_clients(wm, from, to);
}

void minimize_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    iconify(wm, CUR_FOC_CLI(wm)); 
}

void deiconify_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    deiconify(wm, CUR_FOC_CLI(wm)); 
}

void max_restore_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);

    if(c == wm->clients)
        return;

    if(is_win_state_max(c))
    {
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
        restore_client(wm, c);
    }
    else
    {
        max_client(wm, c, FULL_MAX);
        c->win_state.vmax=c->win_state.hmax=1;
    }
    update_net_wm_state(c->win, c->win_state);
}

void maximize_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Client *c=CUR_FOC_CLI(wm);

    if(c == wm->clients)
        return;

    max_client(wm, c, arg.max_way);
    switch(arg.max_way)
    {
        case VERT_MAX:   c->win_state.vmax=1; break;
        case HORZ_MAX:   c->win_state.hmax=1; break;
        case TOP_MAX:    c->win_state.tmax=1; break;
        case BOTTOM_MAX: c->win_state.bmax=1; break;
        case LEFT_MAX:   c->win_state.lmax=1; break;
        case RIGHT_MAX:  c->win_state.rmax=1; break;
        case FULL_MAX:   c->win_state.vmax=c->win_state.hmax=1; break;
    }
    update_net_wm_state(c->win, c->win_state);
}

void toggle_shade_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool shade=false;

    toggle_shade_client_mode(CUR_FOC_CLI(wm), shade=!shade);
}

void toggle_shade_client_mode(Client *c, bool shade)
{
    if(shade && c->titlebar_h)
        XResizeWindow(xinfo.display, c->frame, c->w, c->titlebar_h);
    else if(!shade)
        XResizeWindow(xinfo.display, c->frame, c->w, c->titlebar_h+c->h);
    c->win_state.shaded=shade;
}

void pointer_move_resize_client(WM *wm, XEvent *e, Func_arg arg)
{
    Layout layout=DESKTOP(wm)->cur_layout;
    Move_info m={e->xbutton.x_root, e->xbutton.y_root, 0, 0};
    Client *c=CUR_FOC_CLI(wm);
    Pointer_act act=(arg.resize ? get_resize_act(c, &m) : MOVE);

    if(layout==PREVIEW || !grab_pointer(wm, xinfo.root_win, act))
        return;

    XEvent ev;
    Place_type type=get_dest_place_type_for_move(wm, c);
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            if(c->place_type!=FLOAT_LAYER && type==FLOAT_LAYER)
                move_client(wm, c, NULL, type);
            /* 因X事件是異步的，故xmotion.x和ev.xmotion.y可能不是連續變化 */
            m.nx=ev.xmotion.x, m.ny=ev.xmotion.y;
            do_valid_pointer_move_resize(wm, c, &m, act);
        }
        else
            wm->event_handlers[ev.type](wm, &ev);
    }while(!is_match_button_release(e, &ev));
    XUngrabPointer(xinfo.display, CurrentTime);
    XUnmapWindow(xinfo.display, xinfo.hint_win);
    update_win_state_for_move_resize(wm, c);
}

static void do_valid_pointer_move_resize(WM *wm, Client *c, Move_info *m, Pointer_act act)
{
    Delta_rect d=get_pointer_delta_rect(m, act);

    if(!fix_move_resize_delta_rect(c, &d, act==MOVE))
        return;

    move_resize_client(wm, c, &d);
    update_hint_win_for_move_resize(c);
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

static bool fix_move_resize_delta_rect(Client *c, Delta_rect *d, bool is_move)
{
    if( is_prefer_move(c, d, is_move)
        || fix_delta_rect_for_nonprefer_size(c, &c->size_hint, d))
        return true;

    int dw=d->dw, dh=d->dh;
    fix_dw_by_width_hint(c->w, &c->size_hint, &dw);
    fix_dh_by_height_hint(c->w, &c->size_hint, &dh);
    return fix_delta_rect_for_prefer_size(c, &c->size_hint, dw, dh, d);
}

static bool is_prefer_move(Client *c, Delta_rect *d, bool is_move)
{
    return (is_move && is_on_screen(c->x+d->dx, c->y+d->dy, c->w, c->h));
}

static bool fix_delta_rect_for_nonprefer_size(Client *c, XSizeHints *hint, Delta_rect *d)
{
    // 首次調整尺寸時才要考慮爲偏好而修正尺寸
    if((!d->dw && !d->dh) || is_prefer_size(c->w, c->h, hint))
        return false;

    int ox=c->x, oy=c->y, ow=c->w, oh=c->h;
    fix_win_size_by_hint(&c->size_hint, &c->w, &c->h);
    d->dx=c->x-ox, d->dy=c->y-oy, d->dw=c->w-ow, d->dh=c->h-oh;
    c->x=ox, c->y=oy, c->w=ow, c->h=oh;
    return true;
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
    if((!dw && !dh) || !is_prefer_size(c->w+dw, c->h+dh, hint))
        return false;

    d->dw=dw, d->dh=dh;
    if(d->dx)
        d->dx=-dw;
    if(d->dy)
        d->dy=-dh;
    return true;
}

static void update_hint_win_for_move_resize(Client *c)
{
    char str[BUFSIZ];
    long col=get_win_col(c->w, &c->size_hint),
         row=get_win_row(c->h, &c->size_hint);

    sprintf(str, "(%d, %d) %ldx%ld", c->x, c->y, col, row);
    update_hint_win_for_info(None, str);
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

void pointer_change_place(WM *wm, XEvent *e, Func_arg arg)
{
    XEvent ev;
    Client *from=CUR_FOC_CLI(wm), *to;

    UNUSED(arg);
    if( DESKTOP(wm)->cur_layout!=TILE || from==wm->clients
        || !get_valid_click(wm, CHANGE, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    Window win=ev.xbutton.window, subw=ev.xbutton.subwindow;
    to=win_to_client(wm, subw);
    if(ev.xbutton.x == 0)
        move_client(wm, from, NULL, TILE_LAYER_SECOND);
    else if(ev.xbutton.x == (long)xinfo.screen_width-1)
        move_client(wm, from, NULL, TILE_LAYER_FIXED);
    else if(win==xinfo.root_win && subw==None)
        move_client(wm, from, NULL, TILE_LAYER_MAIN);
    else if(to)
        move_client(wm, from, to, ANY_PLACE);
    update_win_state_for_move_resize(wm, from);
}

void change_layout(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Layout *cl=&DESKTOP(wm)->cur_layout, *pl=&DESKTOP(wm)->prev_layout;

    if(*cl == arg.layout)
        return;

    if(arg.layout == PREVIEW)
        save_place_info_of_clients(wm);
    if(*cl == PREVIEW)
        restore_place_info_of_clients(wm);

    Display *d=xinfo.display;
    *pl=*cl, *cl=arg.layout;
    if(*pl == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->icon)
                XMapWindow(d, c->icon->win), XUnmapWindow(d, c->frame);
    if(*cl == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->icon)
                XMapWindow(d, c->frame), XUnmapWindow(d, c->icon->win);

    if(*pl==TILE && *cl==STACK)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && is_normal_layer(c->place_type))
                c->place_type=FLOAT_LAYER;

    if(*pl==STACK && *cl==TILE)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->place_type==FLOAT_LAYER)
                c->place_type=TILE_LAYER_MAIN;

    update_layout(wm);
    update_titlebar_layout(wm);
    update_taskbar_buttons_bg(wm);
}

void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
    if( DESKTOP(wm)->cur_layout!=TILE
        || !is_layout_adjust_area(wm, e->xbutton.window, e->xbutton.x_root)
        || !grab_pointer(wm, xinfo.root_win, ADJUST_LAYOUT_RATIO))
        return;

    int ox=e->xbutton.x_root, nx, dx;
    XEvent ev;
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, dx=nx-ox;
            if(abs(dx)>=cfg->resize_inc && change_layout_ratio(wm, ox, nx))
                update_layout(wm), ox=nx;
        }
        else
            wm->event_handlers[ev.type](wm, &ev);
    }while(!is_match_button_release(e, &ev));
    XUngrabPointer(xinfo.display, CurrentTime);
}

void show_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool show=false;

    toggle_showing_desktop_mode(wm, show=!show);
}

void toggle_showing_desktop_mode(WM *wm, bool show)
{
    if(show)
        iconify_all_clients(wm);
    else
        deiconify_all_clients(wm);
    set_net_showing_desktop(show);
}

void change_default_place_type(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    DESKTOP(wm)->default_place_type=arg.place_type;
}

void toggle_focus_mode(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    cfg->focus_mode = cfg->focus_mode==ENTER_FOCUS ? CLICK_FOCUS : ENTER_FOCUS;
}

void open_act_center(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
    show_menu(e, wm->act_center, e->xbutton.window);
}

void open_client_menu(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
    show_menu(e, wm->client_menu, e->xbutton.window);
}

void toggle_border_visibility(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);
    c->border_w = c->border_w ? 0 : cfg->border_width;
    XSetWindowBorderWidth(xinfo.display, c->frame, c->border_w);
    update_layout(wm);
}

void toggle_titlebar_visibility(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);
    c->titlebar_h = c->titlebar_h ? 0 : get_font_height_by_pad();
    if(c->titlebar_h)
    {
        create_titlebar(wm, c);
        XMapSubwindows(xinfo.display, c->frame);
    }
    else
    {
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            XDestroyWindow(xinfo.display, c->buttons[i]);
        XDestroyWindow(xinfo.display, c->title_area);
        XDestroyWindow(xinfo.display, c->logo);
    }
    update_layout(wm);
}

void focus_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void next_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_desktop_n(wm, wm->cur_desktop<DESKTOP_N ? wm->cur_desktop+1 : 1);
}

void prev_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_desktop_n(wm, wm->cur_desktop>1 ? wm->cur_desktop-1 : DESKTOP_N);
}

void move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    move_to_desktop_n(wm, CUR_FOC_CLI(wm), get_desktop_n(wm, e, arg));
}

void all_move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_move_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    change_to_desktop_n(wm, CUR_FOC_CLI(wm), get_desktop_n(wm, e, arg));
}

void all_change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_change_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    attach_to_desktop_n(wm, CUR_FOC_CLI(wm), get_desktop_n(wm, e, arg));
}

void attach_to_all_desktops(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    attach_to_desktop_all(wm, CUR_FOC_CLI(wm));
}

void all_attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_attach_to_desktop_n(wm, get_desktop_n(wm, e, arg));
}

void enter_and_run_cmd(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    show_entry(wm->run_cmd);
}

void switch_wallpaper(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    srand((unsigned int)time(NULL));
    unsigned long r1=rand(), r2=rand(), color=(r1<<16)|r2|0xff000000UL;
    Pixmap pixmap=None;
    if(cfg->wallpaper_paths)
    {
        Strings *f=wm->cur_wallpaper;
        if(f)
        {
            f=wm->cur_wallpaper=(f->next ? f->next : wm->wallpapers->next);
            pixmap=create_pixmap_from_file(xinfo.root_win, f->str);
        }
    }
    update_win_bg(xinfo.root_win, color, pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}

void print_screen(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    print_area(xinfo.root_win, 0, 0, xinfo.screen_width, xinfo.screen_height);
}

void print_win(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);
    if(c != wm->clients)
        print_area(c->frame, 0, 0, c->w, c->h);
}

void switch_color_theme(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    if(cfg->color_theme < COLOR_THEME_N-1)
        cfg->color_theme++;
    else
        cfg->color_theme=0;
    // 以下函數會產生Expose事件，而處理Expose事件時會更新窗口的文字
    // 內容及其顏色，故此處不必更新構件文字顏色。
    update_widget_bg(wm);
}

void toggle_compositor(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Window win=get_compositor();

    if(win)
        XKillClient(xinfo.display, win);
    else
        exec(wm, e, (Func_arg)SH_CMD((char *)cfg->compositor));
}
