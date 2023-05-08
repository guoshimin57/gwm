/* *************************************************************************
 *     drawable.h：與drawable.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef DRAWABLE_H
#define DRAWABLE_H

Window get_transient_for(WM *wm, Window w);
Atom get_atom_prop(WM *wm, Window win, Atom prop);
unsigned char *get_prop(WM *wm, Window win, Atom prop, unsigned long *n);
char *get_text_prop(WM *wm, Window win, Atom atom);
void copy_prop(WM *wm, Window dest, Window src);
bool send_event(WM *wm, Atom protocol, Window win);
bool is_pointer_on_win(WM *wm, Window win);
bool is_on_screen(WM *wm, int x, int y, unsigned int w, unsigned int h);
void print_area(WM *wm, Drawable d, int x, int y, unsigned int w, unsigned int h);
bool is_wm_win(WM *wm, Window win, bool before_wm);
void update_win_background(WM *wm, Window win, unsigned long color, Pixmap pixmap);
void set_override_redirect(WM *wm, Window win);
bool get_geometry(WM *wm, Drawable drw, int *x, int *y, unsigned int *w, unsigned int *h, unsigned int *bw, unsigned int *depth);
void set_pos_for_click(WM *wm, Window click, int cx, int *px, int *py, unsigned int pw, unsigned int ph);
bool is_win_exist(WM *wm, Window win, Window parent);
Pixmap create_pixmap_from_file(WM *wm, Window win, const char *filename);
void show_tooltip(WM *wm, Window hover);

#endif
