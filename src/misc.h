/* *************************************************************************
 *     misc.h：與misc.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MISC_H
#define MISC_H

enum order_tag // 文件名排序类型
{
    RISE=-1, NOSORT=0, FALL=1 // 依次为升序、不排序、降序
};
typedef enum order_tag Order;

void *malloc_s(size_t size);
int x_fatal_handler(Display *display, XErrorEvent *e);
void exit_with_perror(const char *s);
void exit_with_msg(const char *msg);
Widget_type get_widget_type(WM *wm, Window win);
Pointer_act get_resize_act(Client *c, const Move_info *m);
void clear_zombies(int signum);
bool is_chosen_button(WM *wm, Widget_type type);
void set_xic(WM *wm, Window win, XIC *ic);
KeySym look_up_key(XIC xic, XKeyEvent *e, wchar_t *keyname, size_t n);
char *copy_string(const char *s);
char *copy_strings(const char *s, ...);
void vfree(void *ptr, ...);
File *get_files_in_paths(const char *paths, const char *regex, Order order, bool is_fullname, size_t *n);
void free_files(File *head);
int base_n_floor(int x, int n);
int base_n_ceil(int x, int n);
void exec_cmd(WM *wm, char *const*cmd);
void update_hint_win_for_info(WM *wm, Window hover, const char *info);

#endif
