/* *************************************************************************
 *     client.h：與client.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

#include "gwm.h"
#include "drawable.h"
#include "ewmh.h"
#include "frame.h"
#include "list.h"

typedef struct client_tag // 客戶窗口相關信息
{
    Widget base;
    Frame *frame; // 客戶窗口裝飾
    bool decorative; // 是否裝飾，即顯示窗口標題欄和邊框
    int ox, oy, ow, oh; // 分别爲win原來的橫、縱坐標和寬、高
    unsigned int desktop_mask; // 所屬虚拟桌面的掩碼
    Place place, old_place; // 窗口現在的位置及原來的位置
    Net_wm_win_type win_type; // win的窗口類型
    Net_wm_state win_state; // win的窗口狀態
    char *title_text; // 標題的文字
    Imlib_Image image; // 圖標映像
    const char *class_name; // 客戶窗口的程序類型名
    XClassHint class_hint; // 客戶窗口的程序類型特性提示
    XWMHints *wm_hint; // 客戶窗口的窗口管理程序條件特性提示
    // 分別爲主窗口節點、亚組組長節點（同屬一個程序實例的客戶構成一個亞組）
    struct client_tag *owner, *subgroup_leader;
    List list;
} Client;

#define subgroup_for_each(c, leader) \
    for(Client *c=leader;\
        c->subgroup_leader==leader;\
        c=list_prev_entry(c, Client, list))

#define clients_is_empty() \
    list_is_empty(&get_clients()->list)

#define clients_last() \
    list_last_entry(&get_clients()->list, Client, list)

#define clients_next(c) \
    list_next_entry(c, Client, list)

#define clients_prev(c) \
    list_prev_entry(c, Client, list)

#define clients_for_each(c) \
    list_for_each_entry(Client, c, &get_clients()->list, list)

#define clients_for_each_safe(c) \
    list_for_each_entry_safe(Client, c, &get_clients()->list, list)

#define clients_for_each_from(c) \
    list_for_each_entry_from(Client, c, &get_clients()->list, list)

#define clients_for_each_reverse(c) \
    list_for_each_entry_reverse(Client, c, &get_clients()->list, list)

void init_client_list(void);
Client *get_clients(void);
Client *client_new(Window win);
void client_del(Client *c);
int get_clients_n(Place type, bool count_icon, bool count_trans, bool count_all_desktop);
bool is_iconic_client(const Client *c);
Client *win_to_client(Window win);
Client *get_next(Client *c);
Client *get_prev(Client *c);
bool is_normal_layer(Place t);
bool is_spec_place_last_client(Client *c, Place type);
int get_subgroup_n(Client *c);
Client *get_subgroup_leader(Client *c);
Client *get_top_transient_client(Client *subgroup_leader, bool only_modal);
void client_set_state_unfocused(Client *c, int value);
void save_place_info_of_client(Client *c);
void restore_place_info_of_client(Client *c);
bool is_tile_client(Client *c);
bool is_tiled_client(Client *c);
void set_state_attent(Client *c, bool attent);
bool is_wm_win(Window win, bool before_wm);
void update_clients_bg(void);
void update_client_bg(Client *c);
void set_client_rect_by_outline(Client *c, int x, int y, int w, int h);
bool is_exist_client(Client *c);
bool is_new_client(Client *c);
Client *get_head_client(const Client *c, Place place);

#endif
