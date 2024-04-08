/* *************************************************************************
 *     mvresize.c：實現移動、調整窗口尺寸的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void key_move_resize_client(WM *wm, XEvent *e, Direction dir);
static void pointer_move_resize_client(WM *wm, XEvent *e, bool resize);
static Delta_rect get_key_delta_rect(Client *c, Direction dir);
static int get_width_inc(const XSizeHints *h);
static int get_height_inc(const XSizeHints *h);
static void do_valid_pointer_move_resize(Client *c, Move_info *m, Pointer_act act);
static Delta_rect get_pointer_delta_rect(const Move_info *m, Pointer_act act);
static bool get_move_resize_delta_rect(Client *c, Delta_rect *d, bool is_move);
static bool is_prefer_move(Client *c, Delta_rect *d);
static void fix_dw_by_width_hint(int w, XSizeHints *hint, int *dw);
static void fix_dh_by_height_hint(int h, XSizeHints *hint, int *dh);
static bool fix_delta_rect(Client *c, Delta_rect *d);
static void update_hint_win_for_move_resize(Client *c);

void move_resize(WM *wm, XEvent *e, Func_arg arg)
{
    if(e->type == KeyPress)
        key_move_resize_client(wm, e, arg.direction);
    else
        pointer_move_resize_client(wm, e, arg.resize);
}

void key_move_resize_client(WM *wm, XEvent *e, Direction dir)
{
    if(DESKTOP(wm)->cur_layout == PREVIEW)
        return;

    Client *c=CUR_FOC_CLI(wm);
    bool is_move = (dir==UP || dir==DOWN || dir==LEFT || dir==RIGHT);
    Delta_rect d=get_key_delta_rect(c, dir);
    Place_type type=get_dest_place_type_for_move(wm, c);

    if(c->place_type!=FLOAT_LAYER && type==FLOAT_LAYER)
        move_client(wm, c, NULL, type);
    if(get_move_resize_delta_rect(c, &d, is_move))
    {
        move_resize_client(c, &d);
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

Place_type get_dest_place_type_for_move(WM *wm, Client *c)
{
    return DESKTOP(wm)->cur_layout==TILE && is_tile_client(c) ?
        FLOAT_LAYER : c->place_type;
}

static Delta_rect get_key_delta_rect(Client *c, Direction dir)
{
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    int wi=hint.width_inc, hi=hint.height_inc;

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

void move_resize_client(Client *c, const Delta_rect *d)
{
    if(d)
        WIDGET_X(c)+=d->dx, WIDGET_Y(c)+=d->dy, WIDGET_W(c)+=d->dw, WIDGET_H(c)+=d->dh;
    Rect fr=get_frame_rect(c), tr=get_title_area_rect(c);
    if(c->titlebar_h)
    {
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
        {
            Rect br=get_button_rect(c, i);
            move_resize_widget(WIDGET(c->frame->buttons[i]), br.x, br.y, br.w, br.h);
        }
        move_resize_widget(c->frame->title_area, tr.x, tr.y, tr.w, tr.h);
    }
    move_resize_widget(WIDGET(c->frame), fr.x, fr.y, fr.w, fr.h);
    XMoveResizeWindow(xinfo.display, WIDGET_WIN(c), 0, c->titlebar_h, WIDGET_W(c), WIDGET_H(c));
}

void pointer_move_resize_client(WM *wm, XEvent *e, bool resize)
{
    Layout layout=DESKTOP(wm)->cur_layout;
    Move_info m={e->xbutton.x_root, e->xbutton.y_root, 0, 0};
    Client *c=CUR_FOC_CLI(wm);
    Pointer_act act=(resize ? get_resize_act(c, &m) : MOVE);

    if(layout==PREVIEW || !grab_pointer(xinfo.root_win, act))
        return;

    XSizeHints hint=get_size_hint(WIDGET_WIN(c));

    XEvent ev;
    bool first=true;
    Place_type type=get_dest_place_type_for_move(wm, c);
    if(act==MOVE || is_resizable(&hint))
    {
        do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
        {
            XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
            if(ev.type == MotionNotify)
            {
                if(first)
                    move_client(wm, c, NULL, type), first=false;
                /* 因X事件是異步的，故xmotion.x和ev.xmotion.y可能不是連續變化 */
                m.nx=ev.xmotion.x, m.ny=ev.xmotion.y;
                do_valid_pointer_move_resize(c, &m, act);
            }
            else
                wm->event_handlers[ev.type](wm, &ev);
        }while(!is_match_button_release(e, &ev));
    }
    XUngrabPointer(xinfo.display, CurrentTime);
    XUnmapWindow(xinfo.display, xinfo.hint_win);
    update_win_state_for_move_resize(wm, c);
}

static void do_valid_pointer_move_resize(Client *c, Move_info *m, Pointer_act act)
{
    int dx=m->nx-m->ox, dy=m->ny-m->oy;
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    if(dx/get_width_inc(&hint)==0 && dy/get_height_inc(&hint)==0)
        return;

    Delta_rect d=get_pointer_delta_rect(m, act);
    if(!get_move_resize_delta_rect(c, &d, act==MOVE))
        return;

    move_resize_client(c, &d);
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

static int get_width_inc(const XSizeHints *h)
{
    return ((h->flags & PResizeInc) && h->width_inc) ? h->width_inc : cfg->resize_inc;
}

static int get_height_inc(const XSizeHints *h)
{
    return ((h->flags & PResizeInc) && h->height_inc) ? h->height_inc : cfg->resize_inc;
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

static bool get_move_resize_delta_rect(Client *c, Delta_rect *d, bool is_move)
{
    return (is_move && is_prefer_move(c, d)) || fix_delta_rect(c, d);
}

static bool is_prefer_move(Client *c, Delta_rect *d)
{
    return is_on_screen(WIDGET_X(c)+d->dx, WIDGET_Y(c)+d->dy, WIDGET_W(c), WIDGET_H(c));
}

static bool fix_delta_rect(Client *c, Delta_rect *d)
{
    int dw=d->dw, dh=d->dh;
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));

    fix_dw_by_width_hint(WIDGET_W(c), &hint, &dw);
    fix_dh_by_height_hint(WIDGET_W(c), &hint, &dh);

    if((!dw && !dh) || !is_prefer_size(WIDGET_W(c)+dw, WIDGET_H(c)+dh, &hint))
        return false;

    d->dw=dw, d->dh=dh;
    if(d->dx)
        d->dx=-dw;
    if(d->dy)
        d->dy=-dh;
    return true;
}

static void fix_dw_by_width_hint(int w, XSizeHints *hint, int *dw)
{
    if(hint->width_inc && *dw/hint->width_inc)
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
    if(hint->height_inc && *dh/hint->height_inc)
    {
        int max=MAX(hint->height_inc, hint->min_height);
        *dh=base_n_floor(*dh, hint->height_inc);
        if(h+*dh < max)
            *dh=max-h;
    }
    else
        *dh=0;
}

static void update_hint_win_for_move_resize(Client *c)
{
    char str[BUFSIZ];
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    long col=get_win_col(WIDGET_W(c), &hint),
         row=get_win_row(WIDGET_H(c), &hint);

    sprintf(str, "(%d, %d) %ldx%ld", WIDGET_X(c), WIDGET_Y(c), col, row);
    update_hint_win_for_info(None, str);
}

void update_win_state_for_move_resize(WM *wm, Client *c)
{
    int x=WIDGET_X(c), y=WIDGET_Y(c), w=WIDGET_W(c), h=WIDGET_H(c), left_x, top_y, max_w, max_h,
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
        update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

Pointer_act get_resize_act(Client *c, const Move_info *m)
{   // 窗口邊框寬度、標題欄調試、可調整尺寸區域的寬度、高度
    // 以及窗口框架左、右橫坐標和上、下縱坐標
    int bw=c->border_w, bh=c->titlebar_h, rw=WIDGET_W(c)/4, rh=WIDGET_H(c)/4,
        lx=WIDGET_X(c)-bw, rx=WIDGET_X(c)+WIDGET_W(c)+bw, ty=WIDGET_Y(c)-bh-bw, by=WIDGET_Y(c)+WIDGET_H(c)+bw;

    if(m->ox>=lx && m->ox<lx+bw+rw && m->oy>=ty && m->oy<ty+bw+rh)
        return TOP_LEFT_RESIZE;
    else if(m->ox>=rx-bw-rw && m->ox<rx && m->oy>=ty && m->oy<ty+bw+rh)
        return TOP_RIGHT_RESIZE;
    else if(m->ox>=lx && m->ox<lx+bw+rw && m->oy>=by-bw-rh && m->oy<by)
        return BOTTOM_LEFT_RESIZE;
    else if(m->ox>=rx-bw-rw && m->ox<rx && m->oy>=by-bw-rh && m->oy<by)
        return BOTTOM_RIGHT_RESIZE;
    else if(m->oy>=ty && m->oy<ty+bw+rh)
        return TOP_RESIZE;
    else if(m->oy>=by-bw-rh && m->oy<by)
        return BOTTOM_RESIZE;
    else if(m->ox>=lx && m->ox<lx+bw+rw)
        return LEFT_RESIZE;
    else if(m->ox>=rx-bw-rw && m->ox<rx)
        return RIGHT_RESIZE;
    else
        return NO_OP;
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
        XResizeWindow(xinfo.display, WIDGET_WIN(c->frame), WIDGET_W(c), c->titlebar_h);
    else if(!shade)
        XResizeWindow(xinfo.display, WIDGET_WIN(c->frame), WIDGET_W(c), c->titlebar_h+WIDGET_H(c));
    c->win_state.shaded=shade;
}
