/* *************************************************************************
 *     mvresize.h：與mvresize.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MVRESIZE_H 
#define MVRESIZE_H 

#include "gwm.h"

typedef struct /* 調整窗口尺寸的信息 */
{
    int dx, dy, dw, dh; /* 分別爲窗口坐標和尺寸的變化量 */
} Delta_rect;

void move_resize(WM *wm, XEvent *e, Func_arg arg);
void move_resize_client(Client *c, const Delta_rect *d);
Place_type get_dest_place_type_for_move(WM *wm, Client *c);
void update_win_state_for_move_resize(WM *wm, Client *c);
Pointer_act get_resize_act(Client *c, const Move_info *m);
void toggle_shade_client(WM *wm, XEvent *e, Func_arg arg);
void toggle_shade_client_mode(Client *c, bool shade);
void set_client_rect_by_outline(Client *c, int x, int y, int w, int h);
void set_client_rect_by_win(Client *c, int x, int y, int w, int h);

#endif
