/* *************************************************************************
 *     hint.c：實現窗口尺寸條件特性的相關功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "config.h"
#include "client.h"
#include "drawable.h"
#include "misc.h"
#include "hint.h"

static void fix_limit_size_hint(XSizeHints *h);
static bool is_prefer_width(unsigned int w, XSizeHints *hint);
static bool is_prefer_height(unsigned int h, XSizeHints *hint);
static bool is_prefer_aspect(unsigned int w, unsigned int h, XSizeHints *hint);

unsigned int get_client_col(Client *c)
{
    return (c->w-c->size_hint.base_width)/c->size_hint.width_inc;
}

unsigned int get_client_row(Client *c)
{
    return (c->h-c->size_hint.base_height)/c->size_hint.height_inc;
}

/* 通常程序在創建窗口時就設置好窗口尺寸特性，一般情況下不會再修改。但實際上有些
 * 奇葩的程序會在調整窗口尺寸後才更新窗口尺寸特性，而有些程序則明明設置了窗口的
 * 尺寸特性標志位，但相應的XSizeHints結構成員其實沒有設置。因此，不要指望在添加
 * 客戶窗口時一勞永逸地存儲窗口尺寸特性。
 */
void update_size_hint(WM *wm, Client *c)
{
    long flags;
    XSizeHints hint={0};

    if(XGetWMNormalHints(wm->display, c->win, &hint, &flags))
    {
        unsigned int basew=0, baseh=0, minw=0, minh=0;
        if(hint.flags & PBaseSize)
            basew=hint.base_width, baseh=hint.base_height;
        if(hint.flags & PMinSize)
            minw=hint.min_width, minh=hint.min_height;
        if(!basew && minw)
            hint.base_width=minw;
        if(!baseh && minh)
            hint.base_height=minh;
        if(!minw && basew)
            hint.min_width=basew;
        if(!minh && baseh)
            hint.min_height=baseh;
        if(!hint.width_inc)
            hint.width_inc=RESIZE_INC;
        if(!hint.height_inc)
            hint.height_inc=RESIZE_INC;
        fix_limit_size_hint(&hint);
        c->size_hint=hint;
    }
    SET_DEF_VAL(c->size_hint.width_inc, RESIZE_INC);
    SET_DEF_VAL(c->size_hint.height_inc, RESIZE_INC);
}

// 有的窗口最大、最小尺寸設置不正確，需要修正，如：lxterminal
static void fix_limit_size_hint(XSizeHints *h)
{
    int minw_incs=base_n_ceil(h->min_width-h->base_width, h->width_inc),
        minh_incs=base_n_ceil(h->min_height-h->base_height, h->height_inc),
        maxw_incs=base_n_floor(h->max_width-h->base_width, h->width_inc),
        maxh_incs=base_n_floor(h->max_height-h->base_height, h->height_inc);
    h->min_width=h->base_width+minw_incs;
    h->min_height=h->base_height+minh_incs;
    h->max_width=h->base_width+maxw_incs;
    h->max_height=h->base_height+maxh_incs;
}

void fix_win_size_by_hint(Client *c)
{
    XSizeHints *p=&c->size_hint;
    c->w = (p->flags & USSize) && p->width ?
        p->width : p->base_width+get_client_col(c)*p->width_inc;
    c->h = (p->flags & USSize) && p->height ?
        p->height : p->base_height+get_client_row(c)*p->height_inc;
    if((p->flags & PMinSize) && p->min_width)
        c->w=MAX(c->w, p->min_width);
    if((p->flags & PMinSize) && p->min_height)
        c->h=MAX(c->h, p->min_height);
    if((p->flags & PMaxSize) && p->max_width)
        c->w=MIN(c->w, p->max_width);
    if((p->flags & PMaxSize) && p->max_height)
        c->h=MIN(c->h, p->max_height);
    if( (p->flags & PAspect) && p->min_aspect.x && p->min_aspect.y
        && p->max_aspect.x && p->max_aspect.y)
    {
        float mina=(float)p->min_aspect.x/p->min_aspect.y,
              maxa=(float)p->max_aspect.x/p->max_aspect.y;
        if((float)c->w/c->h < mina)
            c->h=c->w*mina+0.5;
        else if((float)c->w/c->h > maxa)
            c->w=c->h*maxa+0.5;
    }
}

bool is_prefer_size(unsigned int w, unsigned int h, XSizeHints *hint)
{
    return is_prefer_width(w, hint)
        && is_prefer_height(h, hint)
        && is_prefer_aspect(w, h, hint);
}

static bool is_prefer_width(unsigned int w, XSizeHints *hint)
{
    long f=0;
    return !hint || !(f=hint->flags) ||
        (  (!(f & PMinSize) || w>=hint->min_width)
        && (!(f & PMaxSize) || w<=hint->max_width)
        && (!(f & PBaseSize) || !(f & PResizeInc) || !hint->width_inc
           || (w-hint->base_width)%hint->width_inc == 0));
}

static bool is_prefer_height(unsigned int h, XSizeHints *hint)
{
    long f=0;
    return !hint || !(f=hint->flags) ||
        (  (!(f & PMinSize) || h>=hint->min_height)
        && (!(f & PMaxSize) || h<=hint->max_height)
        && (!(f & PBaseSize) || !(f & PResizeInc) || !hint->height_inc
           || (h-hint->base_height)%hint->height_inc == 0));
}

static bool is_prefer_aspect(unsigned int w, unsigned int h, XSizeHints *hint)
{
    return !hint || !(hint->flags & PAspect) || !w || !h
        || !hint->min_aspect.x || !hint->min_aspect.y
        || !hint->max_aspect.x || !hint->max_aspect.y
        || (  (float)w/h >= (float)hint->min_aspect.x/hint->min_aspect.y
           && (float)w/h <= (float)hint->max_aspect.x/hint->max_aspect.y);
}

void set_input_focus(WM *wm, XWMHints *hint, Window win)
{
    if(!hint || ((hint->flags & InputHint) && hint->input)) // 不抗拒鍵盤輸入
        XSetInputFocus(wm->display, win, RevertToPointerRoot, CurrentTime);
    send_event(wm, wm->icccm_atoms[WM_TAKE_FOCUS], win);
}
