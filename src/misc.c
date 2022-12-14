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
#include "drawable.h"
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
