/* *************************************************************************
 *     mvresize.c：實現移動、調整窗口尺寸的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "clientop.h"
#include "icccm.h"
#include "sizehintwin.h"
#include "focus.h"
#include "grab.h"
#include "mvresize.h"

typedef struct /* 定位器所點擊的窗口位置每次合理移動或調整尺寸所對應的舊、新坐標信息 */
{
    int ox, oy, nx, ny; /* 分別爲舊、新坐標 */
} Move_info;

static bool fix_first_move_resize(Client *c, Delta_rect *d);
static void wait_key_release(XEvent *e);
static Delta_rect get_key_delta_rect(Client *c, Key_act act);
static void do_valid_pointer_move_resize(Client *c, Move_info *m, Pointer_act act, bool is_to_stack);
static Delta_rect get_pointer_delta_rect(const Move_info *m, Pointer_act act);
static bool get_move_resize_delta_rect(Client *c, Delta_rect *d, bool is_move, bool is_to_stack);
static bool is_prefer_move(Client *c, Delta_rect *d);
static bool fix_delta_rect(Client *c, Delta_rect *d);
static void fix_dw_by_width_hint(int w, XSizeHints *hint, int *dw);
static void fix_dh_by_height_hint(int h, XSizeHints *hint, int *dh);

void key_move_resize_client(XEvent *e, Key_act op)
{
    Client *c=get_cur_focus_client();
    bool is_move = (op==UP || op==DOWN || op==LEFT || op==RIGHT),
         is_to_stack=(c->layer==TILE_LAYER);
    Delta_rect d=get_key_delta_rect(c, op);

    if(is_to_stack)
        move_client(c, NULL, STACK_LAYER, ANY_AREA);
    if(get_move_resize_delta_rect(c, &d, is_move, is_to_stack))
    {
        move_resize_client(c, &d);
        Size_hint_win *shw=size_hint_win_new(WIDGET(c));
        widget_show(WIDGET(shw));
        wait_key_release(e);
        widget_del(WIDGET(shw));
    }
}

static bool fix_first_move_resize(Client *c, Delta_rect *d)
{
    int ow=WIDGET_W(c), oh=WIDGET_H(c), nw=ow, nh=oh;
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    fix_win_size_by_hint(&hint, &nw, &nh);
    d->dw=nw-ow;
    d->dh=nh-oh;
    return d->dw || d->dh;
}

static void wait_key_release(XEvent *e)
{
    while(1)
    {
        XEvent ev;
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|KeyReleaseMask, &ev);
        if( ev.type==KeyRelease && ev.xkey.state==e->xkey.state
            && ev.xkey.keycode==e->xkey.keycode)
            break;
        else
            event_handler(&ev);
    }
}

static Delta_rect get_key_delta_rect(Client *c, Key_act act)
{
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    int wi=hint.width_inc, hi=hint.height_inc;

    Delta_rect dr[] =
    {
        [UP]          = {  0, -hi,   0,   0},
        [DOWN]        = {  0,  hi,   0,   0},
        [LEFT]        = {-wi,   0,   0,   0},
        [RIGHT]       = { wi,   0,   0,   0},
        [FALL_WIDTH]  = {  0,   0, -wi,   0},
        [RISE_WIDTH]  = {  0,   0,  wi,   0},
        [FALL_HEIGHT] = {  0,   0,   0, -hi},
        [RISE_HEIGHT] = {  0,   0,   0,  hi},
    };
    return dr[act];
}

void pointer_move_resize_client(XEvent *e, bool resize)
{
    Move_info m={e->xbutton.x_root, e->xbutton.y_root, 0, 0};
    Client *c=get_cur_focus_client();
    Pointer_act act=(resize ? get_resize_act(c, m.ox, m.oy) : MOVE);

    if(!grab_pointer(xinfo.root_win, act))
        return;

    XSizeHints hint=get_size_hint(WIDGET_WIN(c));

    XEvent ev;
    if(act==MOVE || is_resizable(&hint))
    {
        Size_hint_win *shw=size_hint_win_new(WIDGET(c));
        widget_show(WIDGET(shw));
        do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
        {
            XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
            if(ev.type == MotionNotify)
            {
                bool is_to_stack=(c->layer==TILE_LAYER);
                if(is_to_stack)
                    move_client(c, NULL, STACK_LAYER, ANY_AREA);
                /* 因X事件是異步的，故xmotion.x和ev.xmotion.y可能不是連續變化 */
                m.nx=ev.xmotion.x, m.ny=ev.xmotion.y;
                do_valid_pointer_move_resize(c, &m, act, is_to_stack);
                size_hint_win_update(shw);
            }
            else
                event_handler(&ev);
        }while(!is_match_button_release(&e->xbutton, &ev.xbutton));
        widget_del(WIDGET(shw));
    }
    XUngrabPointer(xinfo.display, CurrentTime);
}

static void do_valid_pointer_move_resize(Client *c, Move_info *m, Pointer_act act, bool is_to_stack)
{
    Delta_rect d=get_pointer_delta_rect(m, act);
    if(!get_move_resize_delta_rect(c, &d, act==MOVE, is_to_stack))
        return;

    move_resize_client(c, &d);
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

static bool get_move_resize_delta_rect(Client *c, Delta_rect *d, bool is_move, bool is_to_stack)
{
    if(is_to_stack)
        return fix_first_move_resize(c, d);
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

Pointer_act get_resize_act(Client *c, int x, int y)
{   // 框架邊框寬度、可調整尺寸區域的寬度、高度
    // 以及框架左、右橫坐標和上、下縱坐標
    int bw=WIDGET_BORDER_W(c->frame), reszw=WIDGET_W(c)/4, reszh=WIDGET_H(c)/4,
        lx=WIDGET_X(c->frame), rx=WIDGET_X(c->frame)+WIDGET_W(c->frame)+2*bw,
        ty=WIDGET_Y(c->frame), by=WIDGET_Y(c->frame)+WIDGET_H(c->frame)+2*bw;

    if(x>=lx && x<lx+reszw && y>=ty && y<ty+reszh)
        return TOP_LEFT_RESIZE;
    else if(x>=rx-reszw && x<rx && y>=ty && y<ty+reszh)
        return TOP_RIGHT_RESIZE;
    else if(x>=lx && x<lx+reszw && y>=by-reszh && y<by)
        return BOTTOM_LEFT_RESIZE;
    else if(x>=rx-reszw && x<rx && y>=by-reszh && y<by)
        return BOTTOM_RIGHT_RESIZE;
    else if(y>=ty && y<ty+reszh)
        return TOP_RESIZE;
    else if(y>=by-reszh && y<by)
        return BOTTOM_RESIZE;
    else if(x>=lx && x<lx+reszw)
        return LEFT_RESIZE;
    else if(x>=rx-reszw && x<rx)
        return RIGHT_RESIZE;
    else
        return NO_OP;
}
