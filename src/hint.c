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

static void fix_limit_size_hint(XSizeHints *h);
static bool is_prefer_width(unsigned int w, XSizeHints *hint);
static bool is_prefer_height(unsigned int h, XSizeHints *hint);
static bool is_prefer_aspect(unsigned int w, unsigned int h, XSizeHints *hint);
static void set_net_supported(WM *wm);
static void set_net_client_list(WM *wm);
static void set_net_client_list_stacking(WM *wm);
static int cmp_win(const void *pwin1, const void *pwin2);
static void set_net_number_of_desktops(WM *wm);
static void set_net_desktop_geometry(WM *wm);
static void set_net_desktop_viewport(WM *wm);
static void set_net_desktop_names(WM *wm);
static void set_net_workarea(WM *wm);
static void set_net_supporting_wm_check(WM *wm);

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
            hint.width_inc=wm->cfg->resize_inc;
        if(!hint.height_inc)
            hint.height_inc=wm->cfg->resize_inc;
        fix_limit_size_hint(&hint);
        c->size_hint=hint;
    }
    SET_DEF_VAL(c->size_hint.width_inc, wm->cfg->resize_inc);
    SET_DEF_VAL(c->size_hint.height_inc, wm->cfg->resize_inc);
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
    long col=get_client_col(c), row=get_client_row(c);
    c->w = (p->flags & USSize) && p->width ?
        p->width : p->base_width+col*p->width_inc;
    c->h = (p->flags & USSize) && p->height ?
        p->height : p->base_height+row*p->height_inc;
    if((p->flags & PMinSize) && p->min_width)
        c->w=MAX((long)c->w, p->min_width);
    if((p->flags & PMinSize) && p->min_height)
        c->h=MAX((long)c->h, p->min_height);
    if((p->flags & PMaxSize) && p->max_width)
        c->w=MIN((long)c->w, p->max_width);
    if((p->flags & PMaxSize) && p->max_height)
        c->h=MIN((long)c->h, p->max_height);
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
    long f=0, wl=w;
    return !hint || !(f=hint->flags) ||
        (  (!(f & PMinSize) || wl>=hint->min_width)
        && (!(f & PMaxSize) || wl<=hint->max_width)
        && (!(f & PBaseSize) || !(f & PResizeInc) || !hint->width_inc
           || (wl-hint->base_width)%hint->width_inc == 0));
}

static bool is_prefer_height(unsigned int h, XSizeHints *hint)
{
    long f=0, hl=h;
    return !hint || !(f=hint->flags) ||
        (  (!(f & PMinSize) || hl>=hint->min_height)
        && (!(f & PMaxSize) || hl<=hint->max_height)
        && (!(f & PBaseSize) || !(f & PResizeInc) || !hint->height_inc
           || (hl-hint->base_height)%hint->height_inc == 0));
}

static bool is_prefer_aspect(unsigned int w, unsigned int h, XSizeHints *hint)
{
    return !hint || !(hint->flags & PAspect) || !w || !h
        || !hint->min_aspect.x || !hint->min_aspect.y
        || !hint->max_aspect.x || !hint->max_aspect.y
        || (  (float)w/h >= (float)hint->min_aspect.x/hint->min_aspect.y
           && (float)w/h <= (float)hint->max_aspect.x/hint->max_aspect.y);
}

void set_ewmh(WM *wm)
{
    set_net_supported(wm);
    set_net_number_of_desktops(wm);
    set_net_desktop_geometry(wm);
    set_net_desktop_viewport(wm);
    set_net_current_desktop(wm);
    set_net_desktop_names(wm);
    set_net_workarea(wm);
    set_net_supporting_wm_check(wm);
    set_net_showing_desktop(wm, false);
}

static void set_net_supported(WM *wm)
{
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_SUPPORTED], XA_ATOM, 32,
        PropModeReplace, (unsigned char *)wm->ewmh_atom, EWMH_ATOM_N);
}

void set_all_net_client_list(WM *wm)
{
    set_net_client_list(wm);
    set_net_client_list_stacking(wm);
}

static void set_net_client_list(WM *wm)
{
    Window root=wm->root_win;
    Atom a = wm->ewmh_atom[_NET_CLIENT_LIST];
    unsigned long i=0, n=get_all_clients_n(wm);
    if(n == 0)
        XDeleteProperty(wm->display, root, a);
    else
    {
        Window list[n];
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            list[i++]=c->win;
        qsort(list, n, sizeof(Window), cmp_win);
        XChangeProperty(wm->display, root, a, XA_WINDOW, 32,
            PropModeReplace, (unsigned char *)list, n);
    }
}

static int cmp_win(const void *pwin1, const void *pwin2)
{
    return (*(Window *)pwin1-*(Window *)pwin2);
}

static void set_net_client_list_stacking(WM *wm)
{
    Atom a = wm->ewmh_atom[_NET_CLIENT_LIST_STACKING];
    unsigned int n=0, na=0;
    Window root, parent, *child=NULL;
    if(XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &na))
    {
        Client *c=NULL;
        for(unsigned int i=0; i<na; i++)
            if((c=win_to_client(wm, child[i])) && is_on_cur_desktop(wm, c))
                child[n++]=child[i];
        XChangeProperty(wm->display, wm->root_win, a, XA_WINDOW, 32,
            PropModeReplace, (unsigned char *)child, n);
        XFree(child);
    }
}

static void set_net_number_of_desktops(WM *wm)
{
    int32_t desktop_n=DESKTOP_N;
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_NUMBER_OF_DESKTOPS], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&desktop_n, 1);
}

static void set_net_desktop_geometry(WM *wm)
{
    int32_t size[2]={wm->screen_width, wm->screen_height};
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_DESKTOP_GEOMETRY], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)size, 2);
}

static void set_net_desktop_viewport(WM *wm)
{
    int32_t pos[2]={0, 0};
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_DESKTOP_GEOMETRY], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)pos, 2);
}

void set_net_current_desktop(WM *wm)
{
    int32_t n=wm->cur_desktop-1;
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_CURRENT_DESKTOP], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&n, 1);
}

static void set_net_desktop_names(WM *wm)
{
    size_t n=0, begin=TASKBAR_BUTTON_INDEX(DESKTOP_BUTTON_BEGIN);

    for(size_t i=0; i<DESKTOP_N; i++)
        n += strlen(wm->cfg->taskbar_button_text[begin+i])+1;
    XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_DESKTOP_NAMES], wm->utf8, 8, PropModeReplace,
        (unsigned char *)wm->cfg->taskbar_button_text[begin], n);
}

void set_net_active_window(WM *wm)
{
    XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_ACTIVE_WINDOW], XA_WINDOW, 32, PropModeReplace,
        (unsigned char *)&CUR_FOC_CLI(wm)->win, 1);
}

static void set_net_workarea(WM *wm)
{
    int x=wm->workarea.x, y=wm->workarea.y;
    unsigned int w=wm->workarea.w, h=wm->workarea.h;
    int32_t rect[DESKTOP_N][4];

    for(size_t i=0; i<DESKTOP_N; i++)
        rect[i][0]=x, rect[i][1]=y, rect[i][2]=w, rect[i][3]=h;
    XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_WORKAREA], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)rect, DESKTOP_N*4);
}

static void set_net_supporting_wm_check(WM *wm)
{
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_SUPPORTING_WM_CHECK], XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wm->wm_check_win, 1);

    /* FIXME: 此項設置會導致firefox不能全屏顯示，也許是因爲gwm不支持真正的全屏，確切原因未明
	XChangeProperty(wm->display, wm->wm_check_win,
        wm->ewmh_atom[_NET_SUPPORTING_WM_CHECK], XA_WINDOW, 32,
        PropModeReplace, (unsigned char *)&wm->wm_check_win, 1);
        */

	XChangeProperty(wm->display, wm->wm_check_win,
        wm->ewmh_atom[_NET_WM_NAME], wm->utf8, 8,
        PropModeReplace, (unsigned char *)"gwm", 3);
}

void set_net_showing_desktop(WM *wm, bool show)
{
	XChangeProperty(wm->display, wm->root_win,
        wm->ewmh_atom[_NET_SHOWING_DESKTOP], XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&show, 1);
}

void set_urgency(WM *wm, Client *c, bool urg)
{
    XWMHints *h=XGetWMHints(wm->display, c->win);
    if(!h)
        return;
    h->flags = urg ? (h->flags | XUrgencyHint) : (h->flags & ~XUrgencyHint);
    XSetWMHints(wm->display, c->win, h);
    XFree(h);
}
