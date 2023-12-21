/* *************************************************************************
 *     misc.c：雜項。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static char *dedup_paths(const char *paths);
static size_t get_files_in_path(const char *path, const char *regex, Strings *head, Order order, bool is_fullname);
static bool match(const char *s, const char *r, const char *os);
static bool regcmp(const char *s, const char *regex);
static int cmp_basename(const char *s1, const char *s2);
static void create_file_node(Strings *head, const char *path, char *filename, bool is_fullname);

void *malloc_s(size_t size)
{
    void *p=malloc(size);
    if(p == NULL)
        exit_with_msg(_("錯誤：申請內存失敗"));
    return p;
}

int x_fatal_handler(Display *display, XErrorEvent *e)
{
    UNUSED(display);
    unsigned char ec=e->error_code, rc=e->request_code;

    if(rc==X_ChangeWindowAttributes && ec==BadAccess)
        exit_with_msg(_("錯誤：已經有其他窗口管理器在運行！"));
    if(ec == BadWindow) // Xlib沒提供檢測窗口是否已經被銷毀的API，故忽略這種錯誤
        return 0;
    if(rc==X_ConfigureWindow && ec==BadMatch)
		return -1;
    fprintf(stderr, _("X錯誤：資源號=%#lx, 請求量=%lu, 錯誤碼=%d, 主請求碼=%d, 次請求碼=%d\n"),
        e->resourceid, e->serial, ec, rc, e->minor_code);
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

Pointer_act get_resize_act(Client *c, const Move_info *m)
{   // 窗口邊框寬度、標題欄調試、可調整尺寸區域的寬度、高度
    // 以及窗口框架左、右橫坐標和上、下縱坐標
    int bw=c->border_w, bh=c->titlebar_h, rw=c->w/4, rh=c->h/4,
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

void clear_zombies(int signum)
{
    UNUSED(signum);
	while(0 < waitpid(-1, NULL, WNOHANG))
        ;
}

void set_xic(Window win, XIC *ic)
{
    if(xinfo.xim == NULL)
        return;
    if((*ic=XCreateIC(xinfo.xim, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
        XNClientWindow, win, NULL)) == NULL)
        fprintf(stderr, _("錯誤：窗口（0x%lx）輸入法設置失敗！"), win);
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

void vfree(void *ptr, ...) // 調用時須以NULL結尾
{
    va_list ap;
    va_start(ap, ptr);
    for(void *p=ptr; p; p=va_arg(ap, void *))
        free(p);
    va_end(ap);
}

Strings *get_files_in_paths(const char *paths, const char *regex, Order order, bool is_fullname, int *n)
{
    int sum=0;
    char *p=NULL, *ps=dedup_paths(paths);
    Strings *head=malloc_s(sizeof(Strings));

    head->next=NULL, head->str=NULL;
    for(p=strtok(ps, ":"); p; p=strtok(NULL, ":"))
        sum+=get_files_in_path(p, regex, head, order, is_fullname);
    free(ps);
    if(n)
        *n=sum;
    return head;
}

static char *dedup_paths(const char *paths)
{
    ssize_t readlink(const char *restrict, char *restrict, size_t);
    size_t n=1, i=0, j=0, len=strlen(paths);
    char *p=NULL, *ps=copy_string(paths);

    for(p=ps; *p; p++)
        if(*p == ':')
            n++;

    char list[n][len+1], buf[len+1], *result=malloc_s(len+1);
    for(*result='\0', p=strtok(ps, ":"); p; p=strtok(NULL, ":"))
    {
        ssize_t count=readlink(p, buf, len);
        char *pp = count>0 ? buf : p;
        for(j=0; j<i; j++)
            if(strcmp(pp, list[j])==0)
                break;
        if(j==i)
            strcpy(list[i++], pp), strcat(result, pp), strcat(result, i<n ? ":" : "");
    }
    free(ps);
    return result;
}

static size_t get_files_in_path(const char *path, const char *regex, Strings *head, Order order, bool is_fullname)
{
    size_t n=0;
    char *fn;
    DIR *dir=opendir(path);

    for(struct dirent *d=NULL; dir && (d=readdir(dir));)
        if(strcmp(".", fn=d->d_name) && strcmp("..", fn) && regcmp(fn, regex))
            for(Strings *f=head; f; f=f->next)
                if(!order || !f->next || cmp_basename(fn, f->next->str)/order>0)
                    { create_file_node(f, path, fn, is_fullname); n++; break; }
    if(dir)
        closedir(dir);
    return n;
}

void free_strings(Strings *head)
{
    for(Strings *f=head; f; f=head)
        head=f->next, free(f->str), free(f);
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

static bool regcmp(const char *s, const char *regex) // 簡單的全文(即整個s)正則表達式匹配
{
    return match(s, regex, s);
}

static int cmp_basename(const char *s1, const char *s2)
{
    const char *p1=strrchr(s1, '/'), *p2=strrchr(s2, '/');
    p1=(p1 ? p1+1 : s1), p2=(p2 ? p2+1 : s2);
    return strcmp(p1, p2);
}

static void create_file_node(Strings *head, const char *path, char *filename, bool is_fullname)
{
    Strings *file=malloc_s(sizeof(Strings));
    file->str = is_fullname ? copy_strings(path, "/", filename, NULL) : copy_string(filename);
    file->next=head->next;
    head->next=file;
}

int base_n_floor(int x, int n)
{
    return x/n*n;
}

int base_n_ceil(int x, int n)
{
    return base_n_floor(x, n)+(x%n ? n : 0);
}

void exec_cmd(char *const *cmd)
{
    pid_t pid=fork();
	if(pid == 0)
    {
		if(xinfo.display)
            close(ConnectionNumber(xinfo.display));
		if(!setsid())
            perror(_("未能成功地爲命令創建新會話"));
		if(execvp(cmd[0], cmd) == -1)
            exit_with_perror(_("命令執行錯誤"));
    }
    else if(pid == -1)
        perror(_("未能成功地爲命令創建新進程"));
}

char *get_title_text(Window win, const char *fallback)
{
    char *s=NULL;

    if((s=get_net_wm_name(win)) && strlen(s))
        return s;
    if((s=get_wm_name(win)) && strlen(s))
        return s;
    return copy_string(fallback);
}

char *get_icon_title_text(Window win, const char *fallback)
{
    char *s=NULL;

    if((s=get_net_wm_icon_name(win)) && strlen(s))
        return s;
    if((s=get_wm_icon_name(win)) && strlen(s))
        return s;
    return copy_string(fallback);
}
