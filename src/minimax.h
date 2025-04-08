/* *************************************************************************
 *     minimax.h：與minimax.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MINIMAX_H
#define MINIMAX_H

#include "widget.h"
#include "client.h"

void minimize(XEvent *e, Arg arg);
void deiconify(XEvent *e, Arg arg);
void max_restore(XEvent *e, Arg arg);
void vert_maximize(XEvent *e, Arg arg);
void horz_maximize(XEvent *e, Arg arg);
void top_maximize(XEvent *e, Arg arg);
void bottom_maximize(XEvent *e, Arg arg);
void left_maximize(XEvent *e, Arg arg);
void right_maximize(XEvent *e, Arg arg);
void full_maximize(XEvent *e, Arg arg);
void change_net_wm_state_for_vmax(Client *c, long act);
void change_net_wm_state_for_hmax(Client *c, long act);
void change_net_wm_state_for_tmax(Client *c, long act);
void change_net_wm_state_for_bmax(Client *c, long act);
void change_net_wm_state_for_lmax(Client *c, long act);
void change_net_wm_state_for_rmax(Client *c, long act);
void change_net_wm_state_for_hidden(Client *c, long act);
void change_net_wm_state_for_fullscreen(Client *c, long act);
void show_desktop(XEvent *e, Arg arg);
void toggle_showing_desktop_mode(bool show);

#endif
