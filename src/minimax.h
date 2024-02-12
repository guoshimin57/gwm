/* *************************************************************************
 *     minimax.h：與minimax.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MINIMAX_H
#define MINIMAX_H

void maximize_client(WM *wm, Client *c, Max_way way);
void max_restore_client(WM *wm, Client *c);
void fix_win_rect_by_state(WM *wm, Client *c);
void restore_client(WM *wm, Client *c);
void set_max_rect(WM *wm, Client *c, Max_way max_way);
void get_max_rect(WM *wm, Client *c, int *left_x, int *top_y, int *max_w, int *max_h, int *mid_x, int *mid_y, int *half_w, int *half_h);
bool is_win_state_max(Client *c);
void fix_win_rect_by_state(WM *wm, Client *c);
void iconify(WM *wm, Client *c);
void create_icon(Client *c);
void deiconify(WM *wm, Client *c);
void del_icon(WM *wm, Client *c);
void iconify_all_clients(WM *wm);
void deiconify_all_clients(WM *wm);
void change_net_wm_state_for_vmax(WM *wm, Client *c, long act);
void change_net_wm_state_for_hmax(WM *wm, Client *c, long act);
void change_net_wm_state_for_tmax(WM *wm, Client *c, long act);
void change_net_wm_state_for_bmax(WM *wm, Client *c, long act);
void change_net_wm_state_for_lmax(WM *wm, Client *c, long act);
void change_net_wm_state_for_rmax(WM *wm, Client *c, long act);
void change_net_wm_state_for_hidden(WM *wm, Client *c, long act);
void change_net_wm_state_for_fullscreen(WM *wm, Client *c, long act);

#endif
