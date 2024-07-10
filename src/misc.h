/* *************************************************************************
 *     misc.h：與misc.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MISC_H
#define MISC_H

/* 釋放指針類型的參數所指向的內存。注意：不能用作表达式 */
#define vfree(...)                                      \
    do                                                  \
    {                                                   \
        void *end=(int []){0};                          \
        void **list=(void *[]){__VA_ARGS__, end};       \
        for(size_t i=0; list[i]!=end; i++)              \
            if(list[i])                                 \
                free(list[i]);                          \
    } while(0)

typedef struct strings_tag // 字符串鏈表
{
    char *str;
    struct strings_tag *next;
} Strings;

void *malloc_s(size_t size);
int x_fatal_handler(Display *display, XErrorEvent *e);
void exit_with_perror(const char *s);
void exit_with_msg(const char *msg);
void clear_zombies(int signum);
char *copy_string(const char *s);
char *copy_strings(const char *s, ...);
void vfreetrings(Strings *head);
int base_n_floor(int x, int n);
int base_n_ceil(int x, int n);
char *get_title_text(Window win, const char *fallback);
char *get_icon_title_text(Window win, const char *fallback);
bool is_match_button_release(XEvent *oe, XEvent *ne);
bool is_on_desktop_n(unsigned int n, unsigned int mask);
bool is_on_cur_desktop(unsigned int mask);
unsigned int get_desktop_mask(unsigned int desktop_n);

#endif
