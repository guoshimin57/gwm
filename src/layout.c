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
static void fix_place_type_for_tile(WM *wm);
static void set_rect_of_tile_win_for_tiling(WM *wm);
static void set_rect_of_transient_win_for_tiling(WM *wm);
static void set_rect_of_float_win_for_tiling(WM *wm);
static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh);
static void update_titlebars_layout(WM *wm);
static bool change_layout_ratio(WM *wm, int ox, int nx);

void update_layout(WM *wm)
{
    if(wm->clients == wm->clients->next)
        return;

    switch(DESKTOP(wm)->cur_layout)
    {
        case PREVIEW: set_preview_layout(wm); break;
        case STACK: set_stack_layout(wm); break;
        case TILE: set_tile_layout(wm); break;
    }
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(c->desktop_mask))
            move_resize_client(c, NULL);
}

static void fix_win_rect(WM *wm, Client *c)
{
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    fix_win_size(wm, c, &hint);
    fix_win_pos(wm, c, &hint);
}

static void fix_win_size(WM *wm, Client *c, const XSizeHints *hint)
{
    fix_win_size_by_hint(hint, &WIDGET_W(c), &WIDGET_H(c));
    fix_win_size_by_workarea(wm, c);
}

static void fix_win_size_by_workarea(WM *wm, Client *c)
{
    if(!c->win_type.normal)
        return;

    int ww=wm->workarea.w, wh=wm->workarea.h, bw=WIDGET_BORDER_W(c->frame);
    if(WIDGET_W(c->frame)+2*bw > ww)
        WIDGET_W(c->frame)=ww-2*bw;
    if(WIDGET_H(c->frame)+2*bw > wh)
        WIDGET_H(c->frame)=wh-2*bw;
    Rect r=get_win_rect_by_frame(c->frame);
    WIDGET_W(c)=r.w, WIDGET_H(c)=r.h;
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
        WIDGET_X(c->frame)=hint->x, WIDGET_Y(c->frame)=hint->y;
        Rect r=get_win_rect_by_frame(c->frame);
        WIDGET_X(c)=r.x, WIDGET_Y(c)=r.y;
        return true;
    }
    return false;
}

static void fix_win_pos_by_prop(WM *wm, Client *c)
{
    if(c->owner)
        set_transient_win_pos(c);
    else if(c->win_type.dialog)
        WIDGET_X(c)=wm->workarea.x+(wm->workarea.w-WIDGET_W(c))/2,
        WIDGET_Y(c)=wm->workarea.y+(wm->workarea.h-WIDGET_H(c))/2;
}

static void set_transient_win_pos(Client *c)
{
    WIDGET_X(c)=WIDGET_X(c->owner)+(WIDGET_W(c->owner)-WIDGET_W(c))/2;
    WIDGET_Y(c)=WIDGET_Y(c->owner)+(WIDGET_H(c->owner)-WIDGET_H(c))/2;
}

static void fix_win_pos_by_workarea(WM *wm, Client *c)
{
    if(!c->win_type.normal)
        return;

    int w=WIDGET_W(c->frame), h=WIDGET_H(c->frame),
        bw=WIDGET_BORDER_W(c->frame), wx=wm->workarea.x, wy=wm->workarea.y,
        ww=wm->workarea.w, wh=wm->workarea.h;
    if(WIDGET_X(c->frame) >= wx+ww-w-2*bw) // 窗口在工作區右邊出界
        WIDGET_X(c->frame)=wx+ww-w-2*bw;
    if(WIDGET_X(c->frame) < wx) // 窗口在工作區左邊出界
        WIDGET_X(c->frame)=wx;
    if(WIDGET_Y(c->frame) >= wy+wh-h-2*bw) // 窗口在工作區下邊出界
        WIDGET_Y(c->frame)=wy+wh-h-2*bw;
    if(WIDGET_Y(c->frame) < wy) // 窗口在工作區上邊出界
        WIDGET_Y(c->frame)=wy;
    Rect r=get_win_rect_by_frame(c->frame);
    WIDGET_X(c)=r.x, WIDGET_Y(c)=r.y;
}

static void set_preview_layout(WM *wm)
{
    set_rect_of_main_win_for_preview(wm);
    set_rect_of_transient_win_for_tiling(wm);
}

static void set_rect_of_main_win_for_preview(WM *wm)
{
    int n=get_clients_n(wm->clients, ANY_PLACE, true, false, false);
    if(n == 0)
        return;

    int rows, cols, w, h, wx=wm->workarea.x, wy=wm->workarea.y,
        ww=wm->workarea.w, wh=wm->workarea.h;

    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=n && cols*cols<n; cols++)
        ;
    rows = (cols-1)*cols>=n ? cols-1 : cols;
    w=ww/cols, h=wh/rows;

    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(c->desktop_mask) && !c->owner && (n--)>=0)
            set_client_rect_by_outline(c, wx+(n%cols)*w, wy+(n/cols)*h, w, h);
}

static void set_stack_layout(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_on_cur_desktop(c->desktop_mask) && !is_iconic_client(c))
        {
            if(is_win_state_max(c) || c->win_state.fullscreen)
                fix_win_rect_by_state(wm, c);
            else
                fix_win_rect(wm, c);
        }
    }
}

static void set_tile_layout(WM *wm)
{
    fix_place_type_for_tile(wm);
    set_rect_of_tile_win_for_tiling(wm);
    set_rect_of_transient_win_for_tiling(wm);
    set_rect_of_float_win_for_tiling(wm);
}

static void fix_place_type_for_tile(WM *wm)
{
    int n=0, m=DESKTOP(wm)->n_main_max;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_on_cur_desktop(c->desktop_mask) && !is_iconic_client(c) && !c->owner)
        {
            if(c->place_type==TILE_LAYER_MAIN && ++n>m)
                c->place_type=TILE_LAYER_SECOND;
            else if(c->place_type==TILE_LAYER_SECOND && n<m)
                c->place_type=TILE_LAYER_MAIN, n++;
        }
    }
}

/* 平鋪布局模式中需要平鋪的窗口的空間布置如下：
 *     1、屏幕從左至右分別布置次要區域、主要區域、固定區域；
 *     2、同一區域內的窗口均分本區域空間（末尾窗口取餘量），
 *        若任務欄在下方，則窗口間隔設置在前窗尾部，否則設置在前窗開頭；
 *     3、在次要區域內設置其與主區域的窗口間隔；
 *     4、在固定區域內設置其與主區域的窗口間隔。 */
static void set_rect_of_tile_win_for_tiling(WM *wm)
{
    int i=0, j=0, k=0, mw, sw, fw, mh, sh, fh, g=cfg->win_gap, 
        wx=wm->workarea.x, wy=wm->workarea.y, wh=wm->workarea.h;
    int y_offset = cfg->show_taskbar && cfg->taskbar_on_top ? g : 0;

    get_area_size(wm, &mw, &mh, &sw, &sh, &fw, &fh);
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(is_tile_client(c))
        {
            int x, y, w, h;
            Place_type type=c->place_type;
            if(type == TILE_LAYER_FIXED)
                x=wx+mw+sw+g, y=wy+i++*fh+y_offset, w=fw-g, h=fh-g;
            else if(type == TILE_LAYER_MAIN)
                x=wx+sw, y=wy+j++*mh+y_offset, w=mw, h=mh-g;
            else if(type == TILE_LAYER_SECOND)
                x=wx, y=wy+k++*sh+y_offset, w=sw-g, h=sh-g;
            if(is_last_typed_client(wm->clients, c, type)) // 區末窗口取餘量
                h+=wh%(h+g);
            set_client_rect_by_outline(c, x, y, w, h);
        }
    }
}

/* 平鋪布局模式的非臨時窗口位於其主窗之上並居中 */
static void set_rect_of_transient_win_for_tiling(WM *wm)
{
    XSizeHints hint;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(c->desktop_mask) && c->owner)
            hint=get_size_hint(WIDGET_WIN(c)), fix_win_pos(wm, c, &hint);
}

static void set_rect_of_float_win_for_tiling(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(c->desktop_mask) && c->place_type==FLOAT_LAYER)
            fix_win_rect(wm, c);
}

static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh)
{
    double mr=DESKTOP(wm)->main_area_ratio, fr=DESKTOP(wm)->fixed_area_ratio;
    int n1, n2, n3, ww=wm->workarea.w, wh=wm->workarea.h;

    n1=get_clients_n(wm->clients, TILE_LAYER_MAIN, false, false, false),
    n2=get_clients_n(wm->clients, TILE_LAYER_SECOND, false, false, false),
    n3=get_clients_n(wm->clients, TILE_LAYER_FIXED, false, false, false),
    *mw=mr*ww, *fw=ww*fr, *sw=ww-*fw-*mw;
    *mh = n1 ? wh/n1 : wh, *fh = n3 ? wh/n3 : wh, *sh = n2 ? wh/n2 : wh;
    if(n3 == 0)
        *mw+=*fw, *fw=0;
    if(n2 == 0)
        *mw+=*sw, *sw=0;
}

static void update_titlebars_layout(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(c->show_titlebar && is_on_cur_desktop(c->desktop_mask))
            update_titlebar_layout(c->frame);
}

bool is_main_sec_gap(WM *wm, int x)
{
    Desktop *d=DESKTOP(wm);
    long sw=wm->workarea.w*(1-d->main_area_ratio-d->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(wm->clients, TILE_LAYER_SECOND, false, false, false);
    return (n && x>=wx+sw-cfg->win_gap && x<wx+sw);
}

bool is_main_fix_gap(WM *wm, int x)
{
    long smw=wm->workarea.w*(1-DESKTOP(wm)->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(wm->clients, TILE_LAYER_FIXED, false, false, false);
    return (n && x>=wx+smw && x<wx+smw+cfg->win_gap);
}

bool is_layout_adjust_area(WM *wm, Window win, int x)
{
    return (DESKTOP(wm)->cur_layout==TILE && win==xinfo.root_win
        && (is_main_sec_gap(wm, x) || is_main_fix_gap(wm, x)));
}

void change_layout(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Layout *cl=&DESKTOP(wm)->cur_layout, *pl=&DESKTOP(wm)->prev_layout;

    if(arg.layout == *cl)
        return;

    if(arg.layout == PREVIEW)
        save_place_info_of_clients(wm->clients);
    if(*cl == PREVIEW)
        restore_place_info_of_clients(wm->clients);

    if(*cl == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            {
                update_net_wm_state(WIDGET_WIN(c), c->win_state);
                hide_widget(WIDGET(c->frame));
            }
    if(arg.layout == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            {
                c->win_state.hidden=0;
                update_net_wm_state(WIDGET_WIN(c), c->win_state);
                c->win_state.hidden=1;
                show_widget(WIDGET(c->frame));
            }

    if(*cl==TILE && arg.layout==STACK)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(c->desktop_mask) && is_normal_layer(c->place_type))
                c->place_type=FLOAT_LAYER;

    if(*cl==STACK && arg.layout==TILE)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(c->desktop_mask) && c->place_type==FLOAT_LAYER)
                c->place_type=TILE_LAYER_MAIN;

    *pl=*cl, *cl=arg.layout;
    set_gwm_current_layout(*cl);
    request_layout_update();
    update_titlebars_layout(wm);
    update_taskbar_buttons_bg();
}

void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
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
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm->clients, TILE_LAYER_SECOND, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double mr=d->main_area_ratio+arg.change_ratio, fr=d->fixed_area_ratio;
        long mw=mr*wm->workarea.w, sw=wm->workarea.w*(1-fr)-mw;
        if(sw>=cfg->resize_inc && mw>=cfg->resize_inc)
        {
            d->main_area_ratio=mr;
            request_layout_update();
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm->clients, TILE_LAYER_FIXED, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double fr=d->fixed_area_ratio+arg.change_ratio, mr=d->main_area_ratio;
        long mw=wm->workarea.w*(mr-arg.change_ratio), fw=wm->workarea.w*fr;
        if(mw>=cfg->resize_inc && fw>=cfg->resize_inc)
        {
            d->main_area_ratio-=arg.change_ratio, d->fixed_area_ratio=fr;
            request_layout_update();
        }
    }
}
