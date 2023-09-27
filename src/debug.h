/* *************************************************************************
 *     debug.h：與debug.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef DEBUG_H
#define DEBUG_H

void print_client_and_top_win(WM *wm);
void print_win_tree(WM *wm, Window win);
void print_net_wm_win_type(WM *wm, Window win);
void print_net_wm_state(WM *wm, Window win);
int print_atom_name(WM *wm, Atom atom);
void print_all_atom_name(WM *wm);
void print_place_info(Client *c);
void print_client_win(WM *wm);
void show_top_win(WM *wm);

#endif
