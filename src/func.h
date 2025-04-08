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

#include "widget.h"

void choose_client(XEvent *e, Arg arg);
void exec(XEvent *e, Arg arg);
void quit_wm(XEvent *e, Arg arg);
void clear_wm(void);
void close_client(XEvent *e, Arg arg);
void close_all_clients(XEvent *e, Arg arg);
void next_client(XEvent *e, Arg arg);
void prev_client(XEvent *e, Arg arg);
void increase_main_n(XEvent *e, Arg arg);
void decrease_main_n(XEvent *e, Arg arg);
void adjust_layout_ratio(XEvent *e, Arg arg);
void adjust_main_area_ratio(XEvent *e, Arg arg);
void adjust_fixed_area_ratio(XEvent *e, Arg arg);
void show_desktop(XEvent *e, Arg arg);
void toggle_showing_desktop_mode(bool show);
void change_default_place(XEvent *e, Arg arg);
void toggle_focus_mode(XEvent *e, Arg arg);
void open_act_center(XEvent *e, Arg arg);
void open_client_menu(XEvent *e, Arg arg);
void focus_desktop(XEvent *e, Arg arg);
void next_desktop(XEvent *e, Arg arg);
void prev_desktop(XEvent *e, Arg arg);
void move_to_desktop(XEvent *e, Arg arg);
void all_move_to_desktop(XEvent *e, Arg arg);
void change_to_desktop(XEvent *e, Arg arg);
void all_change_to_desktop(XEvent *e, Arg arg);
void attach_to_desktop(XEvent *e, Arg arg);
void attach_to_all_desktops(XEvent *e, Arg arg);
void all_attach_to_desktop(XEvent *e, Arg arg);
void run_cmd(XEvent *e, Arg arg);
void set_color(XEvent *e, Arg arg);
void switch_wallpaper(XEvent *e, Arg arg);
void print_screen(XEvent *e, Arg arg);
void print_win(XEvent *e, Arg arg);
void toggle_compositor(XEvent *e, Arg arg);

#endif
