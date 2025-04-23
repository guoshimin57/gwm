/* *************************************************************************
 *     misc.c：雜項。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <X11/Xproto.h>
#include "misc.h"

Xinfo xinfo; // 該全局變量一經顯式初始化，就不再修改

static volatile sig_atomic_t quit_flag=false; // 退出gwm的標志
static Event_handler event_handler=NULL; // 事件處理器

void *Malloc(size_t size)
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

char *copy_string(const char *s)
{
    return s ? strcpy(Malloc(strlen(s)+1), s) : NULL;
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

void vfree_strings(Strings *head)
{
    list_for_each_entry_safe(Strings, s, &head->list, list)
        vfree(s->str, s);
}

int base_n_floor(int x, int n)
{
    return x/n*n;
}

int base_n_ceil(int x, int n)
{
    return base_n_floor(x, n)+(x%n ? n : 0);
}

bool is_match_button_release(XButtonEvent *oe, XButtonEvent *ne)
{
    return (ne->type==ButtonRelease && ne->button==oe->button);
}

unsigned int get_desktop_mask(unsigned int desktop_n)
{
    return desktop_n==~0U ? desktop_n : 1U<<desktop_n;
}

bool should_quit(void)
{
    return quit_flag==true;
}

void request_quit(void)
{
    quit_flag=true;
}

void init_event_handler(Event_handler handler)
{
    event_handler=handler;
}

void handle_event(XEvent *ev)
{
    event_handler(ev);
}
