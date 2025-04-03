/* *************************************************************************
 *     clientop.h：與clientop.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CLIENTOP_H
#define CLIENTOP_H

#include <stdbool.h>
#include "gwm.h"
#include "client.h"

void manage_exsit_clients(void);
void add_client(Window win);
void remove_client(Client *c, bool is_for_quit);
void move_resize_client(Client *c, const Delta_rect *d);
void move_client(Client *from, Client *to, Place type);
void swap_clients(Client *a, Client *b);
void restore_client(Client *c);
void iconify_client(Client *c);
void deiconify_client(Client *c);
void iconify_all_clients(void);
void deiconify_all_clients(void);

#endif
