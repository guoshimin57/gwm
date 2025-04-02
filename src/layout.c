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
#include "desktop.h"

static void set_stack_layout(void);
static void set_tile_layout(WM *wm);
static void fix_place_for_tile(void);
static void set_wins_rect_for_tiling(WM *wm);
static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh);
static void update_titlebars_layout(void);
static void fix_wins_rect(void);
static void fix_win_rect(Client *c);
static bool should_fix_win_rect(Client *c);
static void fix_transient_win_size(Client *c);
static void fix_transient_win_pos(Client *c);
static void fix_win_rect_by_hint(Client *c);
static void fix_win_pos_by_hint(Client *c, const XSizeHints *hint);
static void fix_win_rect_by_workarea(Client *c);
static void fix_win_size_by_workarea(Client *c, const Rect *workarea);
static void fix_win_pos_by_workarea(Client *c, const Rect *workarea);
static void fix_dialog_win_rect(Client *c, const Rect *workarea);
static void fix_win_rect_by_frame(Client *c);
static bool change_layout_ratio(WM *wm, int ox, int nx);
static Layout get_layout(void);
static void set_layout(Layout layout);
static int get_main_area_n(void);
static void set_main_area_n(int n);
static void adjust_main_area(WM *wm, double change_ratio);
static void adjust_fixed_area(WM *wm, double change_ratio);
static double get_main_area_ratio(void);
static void set_main_area_ratio(double ratio);
static double get_fixed_area_ratio(void);
static void set_fixed_area_ratio(double ratio);

static int main_area_ns[DESKTOP_N]; // 主區域可容納的客戶窗口數量
static Layout layouts[DESKTOP_N]; // 爲當前布局模式
static double main_area_ratios[DESKTOP_N], fixed_area_ratios[DESKTOP_N]; // 分別爲主要和固定區域與工作區寬度的比值

void update_layout(WM *wm)
{
    if(clients_is_empty())
        return;

    switch(get_layout())
    {
        case STACK: set_stack_layout(); break;
        case TILE: set_tile_layout(wm); break;
    }
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            move_resize_client(c, NULL);
}

static void set_stack_layout(void)
{
    fix_wins_rect();
}

static void set_tile_layout(WM *wm)
{
    fix_place_for_tile();
    set_wins_rect_for_tiling(wm);
    fix_wins_rect();
}

static void fix_place_for_tile(void)
{
    int n=0, m=get_main_area_n();
    clients_for_each(c)
    {
        if(is_on_cur_desktop(c->desktop_mask) && !is_iconic_client(c) && !c->owner)
        {
            if(c->place==MAIN_AREA && ++n>m)
                c->place=SECOND_AREA;
            else if(c->place==SECOND_AREA && n<m)
                c->place=MAIN_AREA, n++;
        }
    }
}

/* 平鋪布局模式中需要平鋪的窗口的空間布置如下：
 *     1、屏幕從左至右分別布置次要區域、主要區域、固定區域；
 *     2、同一區域內的窗口均分本區域空間（末尾窗口取餘量），
 *        若任務欄在下方，則窗口間隔設置在前窗尾部，否則設置在前窗開頭；
 *     3、在次要區域內設置其與主區域的窗口間隔；
 *     4、在固定區域內設置其與主區域的窗口間隔。 */
static void set_wins_rect_for_tiling(WM *wm)
{
    int i=0, j=0, k=0, mw, sw, fw, mh, sh, fh, g=cfg->win_gap, 
        wx=wm->workarea.x, wy=wm->workarea.y, wh=wm->workarea.h;
    int y_offset = cfg->show_taskbar && cfg->taskbar_on_top ? g : 0;

    get_area_size(wm, &mw, &mh, &sw, &sh, &fw, &fh);
    clients_for_each(c)
    {
        if(is_tile_client(c))
        {
            int x=0, y=0, w=0, h=0;
            Place place=c->place;
            if(place == FIXED_AREA)
                x=wx+mw+sw+g, y=wy+i++*fh+y_offset, w=fw-g, h=fh-g;
            else if(place == MAIN_AREA)
                x=wx+sw, y=wy+j++*mh+y_offset, w=mw, h=mh-g;
            else if(place == SECOND_AREA)
                x=wx, y=wy+k++*sh+y_offset, w=sw-g, h=sh-g;
            if(is_spec_place_last_client(c, place)) // 區末窗口取餘量
                h+=wh%(h+g);
            set_client_rect_by_outline(c, x, y, w, h);
        }
    }
}

static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh)
{
    double mr=get_main_area_ratio(), fr=get_fixed_area_ratio();
    int n1, n2, n3, ww=wm->workarea.w, wh=wm->workarea.h;

    n1=get_clients_n(MAIN_AREA, false, false, false),
    n2=get_clients_n(SECOND_AREA, false, false, false),
    n3=get_clients_n(FIXED_AREA, false, false, false),
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
        if(c->decorative && is_on_cur_desktop(c->desktop_mask))
            titlebar_update_layout(c->frame);
}

static void fix_wins_rect(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            fix_win_rect(c);
}

static void fix_win_rect(Client *c)
{
    if(!should_fix_win_rect(c))
        return;

    fix_transient_win_size(c);
    fix_win_rect_by_hint(c);
    fix_transient_win_pos(c);
    fix_win_rect_by_workarea(c);
    fix_win_rect_by_frame(c);
}

static bool should_fix_win_rect(Client *c)
{
    Place p=c->place;
    if(p==FULLSCREEN_LAYER || p==DOCK_LAYER || p==DESKTOP_LAYER)
        return false;

    switch(get_layout())
    {
        case STACK: return is_new_client(c);
        case TILE:  return c->owner || (is_new_client(c) && !is_normal_layer(p));
        default:    return false;
    }
}

static void fix_transient_win_size(Client *c)
{
    if(c->owner)
    {
        WIDGET_W(c)=WIDGET_W(c->owner)/2;
        WIDGET_H(c)=WIDGET_H(c->owner)/2;
    }
}

static void fix_transient_win_pos(Client *c)
{
    if(c->owner)
    {
        WIDGET_X(c)=WIDGET_X(c->owner)+(WIDGET_W(c->owner)-WIDGET_W(c))/2;
        WIDGET_Y(c)=WIDGET_Y(c->owner)+(WIDGET_H(c->owner)-WIDGET_H(c))/2;
    }
}

static void fix_win_rect_by_hint(Client *c)
{
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    fix_win_size_by_hint(&hint, &WIDGET_W(c), &WIDGET_H(c));
    fix_win_pos_by_hint(c, &hint);
}

static void fix_win_pos_by_hint(Client *c, const XSizeHints *hint)
{
    if(!c->owner && ((hint->flags & USPosition) || (hint->flags & PPosition)))
        WIDGET_X(c)=hint->x, WIDGET_Y(c)=hint->y;
}

static void fix_win_rect_by_workarea(Client *c)
{
    if(c->win_type.desktop || c->win_type.dock || c->win_state.fullscreen)
        return;

    Rect r;
    get_net_workarea(&r.x, &r.y, &r.w, &r.h);
    fix_win_size_by_workarea(c, &r);
    fix_win_pos_by_workarea(c, &r);
    fix_dialog_win_rect(c, &r);
}

static void fix_win_size_by_workarea(Client *c, const Rect *workarea)
{
    if(WIDGET_W(c) > workarea->w)
        WIDGET_W(c)=workarea->w;
    if(WIDGET_H(c) > workarea->h)
        WIDGET_H(c)=workarea->h;
}

static void fix_win_pos_by_workarea(Client *c, const Rect *workarea)
{
    int w=WIDGET_W(c), h=WIDGET_H(c),
        wx=workarea->x, wy=workarea->y, ww=workarea->w, wh=workarea->h;
    if(WIDGET_X(c) >= wx+ww-w) // 窗口在工作區右邊出界
        WIDGET_X(c)=wx+ww-w;
    if(WIDGET_X(c) < wx) // 窗口在工作區左邊出界
        WIDGET_X(c)=wx;
    if(WIDGET_Y(c) >= wy+wh-h) // 窗口在工作區下邊出界
        WIDGET_Y(c)=wy+wh-h;
    if(WIDGET_Y(c) < wy) // 窗口在工作區上邊出界
        WIDGET_Y(c)=wy;
}

static void fix_dialog_win_rect(Client *c, const Rect *workarea)
{
    if(!c->owner && c->win_type.dialog)
    {
        WIDGET_W(c)=workarea->w/2;
        WIDGET_H(c)=workarea->h/2;
        WIDGET_X(c)=workarea->x+(workarea->w-WIDGET_W(c))/2;
        WIDGET_Y(c)=workarea->y+(workarea->h-WIDGET_H(c))/2;
    }
}

static void fix_win_rect_by_frame(Client *c)
{
    if(!c->frame)
        return;

    int bw = WIDGET_BORDER_W(c->frame),
        th = frame_get_titlebar_height(c->frame);
    WIDGET_X(c) += bw;
    WIDGET_Y(c) += bw+th;
    WIDGET_H(c) -= th;
}

bool is_main_sec_gap(WM *wm, int x)
{
    long sw=wm->workarea.w*(1-get_main_area_ratio()-get_fixed_area_ratio()),
         wx=wm->workarea.x,
         n=get_clients_n(SECOND_AREA, false, false, false);
    return (n && x>=wx+sw-cfg->win_gap && x<wx+sw);
}

bool is_main_fix_gap(WM *wm, int x)
{
    long smw=wm->workarea.w*(1-get_fixed_area_ratio()),
         wx=wm->workarea.x,
         n=get_clients_n(FIXED_AREA, false, false, false);
    return (n && x>=wx+smw && x<wx+smw+cfg->win_gap);
}

bool is_layout_adjust_area(WM *wm, Window win, int x)
{
    return (get_layout()==TILE && win==xinfo.root_win
        && (is_main_sec_gap(wm, x) || is_main_fix_gap(wm, x)));
}

void change_to_stack(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    change_layout(wm, STACK);
}

void change_to_tile(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    change_layout(wm, TILE);
}

void change_layout(WM *wm, Layout layout)
{
    Layout cl=get_layout();

    if(layout == cl)
        return;

    if(cl==TILE && layout==STACK)
        clients_for_each(c)
            if(is_on_cur_desktop(c->desktop_mask) && is_normal_layer(c->place))
                c->place=NORMAL_LAYER;

    if(cl==STACK && layout==TILE)
        clients_for_each(c)
            if(is_on_cur_desktop(c->desktop_mask) && c->place==NORMAL_LAYER)
                c->place=MAIN_AREA;

    set_layout(layout);
    set_gwm_layout(layout);
    request_layout_update();
    update_titlebars_layout();
    taskbar_buttons_update_bg(wm->taskbar);
}

void adjust_layout_ratio(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(arg);
    if( get_layout()!=TILE
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
            wm->event_handler(wm, &ev);
    }while(!is_match_button_release(e, &ev));
    XUngrabPointer(xinfo.display, CurrentTime);
}

static bool change_layout_ratio(WM *wm, int ox, int nx)
{
    double dr, m=get_main_area_ratio(), f=get_fixed_area_ratio();
    dr=1.0*(nx-ox)/wm->workarea.w;
    if(is_main_sec_gap(wm, ox))
        set_main_area_ratio(m-dr);
    else if(is_main_fix_gap(wm, ox))
        set_main_area_ratio(m+dr), set_fixed_area_ratio(f-dr);
    else
        return false;
    return true;
}

void key_increase_main_area(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    adjust_main_area(wm, (double)cfg->resize_inc/wm->workarea.w);
}

void key_decrease_main_area(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    adjust_main_area(wm, -(double)cfg->resize_inc/wm->workarea.w);
}

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
static void adjust_main_area(WM *wm, double change_ratio)
{
    if( get_layout()==TILE
        && get_clients_n(SECOND_AREA, false, false, false))
    {
        double mr=get_main_area_ratio()+change_ratio, fr=get_fixed_area_ratio();
        long mw=mr*wm->workarea.w, sw=wm->workarea.w*(1-fr)-mw;
        if(sw>=cfg->resize_inc && mw>=cfg->resize_inc)
        {
            set_main_area_ratio(mr);
            request_layout_update();
        }
    }
}

void key_increase_fixed_area(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    adjust_fixed_area(wm, (double)cfg->resize_inc/wm->workarea.w);
}

void key_decrease_fixed_area(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    adjust_fixed_area(wm, -(double)cfg->resize_inc/wm->workarea.w);
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
static void adjust_fixed_area(WM *wm, double change_ratio)
{
    if( get_layout()==TILE
        && get_clients_n(FIXED_AREA, false, false, false))
    {
        double fr=get_fixed_area_ratio()+change_ratio, mr=get_main_area_ratio();
        long mw=wm->workarea.w*(mr-change_ratio), fw=wm->workarea.w*fr;
        if(mw>=cfg->resize_inc && fw>=cfg->resize_inc)
        {
            set_main_area_ratio(mr-change_ratio), set_fixed_area_ratio(fr);
            request_layout_update();
        }
    }
}

void init_layout(void)
{
    for(size_t i=0; i<DESKTOP_N; i++)
    {
        main_area_ns[i]=cfg->default_main_area_n;
        layouts[i]=cfg->default_layout;
        main_area_ratios[i]=cfg->default_main_area_ratio;
        fixed_area_ratios[i]=cfg->default_fixed_area_ratio;
    }
    set_gwm_layout(get_layout());
}

static Layout get_layout(void)
{
    return layouts[get_net_current_desktop()];
}

static void set_layout(Layout layout)
{
    layouts[get_net_current_desktop()]=layout;
}

static int get_main_area_n(void)
{
    return main_area_ns[get_net_current_desktop()];
}

static void set_main_area_n(int n)
{
    main_area_ns[get_net_current_desktop()]=n;
}

static double get_main_area_ratio(void)
{
    return main_area_ratios[get_net_current_desktop()];
}

static void set_main_area_ratio(double ratio)
{
    main_area_ratios[get_net_current_desktop()]=ratio;
}

static double get_fixed_area_ratio(void)
{
    return fixed_area_ratios[get_net_current_desktop()];
}

static void set_fixed_area_ratio(double ratio)
{
    fixed_area_ratios[get_net_current_desktop()]=ratio;
}

void adjust_main_area_n(int n)
{
    if(get_layout() == TILE)
    {
        int m=get_main_area_n();
        set_main_area_n(m+n>=1 ? m+n : 1);
        request_layout_update();
    }
}

bool is_spec_layout(Layout layout)
{
    return layout == get_layout();
}
