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

void choose(XEvent *e, Arg arg);
void exec(XEvent *e, Arg arg);
void quit_wm(XEvent *e, Arg arg);
void quit(XEvent *e, Arg arg);
void quit_all(XEvent *e, Arg arg);
void next(XEvent *e, Arg arg);
void prev(XEvent *e, Arg arg);
void rise_main_n(XEvent *e, Arg arg);
void fall_main_n(XEvent *e, Arg arg);
void adjust_layout_ratio(XEvent *e, Arg arg);
void adjust_main_area_ratio(XEvent *e, Arg arg);
void adjust_fixed_area_ratio(XEvent *e, Arg arg);
void show_desktop(XEvent *e, Arg arg);
void change_default_place(XEvent *e, Arg arg);
void toggle_focus_mode(XEvent *e, Arg arg);
void start(XEvent *e, Arg arg);
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
void mini(XEvent *e, Arg arg);
void deiconify(XEvent *e, Arg arg);
void toggle_max_restore(XEvent *e, Arg arg);
void vmax(XEvent *e, Arg arg);
void hmax(XEvent *e, Arg arg);
void tmax(XEvent *e, Arg arg);
void bmax(XEvent *e, Arg arg);
void lmax(XEvent *e, Arg arg);
void rmax(XEvent *e, Arg arg);
void max(XEvent *e, Arg arg);
void show_desktop(XEvent *e, Arg arg);
void move_up(XEvent *e, Arg arg);
void move_down(XEvent *e, Arg arg);
void move_left(XEvent *e, Arg arg);
void move_right(XEvent *e, Arg arg);
void fall_width(XEvent *e, Arg arg);
void rise_width(XEvent *e, Arg arg);
void fall_height(XEvent *e, Arg arg);
void rise_height(XEvent *e, Arg arg);
void move(XEvent *e, Arg arg);
void resize(XEvent *e, Arg arg);
void toggle_shade(XEvent *e, Arg arg);
void change_place(XEvent *e, Arg arg);
void to_main_area(XEvent *e, Arg arg);
void to_second_area(XEvent *e, Arg arg);
void to_fixed_area(XEvent *e, Arg arg);
void to_stack_layer(XEvent *e, Arg arg);
void fullscreen(XEvent *e, Arg arg);
void to_above_layer(XEvent *e, Arg arg);
void to_below_layer(XEvent *e, Arg arg);
void swap(XEvent *e, Arg arg);
void stack(XEvent *e, Arg arg);
void tile(XEvent *e, Arg arg);
void adjust_layout_ratio(XEvent *e, Arg arg);
void rise_main_area(XEvent *e, Arg arg);
void fall_main_area(XEvent *e, Arg arg);
void rise_fixed_area(XEvent *e, Arg arg);
void fall_fixed_area(XEvent *e, Arg arg);

#endif
