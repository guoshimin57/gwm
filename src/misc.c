/* *************************************************************************
 *     misc.c：雜項。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include "gwm.h"
#include "client.h"
#include "font.h"
#include "misc.h"

void *malloc_s(size_t size)
{
    void *p=malloc(size);
    if(p == NULL)
        exit_with_msg("錯誤：申請內存失敗");
    return p;
}

int x_fatal_handler(Display *display, XErrorEvent *e)
{
    unsigned char ec=e->error_code, rc=e->request_code;
    if(rc==X_ChangeWindowAttributes && ec==BadAccess)
        exit_with_msg("錯誤：已經有其他窗口管理器在運行！");
    fprintf(stderr, "X錯誤：資源號=%#lx, 請求量=%lu, 錯誤碼=%d, 主請求碼=%d, "
            "次請求碼=%d\n", e->resourceid, e->serial, ec, rc, e->minor_code);
	if(ec == BadWindow || (rc==X_ConfigureWindow && ec==BadMatch))
		return -1;
    return 0;
}

void exit_with_perror(const char *s)
{
    perror(s);
    exit(EXIT_FAILURE);
}

void exit_with_msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

bool is_wm_win(WM *wm, Window win)
{
    XWindowAttributes attr;
    return (XGetWindowAttributes(wm->display, win, &attr)
        && attr.map_state!=IsUnmapped && !attr.override_redirect);
}

/* 在調用XSetWindowBackground之後，在收到下一個顯露事件或調用XClearWindow
 * 之前，背景不變。此處用發送顯露事件的方式使背景設置立即生效。*/
void update_win_background(WM *wm, Window win, unsigned long color)
{
    XEvent event={.xexpose={.type=Expose, .window=win}};
    XSetWindowBackground(wm->display, win, color);
    XSendEvent(wm->display, win, False, NoEventMask, &event);
}

Widget_type get_widget_type(WM *wm, Window win)
{
    Widget_type type;
    Client *c;
    if(win == wm->root_win)
        return ROOT_WIN;
    if(win == wm->run_cmd.win)
        return RUN_CMD_ENTRY;
    if(win == wm->hint_win)
        return HINT_WIN;
    for(type=TASKBAR_BUTTON_BEGIN; type<=TASKBAR_BUTTON_END; type++)
        if(win == wm->taskbar.buttons[TASKBAR_BUTTON_INDEX(type)])
            return type;
    if(win == wm->taskbar.status_area)
        return STATUS_AREA;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(c->area_type==ICONIFY_AREA && win==c->icon->win)
            return CLIENT_ICON;
    for(type=CMD_CENTER_ITEM_BEGIN; type<=CMD_CENTER_ITEM_END; type++)
        if(win == wm->cmd_center.items[CMD_CENTER_ITEM_INDEX(type)])
            return type;
    if((c=win_to_client(wm, win)))
    {
        if(win == c->win)
            return CLIENT_WIN;
        else if(win == c->frame)
            return CLIENT_FRAME;
        else if(win == c->title_area)
            return TITLE_AREA;
        else
            for(type=TITLE_BUTTON_BEGIN; type<=TITLE_BUTTON_END; type++)
                if(win == c->buttons[TITLE_BUTTON_INDEX(type)])
                    return type;
    }
    return UNDEFINED;
}

Pointer_act get_resize_act(Client *c, const Move_info *m)
{   // 窗口邊框寬度、標題欄調試、可調整尺寸區域的寬度、高度
    // 以及窗口框架左、右橫坐標和上、下縱坐標
    int bw=c->border_w, bh=c->title_bar_h, rw=c->w/4, rh=c->h/4,
        lx=c->x-bw, rx=c->x+c->w+bw, ty=c->y-bh-bw, by=c->y+c->h+bw;

    if(m->ox>=lx && m->ox<lx+bw+rw && m->oy>=ty && m->oy<ty+bw+rh)
        return TOP_LEFT_RESIZE;
    else if(m->ox>=rx-bw-rw && m->ox<rx && m->oy>=ty && m->oy<ty+bw+rh)
        return TOP_RIGHT_RESIZE;
    else if(m->ox>=lx && m->ox<lx+bw+rw && m->oy>=by-bw-rh && m->oy<by)
        return BOTTOM_LEFT_RESIZE;
    else if(m->ox>=rx-bw-rw && m->ox<rx && m->oy>=by-bw-rh && m->oy<by)
        return BOTTOM_RIGHT_RESIZE;
    else if(m->oy>=ty && m->oy<ty+bw+rh)
        return TOP_RESIZE;
    else if(m->oy>=by-bw-rh && m->oy<by)
        return BOTTOM_RESIZE;
    else if(m->ox>=lx && m->ox<lx+bw+rw)
        return LEFT_RESIZE;
    else if(m->ox>=rx-bw-rw && m->ox<rx)
        return RIGHT_RESIZE;
    else
        return NO_OP;
}

void clear_zombies(int unused)
{
	while(0 < waitpid(-1, NULL, WNOHANG))
        ;
}

bool is_chosen_button(WM *wm, Widget_type type)
{
    return(type == DESKTOP_BUTTON_BEGIN+wm->cur_desktop-1
        || type == LAYOUT_BUTTON_BEGIN+DESKTOP(wm).cur_layout);
}

void set_xic(WM *wm, Window win, XIC *ic)
{
    if(wm->xim == NULL)
        return;
    if((*ic=XCreateIC(wm->xim, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
        XNClientWindow, win, NULL)) == NULL)
        fprintf(stderr, "錯誤：窗口（0x%lx）輸入法設置失敗！", win);
    else
        XSetICFocus(*ic);
}

Window get_transient_for(WM *wm, Window w)
{
    Window pw;
    return XGetTransientForHint(wm->display, w, &pw) ? pw : None;
}

KeySym look_up_key(XIC xic, XKeyEvent *e, wchar_t *keyname, size_t n)
{
	KeySym ks;
    if(xic)
        XwcLookupString(xic, e, keyname, n, &ks, 0);
    else
    {
        char kn[n];
        XLookupString(e, kn, n, &ks, 0);
        mbstowcs(keyname, kn, n);
    }
    return ks;
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

void set_override_redirect(WM *wm, Window win)
{
    XSetWindowAttributes attr={.override_redirect=True};
    XChangeWindowAttributes(wm->display, win, CWOverrideRedirect, &attr);
}

void clear_wm(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        XReparentWindow(wm->display, c->win, wm->root_win, c->x, c->y);
        del_client(wm, c, false);
    }
    XDestroyWindow(wm->display, wm->taskbar.win);
    free(wm->taskbar.status_text);
    XDestroyWindow(wm->display, wm->cmd_center.win);
    XDestroyWindow(wm->display, wm->run_cmd.win);
    XDestroyWindow(wm->display, wm->hint_win);
    XFreeModifiermap(wm->mod_map);
    for(size_t i=0; i<POINTER_ACT_N; i++)
        XFreeCursor(wm->display, wm->cursors[i]);
    XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
    XDestroyIC(wm->run_cmd.xic);
    XCloseIM(wm->xim);
    close_fonts(wm);
    XClearWindow(wm->display, wm->root_win);
    XFlush(wm->display);
    XCloseDisplay(wm->display);
    clear_zombies(0);
}

void get_drawable_size(WM *wm, Drawable drw, unsigned int *w, unsigned int *h)
{
    Window r;
    int xt, yt;
    unsigned int bw, d;
    if(!XGetGeometry(wm->display, drw, &r, &xt, &yt, w, h, &bw, &d))
        *w=*h=0;
}

char *copy_string(const char *s)
{
    return strcpy(malloc_s(strlen(s)+1), s);
}

char *copy_strings(const char *s, ...) // 調用時須以NULL結尾
{
    if(!s)
        return NULL;

    char *result=NULL, *p=NULL;
    size_t len=strlen(s);
    va_list ap;
    va_start(ap, s);
    while((p=va_arg(ap, char *)))
        len+=strlen(p);
    va_end(ap);
    if((result=malloc(len+1)) == NULL)
        return NULL;
    strcpy(result, s);
    va_start(ap, s);
    while((p=va_arg(ap, char *)))
        strcat(result, p);
    va_end(ap);
    return result;
}

/* 坐標均對於根窗口, 後四個參數是將要彈出的窗口的坐標和尺寸 */
void set_pos_for_click(WM *wm, Window click, int cx, int cy, int *px, int *py, unsigned int pw, unsigned int ph)
{
    unsigned int cw, ch, sw=wm->screen_width, sh=wm->screen_height;

    get_drawable_size(wm, click, &cw, &ch);

    if(cx < 0) // 窗口click左邊出屏
        cw=cx+pw, cx=0;
    if(cx+cw > sw) // 窗口click右邊出屏
        cw=sw-cx;

    if(cx+pw <= sw) // 在窗口click的右邊能顯示完整的菜單
        *px=cx;
    else if(cx+cw >= pw) // 在窗口click的左邊能顯示完整的菜單
        *px=cx+cw-pw;
    else if(cx+cw/2 <= sw/2) // 窗口click在屏幕的左半部
        *px=sw-pw;
    else // 窗口click在屏幕的右半部
        *px=0;

    if(cy+ch+ph <= sh) // 在窗口click下能顯示完整的菜單
        *py=cy+ch;
    else if(cy >= ph) // 在窗口click上能顯示完整的菜單
        *py=cy-ph;
    else if(cy+ch/2 <= sh/2) // 窗口click在屏幕的上半部
        *py=sh-ph;
    else // 窗口click在屏幕的下半部
        *py=0;
}

bool is_win_exist(WM *wm, Window win, Window parent)
{
    Window root, pwin, *child=NULL;
    unsigned int n;
    if(XQueryTree(wm->display, parent, &root, &pwin, &child, &n))
        for(size_t i=0; i<n; i++)
            if(win == child[i])
                { XFree(child); return true; }
    return false;
}
