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

#include "misc.h"
#include "config.h"
#include "clientop.h"
#include "layout.h"
#include "prop.h"
#include "icccm.h"
#include "ewmh.h"
#include "grab.h"
#include "taskbar.h"
#include "desktop.h"

static void set_stack_layout(void);
static void set_tile_layout(void);
static void fix_area_for_tile(void);
static void set_wins_rect_for_tiling(void);
static void get_area_size(int *mw, int *mh, int *sw, int *sh, int *fw, int *fh);
static void update_titlebars_layout(void);
static void fix_wins_rect(void);
static void fix_win_rect(Client *c);
static bool should_fix_win_rect(Client *c);
static void fix_win_rect_by_owner(Client *c);
static void fix_win_rect_by_hint(Client *c);
static void fix_win_rect_by_workarea(Client *c);
static void fix_frame_size_by_workarea(Client *c, const Rect *workarea);
static void fix_frame_pos_by_workarea(Client *c, const Rect *workarea);
static void fix_dialog_frame_rect(Client *c, const Rect *workarea);
static void set_frame_rect_by_client(Client *c);
static bool change_layout_ratio(int ox, int nx);
static Layout get_layout(void);
static void set_layout(Layout layout);
static int get_main_area_n(void);
static void set_main_area_n(int n);
static double get_main_area_ratio(void);
static void set_main_area_ratio(double ratio);
static double get_fixed_area_ratio(void);
static void set_fixed_area_ratio(double ratio);

static int main_area_ns[DESKTOP_N]; // 主區域可容納的客戶窗口數量
static Layout layouts[DESKTOP_N]; // 爲當前布局模式
static double main_area_ratios[DESKTOP_N], fixed_area_ratios[DESKTOP_N]; // 分別爲主要和固定區域與工作區寬度的比值

void update_layout(void)
{
    if(clients_is_empty())
        return;

    switch(get_layout())
    {
        case STACK: set_stack_layout(); break;
        case TILE: set_tile_layout(); break;
    }
    clients_for_each(c)
        if(is_on_cur_desktop(c))
            move_resize_client(c, NULL);
}

static void set_stack_layout(void)
{
    fix_wins_rect();
}

static void set_tile_layout(void)
{
    fix_area_for_tile();
    set_wins_rect_for_tiling();
    fix_wins_rect();
}

static void fix_area_for_tile(void)
{
    int n=0, m=get_main_area_n();
    clients_for_each(c)
    {
        if(is_on_cur_desktop(c) && !is_iconic_client(c) && !c->owner)
        {
            if(c->area==MAIN_AREA && ++n>m)
                c->area=SECOND_AREA;
            else if(c->area==SECOND_AREA && n<m)
                c->area=MAIN_AREA, n++;
        }
    }
}

/* 平鋪布局模式中需要平鋪的窗口的空間布置如下：
 *     1、屏幕從左至右分別布置次要區域、主要區域、固定區域；
 *     2、同一區域內的窗口均分本區域空間（末尾窗口取餘量），
 *        若任務欄在下方，則窗口間隔設置在前窗尾部，否則設置在前窗開頭；
 *     3、在次要區域內設置其與主區域的窗口間隔；
 *     4、在固定區域內設置其與主區域的窗口間隔。 */
static void set_wins_rect_for_tiling(void)
{
    int i=0, j=0, k=0, mw, sw, fw, mh, sh, fh, g=cfg->win_gap, 
        y_offset = cfg->show_taskbar && cfg->taskbar_on_top ? g : 0;
    Rect wr=get_net_workarea();

    get_area_size(&mw, &mh, &sw, &sh, &fw, &fh);
    clients_for_each(c)
    {
        if(is_on_cur_desktop(c) && is_tiling_client(c))
        {
            int x=0, y=0, w=0, h=0;
            if(c->area == FIXED_AREA)
                x=wr.x+mw+sw+g, y=wr.y+i++*fh+y_offset, w=fw-g, h=fh-g;
            else if(c->area == MAIN_AREA)
                x=wr.x+sw, y=wr.y+j++*mh+y_offset, w=mw, h=mh-g;
            else if(c->area == SECOND_AREA)
                x=wr.x, y=wr.y+k++*sh+y_offset, w=sw-g, h=sh-g;
            if(is_place_last_client(c)) // 區末窗口取餘量
                h+=wr.h%(h+g);
            set_client_rect_by_outline(c, x, y, w, h);
        }
    }
}

static void get_area_size(int *mw, int *mh, int *sw, int *sh, int *fw, int *fh)
{
    double mr=get_main_area_ratio(), fr=get_fixed_area_ratio();
    int n1, n2, n3;
    Rect wr=get_net_workarea();

    n1=get_clients_n(NORMAL_LAYER, MAIN_AREA, false, false, false),
    n2=get_clients_n(NORMAL_LAYER, SECOND_AREA, false, false, false),
    n3=get_clients_n(NORMAL_LAYER, FIXED_AREA, false, false, false),
    *mw=mr*wr.w, *fw=wr.w*fr, *sw=wr.w-*fw-*mw;
    *mh = n1 ? wr.h/n1 : wr.h;
    *fh = n3 ? wr.h/n3 : wr.h;
    *sh = n2 ? wr.h/n2 : wr.h;
    if(n3 == 0)
        *mw+=*fw, *fw=0;
    if(n2 == 0)
        *mw+=*sw, *sw=0;
}

static void update_titlebars_layout(void)
{
    clients_for_each(c)
        if(c->decorative && is_on_cur_desktop(c))
            titlebar_update_layout(c->frame);
}

static void fix_wins_rect(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c))
            fix_win_rect(c);
}

static void fix_win_rect(Client *c)
{
    if(!should_fix_win_rect(c))
        return;

    fix_win_rect_by_owner(c);
    fix_win_rect_by_workarea(c);
    fix_win_rect_by_hint(c);
}

static bool should_fix_win_rect(Client *c)
{
    Layer layer=c->layer;
    if(layer==FULLSCREEN_LAYER || layer==DOCK_LAYER || layer==DESKTOP_LAYER)
        return false;

    switch(get_layout())
    {
        case STACK: return is_new_client(c);
        case TILE:  return c->owner || (is_new_client(c) && layer!=NORMAL_LAYER);
        default:    return false;
    }
}

static void fix_win_rect_by_owner(Client *c)
{
    if(c->owner == NULL)
        return;

    WIDGET_W(c->frame) = WIDGET_W(c->owner)/2;
    WIDGET_H(c->frame) = WIDGET_H(c->owner)/2;
    WIDGET_X(c->frame) = WIDGET_X(c->owner)
        + (WIDGET_W(c->owner)-WIDGET_W(c->frame))/2;
    WIDGET_Y(c->frame) = WIDGET_Y(c->owner)
        + (WIDGET_H(c->owner)-WIDGET_H(c->frame))/2;
    set_client_rect_by_outline(c, WIDGET_X(c->frame), WIDGET_Y(c->frame),
        WIDGET_W(c->frame), WIDGET_H(c->frame));
}

static void fix_win_rect_by_hint(Client *c)
{
    XSizeHints hint=get_size_hint(WIDGET_WIN(c));
    if(!c->owner && ((hint.flags & USPosition) || (hint.flags & PPosition)))
        WIDGET_X(c)=hint.x, WIDGET_Y(c)=hint.y;
    fix_win_size_by_hint(&hint, &WIDGET_W(c), &WIDGET_H(c));
    set_frame_rect_by_client(c);
}

static void fix_win_rect_by_workarea(Client *c)
{
    if(c->win_type.desktop || c->win_type.dock || c->win_state.fullscreen)
        return;

    Rect r=get_net_workarea();
    fix_frame_size_by_workarea(c, &r);
    fix_frame_pos_by_workarea(c, &r);
    fix_dialog_frame_rect(c, &r);
    set_client_rect_by_outline(c, WIDGET_X(c->frame), WIDGET_Y(c->frame),
        WIDGET_W(c->frame), WIDGET_H(c->frame));
}

static void fix_frame_size_by_workarea(Client *c, const Rect *workarea)
{
    if(WIDGET_W(c->frame) > workarea->w)
        WIDGET_W(c->frame)=workarea->w;
    if(WIDGET_H(c->frame) > workarea->h)
        WIDGET_H(c->frame)=workarea->h;
}

static void fix_frame_pos_by_workarea(Client *c, const Rect *workarea)
{
    int w=WIDGET_W(c->frame), h=WIDGET_H(c->frame),
        wx=workarea->x, wy=workarea->y, ww=workarea->w, wh=workarea->h;
    if(WIDGET_X(c->frame) >= wx+ww-w) // 窗口在工作區右邊出界
        WIDGET_X(c->frame)=wx+ww-w;
    if(WIDGET_X(c->frame) < wx) // 窗口在工作區左邊出界
        WIDGET_X(c->frame)=wx;
    if(WIDGET_Y(c->frame) >= wy+wh-h) // 窗口在工作區下邊出界
        WIDGET_Y(c->frame)=wy+wh-h;
    if(WIDGET_Y(c->frame) < wy) // 窗口在工作區上邊出界
        WIDGET_Y(c->frame)=wy;
}

static void fix_dialog_frame_rect(Client *c, const Rect *workarea)
{
    if(!c->owner && c->win_type.dialog)
    {
        WIDGET_W(c->frame)=workarea->w/2;
        WIDGET_H(c->frame)=workarea->h/2;
        WIDGET_X(c->frame)=workarea->x+(workarea->w-WIDGET_W(c->frame))/2;
        WIDGET_Y(c->frame)=workarea->y+(workarea->h-WIDGET_H(c->frame))/2;
    }
}

static void set_frame_rect_by_client(Client *c)
{
    int bw = WIDGET_BORDER_W(c->frame),
        th = frame_get_titlebar_height(c->frame);
    WIDGET_X(c->frame) = WIDGET_X(c)-bw;
    WIDGET_Y(c->frame) = WIDGET_Y(c)-bw-th;
    WIDGET_H(c->frame) = WIDGET_H(c)+th;
}

bool is_main_sec_gap(int x)
{
    Rect wr=get_net_workarea();
    long sw=wr.w*(1-get_main_area_ratio()-get_fixed_area_ratio()),
         n=get_clients_n(NORMAL_LAYER, SECOND_AREA, false, false, false);
    return (n && x>=wr.x+sw-cfg->win_gap && x<wr.x+sw);
}

bool is_main_fix_gap(int x)
{
    Rect wr=get_net_workarea();
    long smw=wr.w*(1-get_fixed_area_ratio()),
         n=get_clients_n(NORMAL_LAYER, FIXED_AREA, false, false, false);
    return (n && x>=wr.x+smw && x<wr.x+smw+cfg->win_gap);
}

bool is_layout_adjust_area(Window win, int x)
{
    return (get_layout()==TILE && win==xinfo.root_win
        && (is_main_sec_gap(x) || is_main_fix_gap(x)));
}

void change_layout(Layout layout)
{
    Layout cl=get_layout();

    if(layout == cl)
        return;

    set_layout(layout);
    set_gwm_layout(layout);
    clients_for_each(c)
        if(is_on_cur_desktop(c))
            set_default_layer(c), update_net_wm_state_by_layer(c);

    request_layout_update();
    update_titlebars_layout();
    taskbar_update_bg();
}

void pointer_adjust_layout_ratio(XEvent *e)
{
    if( get_layout()!=TILE
        || !is_layout_adjust_area(e->xbutton.window, e->xbutton.x_root)
        || !grab_pointer(xinfo.root_win, LAYOUT_RESIZE))
        return;

    int ox=e->xbutton.x_root, nx, dx;
    XEvent ev;
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, dx=nx-ox;
            if(abs(dx)>=cfg->resize_inc && change_layout_ratio(ox, nx))
                request_layout_update(), ox=nx;
        }
        else
            handle_event(&ev);
    }while(!is_match_button_release(&e->xbutton, &ev.xbutton));
    XUngrabPointer(xinfo.display, CurrentTime);
}

static bool change_layout_ratio(int ox, int nx)
{
    double dr, m=get_main_area_ratio(), f=get_fixed_area_ratio();
    Rect wr=get_net_workarea();
    dr=1.0*(nx-ox)/wr.w;
    if(is_main_sec_gap(ox))
        set_main_area_ratio(m-dr);
    else if(is_main_fix_gap(ox))
        set_main_area_ratio(m+dr), set_fixed_area_ratio(f-dr);
    else
        return false;
    return true;
}

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
void adjust_main_area(double change_ratio)
{
    if( get_layout()==TILE
        && get_clients_n(NORMAL_LAYER, SECOND_AREA, false, false, false))
    {
        Rect wr=get_net_workarea();
        double mr=get_main_area_ratio()+change_ratio, fr=get_fixed_area_ratio();
        long mw=mr*wr.w, sw=wr.w*(1-fr)-mw;
        if(sw>=cfg->resize_inc && mw>=cfg->resize_inc)
        {
            set_main_area_ratio(mr);
            request_layout_update();
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void adjust_fixed_area(double change_ratio)
{
    if( get_layout()==TILE
        && get_clients_n(NORMAL_LAYER, FIXED_AREA, false, false, false))
    {
        Rect wr=get_net_workarea();
        double fr=get_fixed_area_ratio()+change_ratio, mr=get_main_area_ratio();
        long mw=wr.w*(mr-change_ratio), fw=wr.w*fr;
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
