/* *************************************************************************
 *     drawable.c：實現與X可畫物相關功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static bool is_wm_win_type(WM *wm, Window win);
static Pixmap create_pixmap_with_color(WM *wm, Drawable d, unsigned long color);
static void change_prop_for_root_bg(WM *wm, Pixmap pixmap);

Window get_transient_for(WM *wm, Window w)
{
    Window pw;
    return XGetTransientForHint(wm->display, w, &pw) ? pw : None;
}

Atom *get_atom_props(WM *wm, Window win, Atom prop, unsigned long *n)
{
    return (Atom *)get_prop(wm, win, prop, n);
}

Atom get_atom_prop(WM *wm, Window win, Atom prop)
{
    Atom *p=get_atom_props(wm, win, prop, NULL), result = p ? *p : None;
    XFree(p);
    return result;
}

/* 調用此函數時需要注意：若返回的特性數據的實際格式是32位的話，
 * 則特性數據存儲於long型數組中。當long是64位時，前4字節會會填充0。 */
unsigned char *get_prop(WM *wm, Window win, Atom prop, unsigned long *n)
{
    int fmt;
    unsigned long nitems=0, *m=(n ? n : &nitems), rest;
    unsigned char *p=NULL;
    Atom type;

    /* 对于XGetWindowProperty，把要接收的数据长度（第5个参数）设置得比实际长度
     * 長可简化代码，这样就不必考虑要接收的數據是否不足32位。以下同理。 */
    if( XGetWindowProperty(wm->display, win, prop, 0, ~0L, False,
        AnyPropertyType, &type, &fmt, m, &rest, &p)==Success && p && *m)
        return p;
    return NULL;
}

char *get_text_prop(WM *wm, Window win, Atom atom)
{
    int n;
    char **list=NULL, *result=NULL;
    XTextProperty name;

    if(!XGetTextProperty(wm->display, win, &name, atom))
        return NULL;

    if(name.encoding == XA_STRING)
        result=copy_string((char *)name.value);
    else if(Xutf8TextPropertyToTextList(wm->display, &name, &list, &n) == Success
        && n && *list) // 與手冊不太一致的是，返回Success並不一定真的成功，狗血！
        result=copy_string(*list), XFreeStringList(list);
    XFree(name.value);
    return result;
}

char *get_title_text(WM *wm, Window win, const char *fallback)
{
    char *s=NULL;

    if((s=get_text_prop(wm, win, wm->ewmh_atom[NET_WM_NAME])) && strlen(s))
        return s;
    if((s=get_text_prop(wm, win, XA_WM_NAME)) && strlen(s))
        return s;
    return copy_string(fallback);
}

char *get_icon_title_text(WM *wm, Window win, const char *fallback)
{
    char *s=NULL;

    if((s=get_text_prop(wm, win, wm->ewmh_atom[NET_WM_ICON_NAME])) && strlen(s))
        return s;
    if((s=get_text_prop(wm, win, XA_WM_ICON_NAME)) && strlen(s))
        return s;
    return copy_string(fallback);
}

void copy_prop(WM *wm, Window dest, Window src)
{
    int i=0, n=0, fmt=0;
    Atom type, *p=XListProperties(wm->display, src, &n);

    if(!p)
        return;

    unsigned long total, rest;
    for(unsigned char *prop=NULL; i<n; i++, prop && XFree(prop))
        if( XGetWindowProperty(wm->display, src, p[i], 0, ~0L, False,
            AnyPropertyType, &type, &fmt, &total, &rest, &prop) == Success)
            XChangeProperty(wm->display, dest, p[i], type, fmt,
                PropModeReplace, prop, total);
    XFree(p);
}

bool send_event(WM *wm, Atom protocol, Window win)
{
	int i, n;
	Atom *protocols;

	if(!XGetWMProtocols(wm->display, win, &protocols, &n))
        return false;

    XEvent event;
    for(i=0; i<n && protocols[i]!=protocol; i++)
        ;
    XFree(protocols);
    if(i < n)
    {
        event.type=ClientMessage;
        event.xclient.window=win;
        event.xclient.message_type=wm->icccm_atoms[WM_PROTOCOLS];
        event.xclient.format=32;
        event.xclient.data.l[0]=protocol;
        event.xclient.data.l[1]=CurrentTime;
        XSendEvent(wm->display, win, False, NoEventMask, &event);
    }
    return i<n;
}

bool is_pointer_on_win(WM *wm, Window win)
{
    Window r, c;
    int rx, ry, x, y, w, h;
    unsigned int mask;
    
    return get_geometry(wm, win, NULL, NULL, &w, &h, NULL, NULL)
        && XQueryPointer(wm->display, win, &r, &c, &rx, &ry, &x, &y, &mask)
        && x>=0 && x<w && y>=0 && y<h;
}

/* 通過求窗口與屏幕是否有交集來判斷窗口是否已經在屏幕外。
 * 若滿足以下條件，則有交集：窗口與屏幕中心距≤窗口半邊長+屏幕半邊長。
 * 即：|x+w/2-0-sw/2|＜|w/2+sw/2| 且 |y+h/2-0-sh/2|＜|h/2+sh/2|。
 * 兩邊同乘以2，得：|2*x+w-sw|＜|w+sw| 且 |2*y+h-sh|＜|h+sh|。
 */
bool is_on_screen(WM *wm, int x, int y, int w, int h)
{
    long sw=wm->screen_width, sh=wm->screen_height, wl=w, hl=h;
    return labs(2*x+wl-sw)<wl+sw && labs(2*y+hl-sh)<hl+sh;
}

void print_area(WM *wm, Drawable d, int x, int y, int w, int h)
{
    imlib_context_set_drawable(d);
    Imlib_Image image=imlib_create_image_from_drawable(None, x, y, w, h, 0);

    if(!image)
        return;

    time_t timer=time(NULL), err=-1;
    char name[FILENAME_MAX];

    if(wm->cfg->screenshot_path[0] == '~')
        sprintf(name, "%s%s/gwm-", getenv("HOME"), wm->cfg->screenshot_path+1);
    else
        sprintf(name, "%s/gwm-", wm->cfg->screenshot_path);
    if(timer != err)
        strftime(name+strlen(name), FILENAME_MAX, "%Y_%m_%d_%H_%M_%S", localtime(&timer));
    set_visual_for_imlib(wm, d);
    imlib_context_set_image(image);
    imlib_image_set_format(wm->cfg->screenshot_format);
    sprintf(name+strlen(name), ".%s", wm->cfg->screenshot_format);
    imlib_save_image(name);
    imlib_free_image();
}

bool is_wm_win(WM *wm, Window win, bool before_wm)
{
    XWindowAttributes a;
    bool status=XGetWindowAttributes(wm->display, win, &a);
    if(!status || a.override_redirect || !is_wm_win_type(wm, win))
        return false;

    if(!before_wm)
        return !win_to_client(wm, win);

    unsigned char *p=get_prop(wm, win, wm->icccm_atoms[WM_STATE], NULL);
    bool result=((p && (*(unsigned long *)p)==IconicState) || a.map_state==IsViewable);
    XFree(p);
    return result;
}

static bool is_wm_win_type(WM *wm, Window win)
{
    Net_wm_win_type type=get_net_wm_win_type(wm, win);
    return(type.utility || type.dialog || type.normal || type.none);
}

/* 當存在合成器時，合成器會在根窗口上放置特效，即使用XSetWindowBackground*設置
 * 了背景，也會被合成器的特效擋着，目前還沒有標準的方法來設置背景。要給根窗口
 * 設置顏色，就得借助pixmap。這種情況下，若要真正地設置背景，得用E方法。這種方
 * 法是事實上的標準，它通過設置非標準的窗口特性_XROOTPMAP_ID和ESETROOT_PMAP_ID
 * 來達到此目的。通常設置其中之一便可，爲了保險起見，通常同時設置。這種方法的一
 * 個弊端是，不能在設置完背景後馬上釋放pixmap，否則背景設置失效，這會佔用一定的
 * 內存空間。修改背景時，應通過XKillClient來釋放舊的pixmap。通常_XROOTPMAP_ID和
 * ESETROOT_PMAP_ID特性指向相同的pixmap，有非標準化的文檔說兩者指向相同的pixmap
 * 時才應釋放它。詳見：
 *     https://metacpan.org/pod/X11::Protocol::XSetRoot
 *     https://lists.gnome.org/archives/wm-spec-list/2002-January/msg00003.html
 *     https://mail.gnome.org/archives/wm-spec-list/2002-January/msg00011.html
 */
void update_win_bg(WM *wm, Window win, unsigned long color, Pixmap pixmap)
{
    XEvent event={.xexpose={.type=Expose, .window=win}};
    bool compos_root = (win==wm->root_win && have_compositor(wm));

    if(compos_root && !pixmap)
        pixmap=create_pixmap_with_color(wm, win, color);

    if(pixmap)
        XSetWindowBackgroundPixmap(wm->display, win, pixmap);
    else
        XSetWindowBackground(wm->display, win, color);

    /* XSetWindowBackgroundPixmap或XSetWindowBackground不改變窗口當前內容，
       應通過發送顯露事件或調用XClearWindow來立即改變背景。*/
    if(pixmap || win==wm->root_win)
        XClearWindow(wm->display, win);
    else
        XSendEvent(wm->display, win, False, NoEventMask, &event);

    if(compos_root)
        change_prop_for_root_bg(wm, pixmap);
}

static Pixmap create_pixmap_with_color(WM *wm, Drawable d, unsigned long color)
{
    int w, h, red=(color & 0x00ff0000UL)>>16, green=(color & 0x0000ff00UL)>>8,
        blue=(color & 0x000000ffUL), alpha=(color & 0xff000000UL)>>24;
    unsigned int depth;
    Imlib_Image image=NULL;

    if( !get_geometry(wm, d, NULL, NULL, &w, &h, NULL, &depth)
        || !(image=imlib_create_image(w, h)))
        return None;

    Pixmap pixmap=XCreatePixmap(wm->display, d, w, h, depth);
    if(!pixmap)
        return None;

    set_visual_for_imlib(wm, d);
    imlib_context_set_image(image);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color(red, green, blue , alpha);
    imlib_image_fill_rectangle(0, 0, w, h);
    imlib_render_image_on_drawable(0, 0);
    imlib_free_image();
    return pixmap;
}

static void change_prop_for_root_bg(WM *wm, Pixmap pixmap)
{
    Window win=wm->root_win;
    Atom prop_root=XInternAtom(wm->display, "_XROOTPMAP_ID", True);
    Atom prop_esetroot=XInternAtom(wm->display, "ESETROOT_PMAP_ID", True);

    if(prop_root && prop_esetroot)
    {
        unsigned char *rdata=get_prop(wm, win, prop_root, NULL),
                      *edata=get_prop(wm, win, prop_esetroot, NULL);
        Pixmap rid = rdata ? *((Pixmap *)rdata) : None,
               eid = edata ? *((Pixmap *)edata) : None;

        XFree(rdata), XFree(edata);
        if(rid && eid && eid!=rid)
            XKillClient(wm->display, rid);
    }

    prop_root=XInternAtom(wm->display, "_XROOTPMAP_ID", False);
    prop_esetroot=XInternAtom(wm->display, "ESETROOT_PMAP_ID", False);
    XChangeProperty(wm->display, win, prop_root, XA_PIXMAP, 32,
        PropModeReplace, (unsigned char *)&pixmap, 1);
    XChangeProperty(wm->display, win, prop_esetroot, XA_PIXMAP, 32,
        PropModeReplace, (unsigned char *)&pixmap, 1);
}

void set_override_redirect(WM *wm, Window win)
{
    XSetWindowAttributes attr={.override_redirect=True};
    XChangeWindowAttributes(wm->display, win, CWOverrideRedirect, &attr);
}

bool get_geometry(WM *wm, Drawable drw, int *x, int *y, int *w, int *h, int *bw, unsigned int *depth)
{
    Window r;
    int xt, yt;
    unsigned int wt, ht, bwt, dt;

    return XGetGeometry(wm->display, drw, &r, x ? x : &xt, y ? y : &yt,
        w ? (unsigned int *)w : &wt, h ? (unsigned int *)h : &ht,
        bw ? (unsigned int *)bw : &bwt, depth ? depth : &dt);
}

/* 坐標均相對於根窗口, 後四個參數是將要彈出的窗口的坐標和尺寸 */
void set_pos_for_click(WM *wm, Window click, int cx, int *px, int *py, int pw, int ph)
{
    int x=0, y=0, w=0, h=0, bw=0, sw=wm->screen_width, sh=wm->screen_height;
    Window child, root=wm->root_win;

    XTranslateCoordinates(wm->display, click, root, 0, 0, &x, &y, &child);
    get_geometry(wm, click, NULL, NULL, &w, &h, &bw, NULL);
    // 優先考慮右邊顯示彈窗；若不夠位置，則考慮左邊顯示；再不濟則從屏幕左邊開始顯示
    *px = cx+pw<sw ? cx : (cx-pw>0 ? cx-pw : 0);
    /* 優先考慮下邊顯示彈窗；若不夠位置，則考慮上邊顯示；再不濟則從屏幕上邊開始顯示。
       並且彈出窗口與點擊窗口錯開一個像素，以便從視覺上有所區分。*/
    *py = y+(h+bw+ph)<sh ? y+h+bw+1: (y-bw-ph>0 ? y-bw-ph-1 : 0);
}

bool is_win_exist(WM *wm, Window win, Window parent)
{
    Window root, pwin, *child=NULL;
    unsigned int n;

    if(XQueryTree(wm->display, parent, &root, &pwin, &child, &n))
        for(unsigned int i=0; i<n; i++)
            if(win == child[i])
                { XFree(child); return true; }
    return false;
}

Pixmap create_pixmap_from_file(WM *wm, Window win, const char *filename)
{
    int w, h;
    unsigned int d;
    Imlib_Image image=imlib_load_image(filename);

    if(!image || !get_geometry(wm, win, NULL, NULL, &w, &h, NULL, &d))
        return None;

    Pixmap bg=XCreatePixmap(wm->display, win, w, h, d);
    set_visual_for_imlib(wm, win);
    imlib_context_set_image(image);
    imlib_context_set_drawable(bg);   
    imlib_render_image_on_drawable_at_size(0, 0, w, h);
    imlib_free_image();
    return bg;
}

void close_win(WM *wm, Window win)
{
    if(!send_event(wm, wm->icccm_atoms[WM_DELETE_WINDOW], win))
        XDestroyWindow(wm->display, win);
}

Window create_widget_win(WM *wm, Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel)
{
    XSetWindowAttributes attr;
    attr.colormap=wm->colormap;
    attr.border_pixel=border_pixel;
    attr.background_pixel=bg_pixel;
    attr.override_redirect=True;
    return XCreateWindow(wm->display, parent, x, y, w, h, border_w, wm->depth,
        InputOutput, wm->visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);
}

void set_visual_for_imlib(WM *wm, Drawable d)
{
    if(d == wm->root_win)
        imlib_context_set_visual(DefaultVisual(wm->display, wm->screen));
    else
        imlib_context_set_visual(wm->visual);
}

void restack_win(WM *wm, Window win)
{
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        if(win==wm->top_wins[i])
            return;

    Client *c=win_to_client(wm, win);
    Net_wm_win_type type=get_net_wm_win_type(wm, win);
    Window wins[2]={None, c ? c->frame : win};

    if(type.desktop)
        wins[0]=wm->top_wins[DESKTOP_TOP];
    else if(type.dock)
        wins[0]=wm->top_wins[DOCK_TOP];
    else if(c)
    {
        raise_client(wm, c);
        return;
    }
    else
        wins[0]=wm->top_wins[NORMAL_TOP];
    XRestackWindows(wm->display, wins, 2);
}

/* 根據EWMH，窗口可能有多種類型，但實際上絕大部分窗口只設置一種類型 */
Net_wm_win_type get_net_wm_win_type(WM *wm, Window win)
{
    Net_wm_win_type result={0}, unknown={.none=1};
    unsigned long n=0;
    Atom *types=get_atom_props(wm, win, wm->ewmh_atom[NET_WM_WINDOW_TYPE], &n);

    if(!types)
        return unknown;

    for(unsigned long i=0; i<n; i++)
    {
        if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DESKTOP])
            result.desktop=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DOCK])
            result.dock=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_TOOLBAR])
            result.toolbar=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_MENU])
            result.menu=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_UTILITY])
            result.utility=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_SPLASH])
            result.splash=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DIALOG])
            result.dialog=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DROPDOWN_MENU])
            result.dropdown_menu=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_POPUP_MENU])
            result.popup_menu=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_TOOLTIP])
            result.tooltip=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_NOTIFICATION])
            result.notification=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_COMBO])
            result.combo=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DND])
            result.dnd=1;
        else if(types[i] == wm->ewmh_atom[NET_WM_WINDOW_TYPE_NORMAL])
            result.normal=1;
        else
            result.none=1;
    }
    XFree(types);

    return result;
}

/* EWMH未說明窗口可否同時有多種狀態，但實際上絕大部分窗口不設置或只設置一種 */
Net_wm_state get_net_wm_state(WM *wm, Window win)
{
    Net_wm_state result={0};
    unsigned long n=0;
    Atom *states=get_atom_props(wm, win, wm->ewmh_atom[NET_WM_STATE], &n);

    if(!states)
        return result;

    for(unsigned long i=0; i<n; i++)
    {
        if(states[i] == wm->ewmh_atom[NET_WM_STATE_MODAL])
            result.modal=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_STICKY])
            result.sticky=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_MAXIMIZED_VERT])
            result.vmax=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_MAXIMIZED_HORZ])
            result.hmax=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_SHADED])
            result.shaded=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_SKIP_TASKBAR])
            result.skip_taskbar=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_SKIP_PAGER])
            result.skip_pager=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_HIDDEN])
            result.hidden=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_FULLSCREEN])
            result.fullscreen=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_ABOVE])
            result.above=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_BELOW])
            result.below=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_DEMANDS_ATTENTION])
            result.attent=1;
        else if(states[i] == wm->ewmh_atom[NET_WM_STATE_FOCUSED])
            result.focused=1;
    }
    XFree(states);

    return result;
}
