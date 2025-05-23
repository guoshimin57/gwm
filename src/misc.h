/* *************************************************************************
 *     misc.h：與misc.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MISC_H
#define MISC_H

#include <stdlib.h>
#include <libintl.h>
#include <X11/Xlib.h>
#include "gwm.h"
#include "list.h"

#define _(s) gettext(s)
#define UNUSED(x) ((void)(x))
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define MAX(a, b) ((a)>(b) ? (a) : (b))
#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))

// 數學公式法整數相除並向上取整
#define CEIL_DIV(a, b) (((a)+(b)-1)/(b))

/* 向量化執行指定函數。注意：此宏不能用作表达式 */
#define vfunc(type, func, ...)                          \
    do                                                  \
    {                                                   \
        void *end=(int []){0};                          \
        type **list=(type *[]){__VA_ARGS__, end};       \
        for(size_t i=0; list[i]!=end; i++)              \
            func(list[i]);                              \
    } while(0)

/* 釋放指針類型的參數所指向的內存。注意：此宏不能用作表达式 */
#define vfree(...) vfunc(void, free, __VA_ARGS__)

/* 釋放指針類型的參數所指向的X資源。注意：此宏不能用作表达式 */
#define vXFree(...) vfunc(void, XFree, __VA_ARGS__)

#define Free(p) (free(p), (p)=NULL)

typedef struct // 字符串鏈表
{
    char *str;
    List list;
} Strings;

typedef void (*Event_handler)(XEvent *); // 事件處理器類型

extern Xinfo xinfo;

void *Malloc(size_t size);
int x_fatal_handler(Display *display, XErrorEvent *e);
void exit_with_perror(const char *s);
void exit_with_msg(const char *msg);
char *copy_string(const char *s);
char *copy_strings(const char *s, ...);
void vfree_strings(Strings *head);
int base_n_floor(int x, int n);
int base_n_ceil(int x, int n);
bool is_match_button_release(XButtonEvent *oe, XButtonEvent *ne);
unsigned int get_desktop_mask(unsigned int desktop_n);
bool should_quit(void);
void request_quit(void);
void init_event_handler(Event_handler handler);
void handle_event(XEvent *ev);
bool is_equal_modifier_mask(unsigned int m1, unsigned int m2);

#endif
