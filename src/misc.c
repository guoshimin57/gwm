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
#include <dirent.h>
#include <stdarg.h>
#include "gwm.h"
#include "client.h"
#include "font.h"
#include "misc.h"

static size_t get_files_in_path(const char *path, const char *regex, File *head, Order order, bool is_fullname);
static bool match(const char *s, const char *r, const char *os);
static int cmp_basename(const char *s1, const char *s2);
static void create_file_node(File *head, const char *path, char *filename, bool is_fullname);

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

void update_win_background(WM *wm, Window win, unsigned long color, Pixmap pixmap)
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

Pixmap create_pixmap_from_file(WM *wm, Window win, const char *filename)
{
    unsigned int w, h, d;
    Imlib_Image image=imlib_load_image(filename);
    if(image && get_geometry(wm, win, NULL, NULL, &w, &h, &d))
    {
        Pixmap bg=XCreatePixmap(wm->display, win, w, h, d);
        imlib_context_set_image(image);
        imlib_context_set_drawable(bg);   
        imlib_render_image_on_drawable_at_size(0, 0, w, h);
        imlib_free_image();
        return bg;
    }
    return None;
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
    XFreeGC(wm->display, wm->gc);
    XFreeModifiermap(wm->mod_map);
    for(size_t i=0; i<POINTER_ACT_N; i++)
        XFreeCursor(wm->display, wm->cursors[i]);
    XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
    XDestroyIC(wm->run_cmd.xic);
    XCloseIM(wm->xim);
    close_fonts(wm);
    free_files(wm->wallpapers);
    XClearWindow(wm->display, wm->root_win);
    XFlush(wm->display);
    XCloseDisplay(wm->display);
    clear_zombies(0);
}

bool get_geometry(WM *wm, Drawable drw, int *x, int *y, unsigned int *w, unsigned int *h, unsigned int *depth)
{
    Window r;
    int xt, yt;
    unsigned int wt, ht, bw, dt;
    return XGetGeometry(wm->display, drw, &r, x ? x : &xt, y ? y : &yt,
        w ? w : &wt, h ? h : &ht, &bw, depth ? depth : &dt);
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
    unsigned int cw=0, ch=0, sw=wm->screen_width, sh=wm->screen_height;

    get_geometry(wm, click, NULL, NULL, &cw, &ch, NULL);

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

File *get_files_in_paths(const char *paths, const char *regex, Order order, bool is_fullname, size_t *n)
{
    size_t sum=0;
    char *p, *ps=copy_string(paths);
    File *head=malloc_s(sizeof(File));
    head->next=NULL, head->name=NULL;
    for(p=strtok(ps, ":"); p; p=strtok(NULL, ":"))
        sum+=get_files_in_path(p, regex, head, order, is_fullname);
    free(ps);
    if(n)
        *n=sum;
    return head;
}

static size_t get_files_in_path(const char *path, const char *regex, File *head, Order order, bool is_fullname)
{
    size_t n=0;
    char *fn;
    DIR *dir=opendir(path);
    for(struct dirent *d=NULL; dir && (d=readdir(dir));)
        if(strcmp(".", fn=d->d_name) && strcmp("..", fn) && regcmp(fn, regex))
            for(File *f=head; f; f=f->next)
                if(!order || !f->next || cmp_basename(fn, f->next->name)/order>0)
                    { create_file_node(f, path, fn, is_fullname); n++; break; }
    if(dir)
        closedir(dir);
    return n;
}

void free_files(File *head)
{
    for(File *f=head; f; f=head)
        head=f->next, free(f->name), free(f);
}

// *匹配>=0個字符，.匹配一個字符，|匹配其兩側的表達式
static bool match(const char *s, const char *r, const char *os)
{
    char *p=NULL;
    switch(*r)
    {
        case '\0':  return !*s;
        case '*':   return match(s, r+1, os) || (*s && match(s+1, r, os));
        case '.':   return *s && match(s+1, r+1, os);
        case '|':   return (!*(r+1) && !*s) || (*(r+1) && match(os, r+1, os));
        default:    return (*r==*s && match(s+1, r+1, os))
                        || ((p=strchr(r, '|')) && match(os, p+1, os));
    }
}

bool regcmp(const char *s, const char *regex) // 簡單的全文(即整個s)正則表達式匹配
{
    return match(s, regex, s);
}

static int cmp_basename(const char *s1, const char *s2)
{
    const char *p1=strrchr(s1, '/'), *p2=strrchr(s2, '/');
    p1=(p1 ? p1+1 : s1), p2=(p2 ? p2+1 : s2);
    return strcmp(p1, p2);
}

static void create_file_node(File *head, const char *path, char *filename, bool is_fullname)
{
    File *file=malloc_s(sizeof(File));
    file->name = is_fullname ? copy_strings(path, "/", filename, NULL) : copy_string(filename);
    file->next=head->next;
    head->next=file;
}
