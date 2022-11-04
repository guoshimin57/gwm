/* *************************************************************************
 *     func.h：與func.c相應的頭文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef FUNC_H
#define FUNC_H

void choose_client(WM *wm, XEvent *e, Func_arg arg);
void exec(WM *wm, XEvent *e, Func_arg arg);
void key_move_resize_client(WM *wm, XEvent *e, Func_arg arg);
void quit_wm(WM *wm, XEvent *e, Func_arg arg);
void close_client(WM *wm, XEvent *e, Func_arg arg);
void close_all_clients(WM *wm, XEvent *e, Func_arg arg);
void next_client(WM *wm, XEvent *e, Func_arg arg);
void prev_client(WM *wm, XEvent *e, Func_arg arg);
void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg);
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg);
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg);
void change_area(WM *wm, XEvent *e, Func_arg arg);
void pointer_swap_clients(WM *wm, XEvent *e, Func_arg arg);
void maximize_client(WM *wm, XEvent *e, Func_arg arg);
void pointer_move_resize_client(WM *wm, XEvent *e, Func_arg arg);
void pointer_change_area(WM *wm, XEvent *e, Func_arg arg);
void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg);
void iconify_all_clients(WM *wm, XEvent *e, Func_arg arg);
void deiconify_all_clients(WM *wm, XEvent *e, Func_arg arg);
void change_default_area_type(WM *wm, XEvent *e, Func_arg arg);
void toggle_focus_mode(WM *wm, XEvent *e, Func_arg arg);
void open_cmd_center(WM *wm, XEvent *e, Func_arg arg);
void toggle_border_visibility(WM *wm, XEvent *e, Func_arg arg);
void toggle_title_bar_visibility(WM *wm, XEvent *e, Func_arg arg);
void focus_desktop(WM *wm, XEvent *e, Func_arg arg);
void next_desktop(WM *wm, XEvent *e, Func_arg arg);
void prev_desktop(WM *wm, XEvent *e, Func_arg arg);
void move_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void all_move_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void change_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void all_change_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void attach_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void attach_to_all_desktops(WM *wm, XEvent *e, Func_arg arg);
void all_attach_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void enter_and_run_cmd(WM *wm, XEvent *e, Func_arg arg);
void change_wallpaper(WM *wm, XEvent *e, Func_arg arg);

#endif
