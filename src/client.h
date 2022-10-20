/* *************************************************************************
 *     client.h：與client.c相應的頭文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

void add_client(WM *wm, Window win);
void add_client_node(Client *head, Client *c);
void fix_area_type(WM *wm);
void set_default_rect(WM *wm, Client *c);
void update_frame_prop(WM *wm, Client *c);
void create_title_bar(WM *wm, Client *c);
Rect get_title_area_rect(WM *wm, Client *c);
unsigned int get_typed_clients_n(WM *wm, Area_type type);
Client *win_to_client(WM *wm, Window win);
void del_client(WM *wm, Client *c);
void del_client_node(Client *c);
void free_client(WM *wm, Client *c);
void move_resize_client(WM *wm, Client *c, const Delta_rect *d);
void update_frame(WM *wm, unsigned int desktop_n, Client *c);
Client *win_to_iconic_state_client(WM *wm, Window win);
void focus_client(WM *wm, unsigned int desktop_n, Client *c);
void update_client_look(WM *wm, unsigned int desktop_n, Client *c);
void raise_client(WM *wm, unsigned int desktop_n);
void move_client(WM *wm, Client *from, Client *to, Area_type type);
void swap_clients(WM *wm, Client *a, Client *b);
int compare_client_order(WM *wm, Client *c1, Client *c2);
bool send_event(WM *wm, Atom protocol, Window win);
Client *get_next_client(WM *wm, Client *c);
Client *get_prev_client(WM *wm, Client *c);
Client *get_area_head(WM *wm, Area_type type);

#endif
