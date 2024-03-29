/* *************************************************************************
 *     client.h：與client.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

#include "drawable.h"
#include "ewmh.h"

struct icon_tag // 縮微窗口相關信息
{
    Window win; // 位於任務欄的縮微窗口（可能含有圖標名）
    int x, y, w, h; // 無邊框時win的坐標、尺寸
    bool show_text; // 當存在圖標映像時，是否顯示圖標名
    char *title_text; // 圖標名，即XA_WM_ICON_NAME，理論上應比XA_WM_NAME簡短，實際上很多客戶窗口的都是與它一模一樣。
};
typedef struct icon_tag Icon;

struct client_tag // 客戶窗口相關信息
{   // 分別爲客戶窗口、父窗口、圖標窗口, 標題區、標題區按鈕、臨時窗口對應的主窗口
    Window win, frame, logo, title_area, buttons[TITLE_BUTTON_N];
    int x, y, w, h, ox, oy, ow, oh; // 分别爲win現在的和原來的橫、縱坐標和寬、高
    int titlebar_h, border_w; // win的標題欄高、邊框寬
    unsigned int desktop_mask; // 所屬虚拟桌面的掩碼
    long map_n; // win最後一次映射的序號
    Place_type place_type, old_place_type; // 窗口現在的位置類型及原來的位置類型
    Net_wm_win_type win_type; // win的窗口類型
    Net_wm_state win_state; // win的窗口狀態
    char *title_text; // 標題的文字
    Icon *icon; // 圖符信息
    Imlib_Image image; // 圖標映像
    const char *class_name; // 客戶窗口的程序類型名
    XClassHint class_hint; // 客戶窗口的程序類型特性提示
    XWMHints *wm_hint; // 客戶窗口的窗口管理程序條件特性提示
    // 分別爲前、後節點以及主窗口節點、亚組組長節點（同屬一個程序實例的客戶構成一個亞組）
    struct client_tag *prev, *next, *owner, *subgroup_leader;
};

Rect get_frame_rect(Client *c);
Rect get_button_rect(Client *c, size_t index);
void add_client(WM *wm, Window win);
void set_all_net_client_list(Client *list);
void set_win_rect_by_frame(Client *c, const Rect *frame);
void create_titlebar(Client *c);
Rect get_title_area_rect(Client *c);
int get_clients_n(Client *list, Place_type type, bool count_icon, bool count_trans, bool count_all_desktop);
Client *win_to_client(Client *list, Window win);
void del_client(WM *wm, Client *c, bool is_for_quit);
Client *win_to_iconic_state_client(Client *list, Window win);
void raise_client(WM *wm, Client *c);
Client *get_next_client(Client *list, Client *c);
Client *get_prev_client(Client *list, Client *c);
bool is_normal_layer(Place_type t);
bool is_last_typed_client(Client *list, Client *c, Place_type type);
Client *get_head_client(Client *list, Place_type type);
int get_subgroup_n(Client *c);
Client *get_subgroup_leader(Client *c);
Client *get_top_transient_client(Client *subgroup_leader, bool only_modal);
void focus_client(WM *wm, unsigned int desktop_n, Client *c);
bool is_on_desktop_n(unsigned int n, Client *c);
bool is_on_cur_desktop(Client *c);
unsigned int get_desktop_mask(unsigned int desktop_n);
void update_icon_area(Client *list);
void save_place_info_of_client(Client *c);
void save_place_info_of_clients(Client *list);
void restore_place_info_of_client(Client *c);
void restore_place_info_of_clients(Client *list);
bool is_tile_client(Client *c);
Window *get_client_win_list(Client *list, int *n);
Window *get_client_win_list_stacking(Client *list, int *n);
void set_attention(Client *c, bool attent);
void set_urgency(Client *c, bool urg);
bool is_wm_win(Client *list, Window win, bool before_wm);
void restack_win(WM *wm, Window win);
void update_clients_bg(WM *wm);
void update_client_bg(WM *wm, unsigned int desktop_n, Client *c);
void update_frame_bg(WM *wm, unsigned int desktop_n, Client *c);
void create_clients(WM *wm);
void add_subgroup(Client *head, Client *subgroup_leader);
void del_subgroup(Client *subgroup_leader);

#endif
