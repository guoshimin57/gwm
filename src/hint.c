/* *************************************************************************
 *     hint.c：實現窗口尺寸條件特性的相關功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "client.h"

static bool is_prefer_width_inc(unsigned int w, int dw, XSizeHints *hint);
static bool is_prefer_height_inc(unsigned int h, int dh, XSizeHints *hint);
static int get_fixed_width_inc(unsigned int w, XSizeHints *hint);
static int get_fixed_height_inc(unsigned int h, XSizeHints *hint);
static bool is_prefer_size(unsigned int w, unsigned int h, XSizeHints *hint);
static bool is_prefer_aspect(unsigned int w, unsigned int h, XSizeHints *hint);

unsigned int get_client_col(WM *wm, Client *c)
{
    return (c->w-c->size_hint.base_width)/c->size_hint.width_inc;
}

unsigned int get_client_row(WM *wm, Client *c)
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
            hint.width_inc=MOVE_RESIZE_INC;
        if(!hint.height_inc)
            hint.height_inc=MOVE_RESIZE_INC;
        c->size_hint=hint;
    }
    SET_DEF_VAL(c->size_hint.width_inc, MOVE_RESIZE_INC);
    SET_DEF_VAL(c->size_hint.height_inc, MOVE_RESIZE_INC);
}

bool get_prefer_resize(WM *wm, Client *c, Delta_rect *d)
{
    XSizeHints *p=&c->size_hint;
    int dx=0, dy=0, dw=0, dh=0;
    if((c->area_type==FLOATING_AREA || DESKTOP(wm).cur_layout==STACK) && p->flags)
    {
        unsigned int w=c->w, h=c->h, wi=p->width_inc, hi=p->height_inc;
        int sdw=(d->dw>=0 ? 1 : -1), sdh=(d->dh>=0 ? 1 : -1);
        bool flag, flag1, flag2, flag3, wflag, hflag;
        if(wi)
            dw=(abs(d->dw)<=wi ? d->dw : sdw*wi), w+=dw;
        if(hi)
            dh=(abs(d->dh)<=hi ? d->dh : sdh*hi), h+=dh;
        wflag=is_prefer_width_inc(c->w, dw, p);
        hflag=is_prefer_height_inc(c->h, dh, p);
        flag1=(is_prefer_size(w, h, p) && is_prefer_aspect(w, h, p));
        flag2=(is_prefer_size(w, c->h, p) && is_prefer_aspect(w, c->h, p));
        flag3=(is_prefer_size(c->w, h, p) && is_prefer_aspect(c->w, h, p));
        flag=flag1 || flag2 || flag3;
        if(flag && (wflag || hflag))
        {
            if(wflag && (flag1 || flag2))
                dx = (d->dx && d->dw) ? -dw : 0;
            else
                dx=0, dw=0;
            if(hflag && (flag1 || flag3))
                dy = (d->dy && d->dh) ? -dh : 0;
            else
                dy=0, dh=0;
        }
        else
            dx=0, dy=0, dw=0, dh=0;
    }
    d->dx=dx, d->dy=dy, d->dw=dw, d->dh=dh;
    return d->dw || d->dh;
}

static bool is_prefer_width_inc(unsigned int w, int dw, XSizeHints *hint)
{
    return !hint->width_inc || abs(dw)>=abs(get_fixed_width_inc(w, hint));
}

static bool is_prefer_height_inc(unsigned int h, int dh, XSizeHints *hint)
{
    return !hint->height_inc || abs(dh)>=abs(get_fixed_height_inc(h, hint));
}

static int get_fixed_width_inc(unsigned int w, XSizeHints *hint)
{
    int inc=MOVE_RESIZE_INC;
    if(hint->base_width)
        inc=(w-hint->base_width)%hint->width_inc;
    else if(hint->min_width)
        inc=(w-hint->min_width)%hint->width_inc;
    return inc ? inc : hint->width_inc;
}

static int get_fixed_height_inc(unsigned int h, XSizeHints *hint)
{
    int inc=MOVE_RESIZE_INC;
    if(hint->base_height)
        inc=(h-hint->base_height)%hint->height_inc;
    else if(hint->min_height)
        inc=(h-hint->min_height)%hint->height_inc;
    return inc ? inc : hint->height_inc;
}

static bool is_prefer_size(unsigned int w, unsigned int h, XSizeHints *hint)
{
    return (!hint->min_width || w>=hint->min_width)
        && (!hint->min_height || h>=hint->min_height)
        && (!hint->max_width || w<=hint->max_width)
        && (!hint->max_height || h<=hint->max_height);
}

static bool is_prefer_aspect(unsigned int w, unsigned int h, XSizeHints *hint)
{
    return (!hint->min_aspect.x || !hint->min_aspect.y
        || !hint->max_aspect.x || !hint->max_aspect.y
        || ( (float)w/h >= (float)hint->min_aspect.x/hint->min_aspect.y
        && (float)w/h <= (float)hint->max_aspect.x/hint->max_aspect.y));
}

void set_input_focus(WM *wm, XWMHints *hint, Window win)
{
    if(!hint || (hint->flags & InputHint) || hint->input)
        XSetInputFocus(wm->display, win, RevertToPointerRoot, CurrentTime);
    send_event(wm, wm->icccm_atoms[WM_TAKE_FOCUS], win);
}
