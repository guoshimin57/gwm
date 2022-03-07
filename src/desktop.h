/* *************************************************************************
 *     desktop.h：與desktop.c相應的頭文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef DESKTOP_H
#define DESKTOP_H

void next_desktop(WM *wm, XEvent *e, Func_arg arg);
void prev_desktop(WM *wm, XEvent *e, Func_arg arg);
unsigned int get_desktop_n(WM *wm, XEvent *e, Func_arg arg);
void focus_desktop(WM *wm, XEvent *e, Func_arg arg);
void focus_desktop_n(WM *wm, unsigned int n);
bool is_on_cur_desktop(WM *wm, Client *c);
bool is_on_desktop_n(unsigned int desktop_n, Client *c);
unsigned int get_desktop_mask(unsigned int desktop_n);
void move_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void all_move_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void change_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void all_change_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void update_taskbar_buttons(WM *wm);
void attach_to_desktop(WM *wm, XEvent *e, Func_arg arg);
void attach_to_all_desktops(WM *wm, XEvent *e, Func_arg arg);
void all_attach_to_desktop(WM *wm, XEvent *e, Func_arg arg);

#endif
