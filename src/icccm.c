/* *************************************************************************
 *     icccm.c：實現ICCCM規範。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

const char *icccm_atom_names[ICCCM_ATOMS_N]= // ICCCM規範標識符名稱
{
    "WM_PROTOCOLS", "WM_DELETE_WINDOW", "WM_STATE", "WM_CHANGE_STATE",
    "WM_TAKE_FOCUS", "UTF8_STRING",
};

static Atom icccm_atoms[ICCCM_ATOMS_N]; // ICCCM規範的標識符

static void fix_limit_size_hint(XSizeHints *h);
static bool is_prefer_width(int w, const XSizeHints *hint);
static bool is_prefer_height(int h, const XSizeHints *hint);
static bool is_prefer_aspect(int w, int h, const XSizeHints *hint);

bool is_spec_icccm_atom(Atom spec, ICCCM_atom_id id)
{
    return spec == icccm_atoms[id];
}

/* UTF8_STRING需要特殊處理，因其是否屬於ICCCM尚存疑，詳見：
 *     https://gitlab.freedesktop.org/xorg/doc/xorg-docs/-/issues/5 */
Atom get_utf8_string_atom(void)
{
    return icccm_atoms[UTF8_STRING];
}

void replace_utf8_prop(Window win, Atom prop, const void *strs, int n)
{
    XChangeProperty(xinfo.display, win, prop, get_utf8_string_atom(), 8,
        PropModeReplace, (unsigned char *)strs, n);
}

void set_icccm_atoms(void)
{
    for(int i=0; i<ICCCM_ATOMS_N; i++)
        icccm_atoms[i]=XInternAtom(xinfo.display, icccm_atom_names[i], False);
}

int get_win_col(int width, const XSizeHints *hint)
{
    return (width-hint->base_width)/hint->width_inc;
}

int get_win_row(int height, const XSizeHints *hint)
{
    return (height-hint->base_height)/hint->height_inc;
}

/* 通常程序在創建窗口時就設置好窗口尺寸特性，一般情況下不會再修改。但實際上有些
 * 奇葩的程序會在調整窗口尺寸後才更新窗口尺寸特性，而有些程序則明明設置了窗口的
 * 尺寸特性標志位，但相應的XSizeHints結構成員其實沒有設置。因此，不要指望在添加
 * 客戶窗口時一勞永逸地存儲窗口尺寸特性。
 */
void update_size_hint(Window win, int fallback_inc, XSizeHints *size_hint)
{
    long f=0, flags=0;
    XSizeHints h={0};

    if(XGetWMNormalHints(xinfo.display, win, &h, &flags))
    {
        /* h.flags只使用最低字節保存了ICCCM之前確立的標志，
         * flags只使用除最低字節以外的字節保存了ICCCM新增的標志 */
        f = (h.flags & 0xff) | (flags & ~0xff);
        if(!(f & PMinSize) && (f & PBaseSize))
            h.min_width=h.base_width, h.min_height=h.base_height, f|=PMinSize;
        if(!(f & PBaseSize) && (f & PMinSize))
            h.base_width=h.min_width, h.base_height=h.min_height, f|=PBaseSize;
        if(!(f & PResizeInc))
            h.width_inc=h.height_inc=fallback_inc;
    }
    if(!h.width_inc)
        h.width_inc=fallback_inc;
    if(!h.height_inc)
        h.height_inc=fallback_inc;
    f|=PResizeInc;
    h.flags=f;
    fix_limit_size_hint(&h);
    *size_hint=h;
}

// 有的窗口最大、最小尺寸設置不正確，需要修正，如：lxterminal
static void fix_limit_size_hint(XSizeHints *h)
{
    if( h->flags & PMinSize && h->flags & PMaxSize
        && h->min_width==h->max_width && h->min_height==h->max_height)
        return;

    int minw_incs=base_n_ceil(h->min_width-h->base_width, h->width_inc),
        minh_incs=base_n_ceil(h->min_height-h->base_height, h->height_inc),
        maxw_incs=base_n_floor(h->max_width-h->base_width, h->width_inc),
        maxh_incs=base_n_floor(h->max_height-h->base_height, h->height_inc);

    h->min_width=h->base_width+minw_incs;
    h->min_height=h->base_height+minh_incs;
    h->max_width=h->base_width+maxw_incs;
    h->max_height=h->base_height+maxh_incs;
}

void fix_win_size_by_hint(const XSizeHints *size_hint, int *w, int *h)
{
    const XSizeHints *p=size_hint;
    long f=p->flags;
    int col=get_win_col(*w, p), row=get_win_row(*w, p);

    if((f & USSize || f & PSize) && p->width)
        *w=p->width;
    else if((f & PBaseSize) && p->base_width)
        *w=p->base_width+col*p->width_inc;
    if((f & USSize || f & PSize) && p->height)
        *h=p->height;
    else if((f & PBaseSize) && p->base_height)
        *h=p->base_height+row*p->height_inc;
    if((f & PMinSize) && p->min_width)
        *w=MAX(*w, p->min_width);
    if((f & PMinSize) && p->min_height)
        *h=MAX(*h, p->min_height);
    if((f & PMaxSize) && p->max_width)
        *w=MIN(*w, p->max_width);
    if((f & PMaxSize) && p->max_height)
        *h=MIN(*h, p->max_height);
    if( (f & PAspect) && p->min_aspect.x && p->min_aspect.y
        && p->max_aspect.x && p->max_aspect.y)
    {
        float mina=(float)p->min_aspect.x/p->min_aspect.y,
              maxa=(float)p->max_aspect.x/p->max_aspect.y;
        if((float)*w / *h < mina)
            *h=*w*mina+0.5;
        else if((float)*w / *h > maxa)
            *w=*h*maxa+0.5;
    }
}

bool is_prefer_size(int w, int h, const XSizeHints *hint)
{
    return is_prefer_width(w, hint)
        && is_prefer_height(h, hint)
        && is_prefer_aspect(w, h, hint);
}

static bool is_prefer_width(int w, const XSizeHints *hint)
{
    long f=hint->flags;
    return !hint || !f ||
        (  (!(f & PMinSize) || w>=hint->min_width)
        && (!(f & PMaxSize) || w<=hint->max_width)
        && (!(f & PBaseSize) || (w-hint->base_width)%hint->width_inc==0));
}

static bool is_prefer_height(int h, const XSizeHints *hint)
{
    long f=hint->flags;
    return !hint || !f ||
        (  (!(f & PMinSize) || h>=hint->min_height)
        && (!(f & PMaxSize) || h<=hint->max_height)
        && (!(f & PBaseSize) || (h-hint->base_height)%hint->height_inc==0));
}

static bool is_prefer_aspect(int w, int h, const XSizeHints *hint)
{
    float minax=hint->min_aspect.x, minay=hint->min_aspect.y, mina=minax/minay,
          maxax=hint->max_aspect.x, maxay=hint->max_aspect.y, maxa=maxax/maxay;

    return !hint || !(hint->flags & PAspect) || !w || !h || !minax || !minay ||
        !maxax || !maxay || ((float)w/h>=mina && (float)w/h<=maxa);
}

void set_input_focus(Window win, const XWMHints *hint)
{
    if(has_focus_hint(hint))
        XSetInputFocus(xinfo.display, win, RevertToPointerRoot, CurrentTime);
    if(has_spec_wm_protocol(win, icccm_atoms[WM_TAKE_FOCUS]))
        send_wm_protocol_msg(icccm_atoms[WM_TAKE_FOCUS], win);
}

bool has_focus_hint(const XWMHints *hint)
{
    // 若未設置hint，則視爲暗示可聚焦
    return !hint || ((hint->flags & InputHint) && hint->input);
}

bool send_wm_protocol_msg(Atom protocol, Window win)
{
    if(has_spec_wm_protocol(win, protocol))
    {
        XEvent event;
        event.type=ClientMessage;
        event.xclient.window=win;
        event.xclient.message_type=icccm_atoms[WM_PROTOCOLS];
        event.xclient.format=32;
        event.xclient.data.l[0]=protocol;
        event.xclient.data.l[1]=CurrentTime;
        return XSendEvent(xinfo.display, win, False, NoEventMask, &event);
    }
    return false;
}

bool has_spec_wm_protocol(Window win, Atom protocol)
{
	int i, n;
	Atom *protocols=NULL;
	if(XGetWMProtocols(xinfo.display, win, &protocols, &n))
        for(i=0; i<n; i++)
            if(protocols[i] == protocol)
                { XFree(protocols); return true; }
    return false;
}

bool set_urgency_hint(Window win, XWMHints *h, bool urg)
{
    if(!h || urg==!!(h->flags & XUrgencyHint)) // 避免重復設置
        return false;

    h->flags = urg ? (h->flags | XUrgencyHint) : (h->flags & ~XUrgencyHint);

    XSetWMHints(xinfo.display, win, h);
    return true;
}

bool is_iconic_state(Window win)
{
    int *p=(int *)get_prop(win, icccm_atoms[WM_STATE], NULL);
    bool result = (p && *p==IconicState);

    XFree(p);

    return result;
}

void close_win(Window win)
{
    if(!send_wm_protocol_msg(icccm_atoms[WM_DELETE_WINDOW], win))
        XKillClient(xinfo.display, win);
}

char *get_wm_name(Window win)
{
    return get_text_prop(win, XA_WM_NAME);
}

char *get_wm_icon_name(Window win)
{
    return get_text_prop(win, XA_WM_ICON_NAME);
}
