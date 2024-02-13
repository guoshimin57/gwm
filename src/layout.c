/* *************************************************************************
 *     layout.c：實現與窗口布局相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void fix_win_rect(WM *wm, Client *c);
static void fix_win_size(WM *wm, Client *c, const XSizeHints *hint);
static void fix_win_size_by_workarea(WM *wm, Client *c);
static void fix_win_pos(WM *wm, Client *c, const XSizeHints *hint);
static bool fix_win_pos_by_hint(Client *c, const XSizeHints *hint);
static void fix_win_pos_by_prop(WM *wm, Client *c);
static void set_transient_win_pos(Client *c);
static void fix_win_pos_by_workarea(WM *wm, Client *c);
static void set_preview_layout(WM *wm);
static void set_rect_of_main_win_for_preview(WM *wm);
static void set_stack_layout(WM *wm);
static void set_tile_layout(WM *wm);
static void set_rect_of_tile_win_for_tiling(WM *wm);
static void set_rect_of_transient_win_for_tiling(WM *wm);
static void set_rect_of_float_win_for_tiling(WM *wm);
static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh);
static bool change_layout_ratio(WM *wm, int ox, int nx);

void update_layout(WM *wm)
{
    if(wm->clients == wm->clients->next)
        return;

    fix_place_type_for_tile(wm);
    switch(DESKTOP(wm)->cur_layout)
    {
        case PREVIEW: set_preview_layout(wm); break;
        case STACK: set_stack_layout(wm); break;
        case TILE: set_tile_layout(wm); break;
    }
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            move_resize_client(wm, c, NULL);
}

static void fix_win_rect(WM *wm, Client *c)
{
    XSizeHints hint=get_size_hint(c->win);
    fix_win_size(wm, c, &hint);
    fix_win_pos(wm, c, &hint);
}

static void fix_win_size(WM *wm, Client *c, const XSizeHints *hint)
{
    fix_win_size_by_hint(hint, &c->w, &c->h);
    fix_win_size_by_workarea(wm, c);
}

static void fix_win_size_by_workarea(WM *wm, Client *c)
{
    if(!c->win_type.normal)
        return;

    long ww=wm->workarea.w, wh=wm->workarea.h, bh=c->titlebar_h, bw=c->border_w;
    if(c->w+2*bw > ww)
        c->w=ww-2*bw;
    if(c->h+bh+2*bw > wh)
        c->h=wh-bh-2*bw;
}

static void fix_win_pos(WM *wm, Client *c, const XSizeHints *hint)
{
    if(!fix_win_pos_by_hint(c, hint))
        fix_win_pos_by_prop(wm, c), fix_win_pos_by_workarea(wm, c);
}

static bool fix_win_pos_by_hint(Client *c, const XSizeHints *hint)
{
    if(!c->owner && ((hint->flags & USPosition) || (hint->flags & PPosition)))
    {
        c->x=hint->x+c->border_w, c->y=hint->y+c->border_w+c->titlebar_h;
        return true;
    }
    return false;
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
    if(!c->win_type.normal)
        return;

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

static void set_preview_layout(WM *wm)
{
    set_rect_of_main_win_for_preview(wm);
    set_rect_of_transient_win_for_tiling(wm);
}

static void set_rect_of_main_win_for_preview(WM *wm)
{
    int n=get_clients_n(wm, ANY_PLACE, true, false, false);
    if(n == 0)
        return;

    int rows, cols, w, h, g=cfg->win_gap, wx=wm->workarea.x,
        wy=wm->workarea.y, ww=wm->workarea.w, wh=wm->workarea.h;
    Rect frame;

    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=n && cols*cols<n; cols++)
        ;
    rows = (cols-1)*cols>=n ? cols-1 : cols;
    w=ww/cols, h=wh/rows;

    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(is_on_cur_desktop(wm, c) && !c->owner)
        {
            n--;
            frame=(Rect){wx+(n%cols)*w+g, wy+(n/cols)*h+g, w-2*g, h-2*g};
            set_win_rect_by_frame(c, &frame);
        }
    }
}

static void set_stack_layout(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && !c->icon
            && !is_win_state_max(c) && !c->win_state.fullscreen)
            fix_win_rect(wm, c);
}

static void set_tile_layout(WM *wm)
{
    set_rect_of_tile_win_for_tiling(wm);
    set_rect_of_transient_win_for_tiling(wm);
    set_rect_of_float_win_for_tiling(wm);
}

/* 平鋪布局模式中需要平鋪的窗口的空間布置如下：
 *     1、屏幕從左至右分別布置次要區域、主要區域、固定區域；
 *     2、同一區域內的窗口均分本區域空間（末尾窗口取餘量），窗口間隔設置在前窗尾部；
 *     3、在次要區域內設置其與主區域的窗口間隔；
 *     4、在固定區域內設置其與主區域的窗口間隔。 */
static void set_rect_of_tile_win_for_tiling(WM *wm)
{
    int i=0, j=0, k=0, mw, sw, fw, mh, sh, fh, g=cfg->win_gap,
        wx=wm->workarea.x, wy=wm->workarea.y, wh=wm->workarea.h;
    Rect frame;

    get_area_size(wm, &mw, &mh, &sw, &sh, &fw, &fh);
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_tile_client(wm, c))
        {
            Place_type type=c->place_type;
            if(type == TILE_LAYER_FIXED)
                frame=(Rect){wx+mw+sw+g, wy+i++*fh, fw-g, fh-g};
            else if(type == TILE_LAYER_MAIN)
                frame=(Rect){wx+sw, wy+j++*mh, mw, mh-g};
            else if(type == TILE_LAYER_SECOND)
                frame=(Rect){wx, wy+k++*sh, sw-g, sh-g};
            if(is_last_typed_client(wm, c, type)) // 區末窗口取餘量
                frame.h+=wh%(frame.h+g)+g;
            set_win_rect_by_frame(c, &frame);
        }
    }
}

/* 平鋪布局模式的非臨時窗口位於其主窗之上並居中 */
static void set_rect_of_transient_win_for_tiling(WM *wm)
{
    XSizeHints hint;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c) && c->owner)
            hint=get_size_hint(c->win), fix_win_pos(wm, c, &hint);
}

static void set_rect_of_float_win_for_tiling(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c) && c->place_type==FLOAT_LAYER)
            fix_win_rect(wm, c);
}

static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh)
{
    double mr=DESKTOP(wm)->main_area_ratio, fr=DESKTOP(wm)->fixed_area_ratio;
    int n1, n2, n3, ww=wm->workarea.w, wh=wm->workarea.h;

    n1=get_clients_n(wm, TILE_LAYER_MAIN, false, false, false),
    n2=get_clients_n(wm, TILE_LAYER_SECOND, false, false, false),
    n3=get_clients_n(wm, TILE_LAYER_FIXED, false, false, false),
    *mw=mr*ww, *fw=ww*fr, *sw=ww-*fw-*mw;
    *mh = n1 ? wh/n1 : wh, *fh = n3 ? wh/n3 : wh, *sh = n2 ? wh/n2 : wh;
    if(n3 == 0)
        *mw+=*fw, *fw=0;
    if(n2 == 0)
        *mw+=*sw, *sw=0;
}

void update_titlebar_layout(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(c->titlebar_h && is_on_cur_desktop(wm, c))
        {
            Rect r=get_title_area_rect(wm, c);
            XResizeWindow(xinfo.display, c->title_area, r.w, r.h);
        }
    }
}

bool is_main_sec_gap(WM *wm, int x)
{
    Desktop *d=DESKTOP(wm);
    long sw=wm->workarea.w*(1-d->main_area_ratio-d->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(wm, TILE_LAYER_SECOND, false, false, false);
    return (n && x>=wx+sw-cfg->win_gap && x<wx+sw);
}

bool is_main_fix_gap(WM *wm, int x)
{
    long smw=wm->workarea.w*(1-DESKTOP(wm)->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(wm, TILE_LAYER_FIXED, false, false, false);
    return (n && x>=wx+smw && x<wx+smw+cfg->win_gap);
}

bool is_layout_adjust_area(WM *wm, Window win, int x)
{
    return (DESKTOP(wm)->cur_layout==TILE && win==xinfo.root_win
        && (is_main_sec_gap(wm, x) || is_main_fix_gap(wm, x)));
}

void change_to_spec_layout(WM *wm, Layout layout)
{
    Layout *cl=&DESKTOP(wm)->cur_layout, *pl=&DESKTOP(wm)->prev_layout;

    if(layout == *cl)
        return;

    if(layout == PREVIEW)
        save_place_info_of_clients(wm);
    if(*cl == PREVIEW)
        restore_place_info_of_clients(wm);

    Display *d=xinfo.display;
    if(*cl == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->icon)
                XMapWindow(d, c->icon->win), XUnmapWindow(d, c->frame);
    if(layout == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->icon)
                XMapWindow(d, c->frame), XUnmapWindow(d, c->icon->win);

    if(*cl==TILE && layout==STACK)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && is_normal_layer(c->place_type))
                c->place_type=FLOAT_LAYER;

    if(*cl==STACK && layout==TILE)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->place_type==FLOAT_LAYER)
                c->place_type=TILE_LAYER_MAIN;

    *pl=*cl, *cl=layout;
    request_layout_update();
    update_titlebar_layout(wm);
    update_taskbar_buttons_bg();
    set_gwm_current_layout(*cl);
}

void pointer_adjust_layout_ratio(WM *wm, XEvent *e)
{
    if( DESKTOP(wm)->cur_layout!=TILE
        || !is_layout_adjust_area(wm, e->xbutton.window, e->xbutton.x_root)
        || !grab_pointer(xinfo.root_win, ADJUST_LAYOUT_RATIO))
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
                request_layout_update(), ox=nx;
        }
        else
            wm->event_handlers[ev.type](wm, &ev);
    }while(!is_match_button_release(e, &ev));
    XUngrabPointer(xinfo.display, CurrentTime);
}

static bool change_layout_ratio(WM *wm, int ox, int nx)
{
    Desktop *d=DESKTOP(wm);
    double dr;
    dr=1.0*(nx-ox)/wm->workarea.w;
    if(is_main_sec_gap(wm, ox))
        d->main_area_ratio-=dr;
    else if(is_main_fix_gap(wm, ox))
        d->main_area_ratio+=dr, d->fixed_area_ratio-=dr;
    else
        return false;
    return true;
}

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
void key_adjust_main_area_ratio(WM *wm, double change_ratio)
{
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm, TILE_LAYER_SECOND, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double mr=d->main_area_ratio+change_ratio, fr=d->fixed_area_ratio;
        long mw=mr*wm->workarea.w, sw=wm->workarea.w*(1-fr)-mw;
        if(sw>=cfg->resize_inc && mw>=cfg->resize_inc)
        {
            d->main_area_ratio=mr;
            request_layout_update();
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void key_adjust_fixed_area_ratio(WM *wm, double change_ratio)
{ 
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm, TILE_LAYER_FIXED, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double fr=d->fixed_area_ratio+change_ratio, mr=d->main_area_ratio;
        long mw=wm->workarea.w*(mr-change_ratio), fw=wm->workarea.w*fr;
        if(mw>=cfg->resize_inc && fw>=cfg->resize_inc)
        {
            d->main_area_ratio-=change_ratio, d->fixed_area_ratio=fr;
            request_layout_update();
        }
    }
}
