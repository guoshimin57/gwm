/* *************************************************************************
 *     layout.c：實現與窗口布局相關的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "layout.h"
#include "prop.h"
#include "icccm.h"
#include "ewmh.h"
#include "minimax.h"
#include "taskbar.h"

static void set_preview_layout(WM *wm);
static void set_rect_of_main_win_for_preview(WM *wm);
static void set_stack_layout(void);
static void set_tile_layout(WM *wm);
static void fix_place_type_for_tile(WM *wm);
static void set_rect_of_tile_win_for_tiling(WM *wm);
static void set_rect_of_transient_win_for_tiling(void);
static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh);
static void update_titlebars_layout(void);
static bool change_layout_ratio(WM *wm, int ox, int nx);

void update_layout(WM *wm)
{
    if(clients_is_empty())
        return;

    switch(DESKTOP(wm)->cur_layout)
    {
        case PREVIEW: set_preview_layout(wm); break;
        case STACK: set_stack_layout(); break;
        case TILE: set_tile_layout(wm); break;
    }
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            move_resize_client(c, NULL);
}

static void set_preview_layout(WM *wm)
{
    set_rect_of_main_win_for_preview(wm);
    set_rect_of_transient_win_for_tiling();
}

static void set_rect_of_main_win_for_preview(WM *wm)
{
    int n=get_clients_n(ANY_PLACE, true, false, false);
    if(n == 0)
        return;

    int rows, cols, w, h, wx=wm->workarea.x, wy=wm->workarea.y,
        ww=wm->workarea.w, wh=wm->workarea.h;

    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=n && cols*cols<n; cols++)
        ;
    rows = (cols-1)*cols>=n ? cols-1 : cols;
    w=ww/cols, h=wh/rows;

    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask) && !c->owner && (n--)>=0)
            set_client_rect_by_outline(c, wx+(n%cols)*w, wy+(n/cols)*h, w, h);
}

static void set_stack_layout(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && !is_iconic_client(c))
            set_win_rect(c);
}

static void set_tile_layout(WM *wm)
{
    fix_place_type_for_tile(wm);
    set_rect_of_tile_win_for_tiling(wm);
    set_rect_of_transient_win_for_tiling();
}

static void fix_place_type_for_tile(WM *wm)
{
    int n=0, m=DESKTOP(wm)->n_main_max;
    clients_for_each(c)
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
    clients_for_each(c)
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
            if(is_last_typed_client(c, type)) // 區末窗口取餘量
                h+=wh%(h+g);
            set_client_rect_by_outline(c, x, y, w, h);
        }
    }
}

/* 平鋪布局模式的臨時窗口位於其主窗之上並居中 */
static void set_rect_of_transient_win_for_tiling(void)
{
    clients_for_each_reverse(c)
        if(is_on_cur_desktop(c->desktop_mask) && c->owner)
            set_win_rect(c);
}

static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh)
{
    double mr=DESKTOP(wm)->main_area_ratio, fr=DESKTOP(wm)->fixed_area_ratio;
    int n1, n2, n3, ww=wm->workarea.w, wh=wm->workarea.h;

    n1=get_clients_n(TILE_LAYER_MAIN, false, false, false),
    n2=get_clients_n(TILE_LAYER_SECOND, false, false, false),
    n3=get_clients_n(TILE_LAYER_FIXED, false, false, false),
    *mw=mr*ww, *fw=ww*fr, *sw=ww-*fw-*mw;
    *mh = n1 ? wh/n1 : wh, *fh = n3 ? wh/n3 : wh, *sh = n2 ? wh/n2 : wh;
    if(n3 == 0)
        *mw+=*fw, *fw=0;
    if(n2 == 0)
        *mw+=*sw, *sw=0;
}

static void update_titlebars_layout(void)
{
    clients_for_each(c)
        if(c->decorative  && is_on_cur_desktop(c->desktop_mask))
            titlebar_update_layout(c->frame);
}

bool is_main_sec_gap(WM *wm, int x)
{
    Desktop *d=DESKTOP(wm);
    long sw=wm->workarea.w*(1-d->main_area_ratio-d->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(TILE_LAYER_SECOND, false, false, false);
    return (n && x>=wx+sw-cfg->win_gap && x<wx+sw);
}

bool is_main_fix_gap(WM *wm, int x)
{
    long smw=wm->workarea.w*(1-DESKTOP(wm)->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(TILE_LAYER_FIXED, false, false, false);
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
        save_place_info_of_clients();
    if(*cl == PREVIEW)
        restore_place_info_of_clients();

    if(*cl == PREVIEW)
        clients_for_each(c)
            if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            {
                update_net_wm_state(WIDGET_WIN(c), c->win_state);
                widget_hide(WIDGET(c->frame));
            }
    if(arg.layout == PREVIEW)
        clients_for_each(c)
            if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            {
                c->win_state.hidden=0;
                update_net_wm_state(WIDGET_WIN(c), c->win_state);
                c->win_state.hidden=1;
                widget_show(WIDGET(c->frame));
            }

    if(*cl==TILE && arg.layout==STACK)
        clients_for_each(c)
            if(is_on_cur_desktop(c->desktop_mask) && is_normal_layer(c->place_type))
                c->place_type=FLOAT_LAYER;

    if(*cl==STACK && arg.layout==TILE)
        clients_for_each(c)
            if(is_on_cur_desktop(c->desktop_mask) && c->place_type==FLOAT_LAYER)
                c->place_type=TILE_LAYER_MAIN;

    *pl=*cl, *cl=arg.layout;
    set_gwm_current_layout(*cl);
    request_layout_update();
    update_titlebars_layout();
    taskbar_buttons_update_bg(wm->taskbar);
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
            wm->handle_event(wm, &ev);
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
        && get_clients_n(TILE_LAYER_SECOND, false, false, false))
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
        && get_clients_n(TILE_LAYER_FIXED, false, false, false))
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
