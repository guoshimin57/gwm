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

char *get_title_text(Window win, const char *fallback);
char *get_icon_title_text(Window win, const char *fallback);
bool is_pointer_on_win(Window win);
bool is_on_screen(int x, int y, int w, int h);
void print_area(Drawable d, int x, int y, int w, int h);
bool is_wm_win(WM *wm, Window win, bool before_wm);
void update_win_bg(Window win, unsigned long color, Pixmap pixmap);
void set_override_redirect(Window win);
bool get_geometry(Drawable drw, int *x, int *y, int *w, int *h, int *bw, unsigned int *depth);
void set_pos_for_click(Window click, int cx, int *px, int *py, int pw, int ph);
bool is_win_exist(Window win, Window parent);
Pixmap create_pixmap_from_file(Window win, const char *filename);
Window create_widget_win(Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel);
void set_visual_for_imlib(Drawable d);
void restack_win(WM *wm, Window win);
void draw_icon(Drawable d, Imlib_Image image, const char *name, int size);

#endif
