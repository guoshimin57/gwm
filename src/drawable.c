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

Window get_transient_for(WM *wm, Window w)
{
    Window pw;
    return XGetTransientForHint(wm->display, w, &pw) ? pw : None;
}

Atom get_atom_prop(WM *wm, Window win, Atom prop)
{
    unsigned char *p=get_prop(wm, win, prop, NULL);
    return p ? *(Atom *)p : None;
}

/* 調用此函數時需要注意：若返回的特性數據的實際格式是32位的話，
 * 則特性數據存儲於long型數組中。當long是64位時，前4字節會會填充0。 */
unsigned char *get_prop(WM *wm, Window win, Atom prop, unsigned long *n)
{
    int fmt;
    unsigned long nitems, rest;
    unsigned char *p=NULL;
    Atom type;

    /* 对于XGetWindowProperty，把要接收的数据长度（第5个参数）设置得比实际长度
     * 長可简化代码，这样就不必考虑要接收的數據是否不足32位。以下同理。 */
    if( XGetWindowProperty(wm->display, win, prop, 0, ~0L, False,
        AnyPropertyType, &type, &fmt, n ? n : &nitems, &rest, &p)==Success && p)
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
    imlib_context_set_image(image);
    imlib_image_set_format(wm->cfg->screenshot_format);
    sprintf(name+strlen(name), ".%s", wm->cfg->screenshot_format);
    imlib_save_image(name);
    imlib_free_image();
}

bool is_wm_win(WM *wm, Window win, bool before_wm)
{
    XWindowAttributes a;
    if( !XGetWindowAttributes(wm->display, win, &a) || a.override_redirect
        || !is_wm_win_type(wm, win))
        return false;
    if(before_wm)
        return a.map_state == IsViewable
            || get_atom_prop(wm, win, wm->icccm_atoms[WM_STATE]) == IconicState;
    else
        return !win_to_client(wm, win);
}

static bool is_wm_win_type(WM *wm, Window win)
{
    Atom type=get_atom_prop(wm, win, wm->ewmh_atom[NET_WM_WINDOW_TYPE]);
    return(type == None
        || type == wm->ewmh_atom[NET_WM_WINDOW_TYPE_NORMAL]
        || type == wm->ewmh_atom[NET_WM_WINDOW_TYPE_UTILITY]
        || type == wm->ewmh_atom[NET_WM_WINDOW_TYPE_DIALOG]);
}

void update_win_bg(WM *wm, Window win, unsigned long color, Pixmap pixmap)
{
    XEvent event={.xexpose={.type=Expose, .window=win}};
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
    Window child;

    XTranslateCoordinates(wm->display, click, wm->root_win, 0, 0, &x, &y, &child);
    get_geometry(wm, click, NULL, NULL, &w, &h, &bw, NULL);
    // 優先考慮右邊顯示彈窗；若不夠位置，則考慮左邊顯示；再不濟則從屏幕左邊開始顯示
    *px = cx+(int)pw<(int)sw ? cx : (cx-(int)pw>0 ? cx-(int)pw : 0);
    /* 優先考慮下邊顯示彈窗；若不夠位置，則考慮上邊顯示；再不濟則從屏幕上邊開始顯示。
       並且彈出窗口與點擊窗口錯開一個像素，以便從視覺上有所區分。*/
    *py = y+(int)(h+bw+ph)<(int)sh ? y+(int)(h+bw)+1 : (y-(int)(bw+ph)>0 ? y-(int)(bw+ph)-1 : 0);
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
    imlib_context_set_image(image);
    imlib_context_set_drawable(bg);   
    imlib_render_image_on_drawable_at_size(0, 0, w, h);
    imlib_free_image();
    return bg;
}

void show_tooltip(WM *wm, Window hover)
{
    Widget_type type=get_widget_type(wm, hover);
    const char *s=NULL;

    switch(type)
    {
        case CLIENT_ICON: s=win_to_iconic_state_client(wm, hover)->icon->title_text; break;
        case TITLE_AREA: s=win_to_client(wm, hover)->title_text; break;
        default: s=wm->cfg->tooltip[type]; break;
    }

    if(s)
        update_hint_win_for_info(wm, hover, s);
}

void close_win(WM *wm, Window win)
{
    if(!send_event(wm, wm->icccm_atoms[WM_DELETE_WINDOW], win))
        XDestroyWindow(wm->display, win);
}
