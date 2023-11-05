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

static void set_preview_layout(WM *wm);
static void set_rect_of_main_win_for_preview(WM *wm);
static void set_tile_layout(WM *wm);
static void set_rect_of_tile_win_for_tiling(WM *wm);
static void set_rect_of_transient_win_for_tiling(WM *wm);
static void get_area_size(WM *wm, int *mw, int *mh, int *sw, int *sh, int *fw, int *fh);

void update_layout(WM *wm)
{
    if(wm->clients == wm->clients->next)
        return;

    fix_place_type_for_tile(wm);
    switch(DESKTOP(wm)->cur_layout)
    {
        case PREVIEW: set_preview_layout(wm); break;
        case STACK: break;
        case TILE: set_tile_layout(wm); break;
    }
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            move_resize_client(wm, c, NULL);
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

    int rows, cols, w, h, g=wm->cfg->win_gap, wx=wm->workarea.x,
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

static void set_tile_layout(WM *wm)
{
    set_rect_of_tile_win_for_tiling(wm);
    set_rect_of_transient_win_for_tiling(wm);
}

/* 平鋪布局模式中需要平鋪的窗口的空間布置如下：
 *     1、屏幕從左至右分別布置次要區域、主要區域、固定區域；
 *     2、同一區域內的窗口均分本區域空間（末尾窗口取餘量），窗口間隔設置在前窗尾部；
 *     3、在次要區域內設置其與主區域的窗口間隔；
 *     4、在固定區域內設置其與主區域的窗口間隔。 */
static void set_rect_of_tile_win_for_tiling(WM *wm)
{
    int i=0, j=0, k=0, mw, sw, fw, mh, sh, fh, g=wm->cfg->win_gap,
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
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
        if(is_on_cur_desktop(wm, c) && c->owner)
            fix_win_pos(wm, c);
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
    return (n && x>=wx+sw-wm->cfg->win_gap && x<wx+sw);
}

bool is_main_fix_gap(WM *wm, int x)
{
    long smw=wm->workarea.w*(1-DESKTOP(wm)->fixed_area_ratio),
         wx=wm->workarea.x,
         n=get_clients_n(wm, TILE_LAYER_FIXED, false, false, false);
    return (n && x>=wx+smw && x<wx+smw+wm->cfg->win_gap);
}

bool is_layout_adjust_area(WM *wm, Window win, int x)
{
    return (DESKTOP(wm)->cur_layout==TILE && win==xinfo.root_win
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
