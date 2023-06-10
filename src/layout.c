/* *************************************************************************
 *     layout.c：實現與窗口布局相關的功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void set_full_layout(WM *wm);
static void set_preview_layout(WM *wm);
static void set_tile_layout(WM *wm);
static void get_area_size(WM *wm, long *mw, long *mh, long *sw, long *sh, long *fw, long *fh);
static void fix_win_rect_for_frame(WM *wm);
static bool should_fix_win_rect(WM *wm, Client *c);
static void fix_cur_focus_client_rect(WM *wm);

void update_layout(WM *wm)
{
    if(wm->clients == wm->clients->next)
        return;

    fix_area_type(wm);
    switch(DESKTOP(wm)->cur_layout)
    {
        case FULL: set_full_layout(wm); break;
        case PREVIEW: set_preview_layout(wm); break;
        case STACK: break;
        case TILE: set_tile_layout(wm); break;
    }
    fix_win_rect_for_frame(wm);
    fix_cur_focus_client_rect(wm);
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            move_resize_client(wm, c, NULL);
    if(DESKTOP(wm)->cur_layout == FULL)
        XUnmapWindow(wm->display, wm->taskbar->win);
    else if(DESKTOP(wm)->prev_layout == FULL)
        XMapWindow(wm->display, wm->taskbar->win);
}

static void set_full_layout(WM *wm)
{
    Client *c=CUR_FOC_CLI(wm);
    c->x=c->y=0, c->w=wm->screen_width, c->h=wm->screen_height;
}

static void set_preview_layout(WM *wm)
{
    long n=get_clients_n(wm), i=n-1, rows, cols, w, h, gap=wm->cfg->win_gap,
         wx=wm->workarea.x, wy=wm->workarea.y,
         ww=wm->workarea.w, wh=wm->workarea.h;
    if(n == 0)
        return;
    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=n && cols*cols<n; cols++)
        ;
    rows=(cols-1)*cols>=n ? cols-1 : cols;
    w=ww/cols, h=wh/rows;
    if(wm->cfg->show_taskbar && wm->cfg->taskbar_on_top)
        wy+=gap;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(is_on_cur_desktop(wm, c))
        {
            c->x=wx+(i%cols)*w, c->y=wy+(i/cols)*h;
            /* 下邊和右邊的窗口佔用剩餘空間 */
            c->w=(i+1)%cols ? w-gap : w+(ww-w*cols);
            c->h=i<cols*(rows-1) ? h-gap : h+(wh-h*rows)-gap;
            i--;
        }
    }
}

/* 平鋪布局模式的空間布置如下：
 *     1、屏幕從左至右分別布置次要區域、主要區域、固定區域；
 *     2、同一區域內的窗口均分本區域空間（末尾窗口取餘量），窗口間隔設置在前窗尾部；
 *     3、在次要區域內設置其與主區域的窗口間隔；
 *     4、在固定區域內設置其與主區域的窗口間隔。 */
static void set_tile_layout(WM *wm)
{
    long i=0, j=0, k=0, mw, sw, fw, mh, sh, fh, g=wm->cfg->win_gap,
         wx=wm->workarea.x, wy=wm->workarea.y, wh=wm->workarea.h;

    get_area_size(wm, &mw, &mh, &sw, &sh, &fw, &fh);
    if(wm->cfg->show_taskbar && wm->cfg->taskbar_on_top)
        wy+=g;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        Area_type type=c->area_type;
        if( is_on_cur_desktop(wm, c)
            && (type==MAIN_AREA || type==SECOND_AREA || type==FIXED_AREA))
        {
            if(type == FIXED_AREA)
                c->x=wx+mw+sw+g, c->y=wy+i++*fh, c->w=fw-g, c->h=fh-g;
            else if(type == MAIN_AREA)
                c->x=wx+sw, c->y=wy+j++*mh, c->w=mw, c->h=mh-g;
            else if(type == SECOND_AREA)
                c->x=wx, c->y=wy+k++*sh, c->w=sw-g, c->h=sh-g;
            // 區末窗口取餘量
            if(is_last_typed_client(wm, c, type))
                c->h+=wh%(c->h+g);
        }
    }
}

static void get_area_size(WM *wm, long *mw, long *mh, long *sw, long *sh, long *fw, long *fh)
{
    double mr=DESKTOP(wm)->main_area_ratio, fr=DESKTOP(wm)->fixed_area_ratio;
    long n1, n2, n3, ww=wm->workarea.w, wh=wm->workarea.h;
    n1=get_typed_clients_n(wm, MAIN_AREA),
    n2=get_typed_clients_n(wm, SECOND_AREA),
    n3=get_typed_clients_n(wm, FIXED_AREA),
    *mw=mr*ww, *fw=ww*fr, *sw=ww-*fw-*mw;
    *mh = n1 ? wh/n1 : wh, *fh = n3 ? wh/n3 : wh, *sh = n2 ? wh/n2 : wh;
    if(n3 == 0)
        *mw+=*fw, *fw=0;
    if(n2 == 0)
        *mw+=*sw, *sw=0;
}

static void fix_win_rect_for_frame(WM *wm)
{
    if(DESKTOP(wm)->cur_layout==FULL || DESKTOP(wm)->cur_layout==STACK)
        return;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(should_fix_win_rect(wm, c))
            c->x+=c->border_w, c->y+=c->title_bar_h+c->border_w,
            c->w-=2*c->border_w, c->h-=c->title_bar_h+2*c->border_w;
}

static bool should_fix_win_rect(WM *wm, Client *c)
{
    Area_type t=c->area_type;
    return (is_on_cur_desktop(wm, c)
        && (DESKTOP(wm)->cur_layout==PREVIEW || (DESKTOP(wm)->cur_layout==TILE
        && (t==MAIN_AREA || t==SECOND_AREA || t==FIXED_AREA))));
}

static void fix_cur_focus_client_rect(WM *wm)
{
    Client *c=CUR_FOC_CLI(wm);
    if( DESKTOP(wm)->prev_layout==FULL && c->area_type==FLOATING_AREA
        && (DESKTOP(wm)->cur_layout==TILE || DESKTOP(wm)->cur_layout==STACK))
        set_default_win_rect(wm, c);
}


void update_title_bar_layout(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(c->title_bar_h && is_on_cur_desktop(wm, c))
        {
            Rect r=get_title_area_rect(wm, c);
            XResizeWindow(wm->display, c->title_area, r.w, r.h);
        }
    }
}

void update_taskbar_buttons(WM *wm)
{
    for(size_t h=TASKBAR_HEIGHT(wm), b=TASKBAR_BUTTON_BEGIN; b<=TASKBAR_BUTTON_END; b++)
    {
        size_t i=TASKBAR_BUTTON_INDEX(b);
        String_format f={{0, 0, wm->cfg->taskbar_button_width, h}, CENTER, true,
            TASKBAR_BUTTON_COLOR(wm, b),
            TEXT_COLOR(wm, TASKBAR_BUTTON), TASKBAR_BUTTON_FONT};
        draw_string(wm, wm->taskbar->buttons[i], wm->cfg->taskbar_button_text[i], &f);
    }
}

bool is_main_sec_gap(WM *wm, int x)
{
    Desktop *d=DESKTOP(wm);
    long sw=wm->workarea.w*(1-d->main_area_ratio-d->fixed_area_ratio),
         wx=wm->workarea.x, n=get_typed_clients_n(wm, SECOND_AREA);
    return (n && x>=wx+sw-wm->cfg->win_gap && x<wx+sw);
}

bool is_main_fix_gap(WM *wm, int x)
{
    long smw=wm->workarea.w*(1-DESKTOP(wm)->fixed_area_ratio),
         wx=wm->workarea.x, n=get_typed_clients_n(wm, FIXED_AREA);
    return (n && x>=wx+smw && x<wx+smw+wm->cfg->win_gap);
}

bool is_layout_adjust_area(WM *wm, Window win, int x)
{
    return (DESKTOP(wm)->cur_layout==TILE && win==wm->root_win
        && (is_main_sec_gap(wm, x) || is_main_fix_gap(wm, x)));
}

bool change_layout_ratio(WM *wm, int ox, int nx)
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
