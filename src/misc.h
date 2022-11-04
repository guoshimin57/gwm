/* *************************************************************************
 *     misc.h：與misc.c相應的頭文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MISC_H
#define MISC_H

void *malloc_s(size_t size);
int x_fatal_handler(Display *display, XErrorEvent *e);
void exit_with_perror(const char *s);
void exit_with_msg(const char *msg);
bool is_wm_win(WM *wm, Window win);
void update_win_background(WM *wm, Window win, unsigned long color, Pixmap pixmap);
Pixmap create_pixmap_from_file(WM *wm, Window win, const char *filename);
Widget_type get_widget_type(WM *wm, Window win);
Pointer_act get_resize_act(Client *c, const Move_info *m);
void clear_zombies(int unused);
bool is_chosen_button(WM *wm, Widget_type type);
void set_xic(WM *wm, Window win, XIC *ic);
Window get_transient_for(WM *wm, Window w);
KeySym look_up_key(XIC xic, XKeyEvent *e, wchar_t *keyname, size_t n);
Atom get_atom_prop(WM *wm, Window win, Atom prop);
unsigned char *get_prop(WM *wm, Window win, Atom prop, unsigned long *n);
void set_override_redirect(WM *wm, Window win);
void clear_wm(WM *wm);
bool get_geometry(WM *wm, Drawable drw, unsigned int *w, unsigned int *h, unsigned int *depth);
char *copy_string(const char *s);
char *copy_strings(const char *s, ...);
void set_pos_for_click(WM *wm, Window click, int cx, int cy, int *px, int *py, unsigned int pw, unsigned int ph);
bool is_win_exist(WM *wm, Window win, Window parent);
File *get_files_in_dirs(const char *paths[], size_t n, const char *exts[], size_t m, Order order, bool is_fullname);

#endif
