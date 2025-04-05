/* *************************************************************************
 *     mvresize.h：與mvresize.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
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
#include "client.h"

void key_move_up(XEvent *e, Arg arg);
void key_move_down(XEvent *e, Arg arg);
void key_move_left(XEvent *e, Arg arg);
void key_move_right(XEvent *e, Arg arg);
void key_resize_up2up(XEvent *e, Arg arg);
void key_resize_up2down(XEvent *e, Arg arg);
void key_resize_down2up(XEvent *e, Arg arg);
void key_resize_down2down(XEvent *e, Arg arg);
void key_resize_left2left(XEvent *e, Arg arg);
void key_resize_left2right(XEvent *e, Arg arg);
void key_resize_right2right(XEvent *e, Arg arg);
void key_resize_right2left(XEvent *e, Arg arg);
void pointer_move(XEvent *e, Arg arg);
void pointer_resize(XEvent *e, Arg arg);
Pointer_act get_resize_act(Client *c, const Move_info *m);
void toggle_shade_client(XEvent *e, Arg arg);
void toggle_shade_client_mode(Client *c, bool shade);


#endif
