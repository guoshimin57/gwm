/* *************************************************************************
 *     func.h：與func.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef FUNC_H
#define FUNC_H

#include "gwm.h"

bool is_drag_func(void (*func)(WM *, XEvent *, Arg));
bool get_valid_click(WM *wm, Pointer_act act, XEvent *oe, XEvent *ne);
void choose_client(WM *wm, XEvent *e, Arg arg);
void exec(WM *wm, XEvent *e, Arg arg);
void quit_wm(WM *wm, XEvent *e, Arg arg);
void clear_wm(WM *wm);
void close_client(WM *wm, XEvent *e, Arg arg);
void close_all_clients(WM *wm, XEvent *e, Arg arg);
void next_client(WM *wm, XEvent *e, Arg arg);
void prev_client(WM *wm, XEvent *e, Arg arg);
void increase_main_n(WM *wm, XEvent *e, Arg arg);
void decrease_main_n(WM *wm, XEvent *e, Arg arg);
void adjust_layout_ratio(WM *wm, XEvent *e, Arg arg);
void adjust_main_area_ratio(WM *wm, XEvent *e, Arg arg);
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Arg arg);
void show_desktop(WM *wm, XEvent *e, Arg arg);
void toggle_showing_desktop_mode(bool show);
void change_default_place_type(WM *wm, XEvent *e, Arg arg);
void toggle_focus_mode(WM *wm, XEvent *e, Arg arg);
void open_act_center(WM *wm, XEvent *e, Arg arg);
void open_client_menu(WM *wm, XEvent *e, Arg arg);
void focus_desktop(WM *wm, XEvent *e, Arg arg);
void next_desktop(WM *wm, XEvent *e, Arg arg);
void prev_desktop(WM *wm, XEvent *e, Arg arg);
void move_to_desktop(WM *wm, XEvent *e, Arg arg);
void all_move_to_desktop(WM *wm, XEvent *e, Arg arg);
void change_to_desktop(WM *wm, XEvent *e, Arg arg);
void all_change_to_desktop(WM *wm, XEvent *e, Arg arg);
void attach_to_desktop(WM *wm, XEvent *e, Arg arg);
void attach_to_all_desktops(WM *wm, XEvent *e, Arg arg);
void all_attach_to_desktop(WM *wm, XEvent *e, Arg arg);
void run_cmd(WM *wm, XEvent *e, Arg arg);
void set_color(WM *wm, XEvent *e, Arg arg);
void switch_wallpaper(WM *wm, XEvent *e, Arg arg);
void print_screen(WM *wm, XEvent *e, Arg arg);
void print_win(WM *wm, XEvent *e, Arg arg);
void toggle_compositor(WM *wm, XEvent *e, Arg arg);

#endif
