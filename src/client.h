/* *************************************************************************
 *     client.h：與client.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CLIENT_H
#define CLIENT_H

struct client_tag // 客戶窗口相關信息
{   /* 分別爲客戶窗口、父窗口、標題區、標題區按鈕、臨時窗口對應的主窗口 */
    Window win, frame, title_area, buttons[TITLE_BUTTON_N], owner;
    int x, y; // win的橫、縱坐標
    /* win的寬、高、標題欄高、邊框寬、所属虚拟桌面的掩碼 */
    unsigned int w, h, title_bar_h, border_w, desktop_mask;
    Area_type area_type; // 區域類型
    char *title_text; // 標題的文字
    Icon *icon; // 圖符信息
    Imlib_Image image; // 圖符的圖像
    const char *class_name; // 客戶窗口的程序類型名
    XClassHint class_hint; // 客戶窗口的程序類型特性提示
    XSizeHints size_hint; // 客戶窗口的窗口尺寸條件特性提示
    XWMHints *wm_hint; // 客戶窗口的窗口管理程序條件特性提示
    struct client_tag *prev, *next; // 分別爲前、後節點
};

void add_client(WM *wm, Window win);
void add_client_node(Client *head, Client *c);
void fix_area_type(WM *wm);
void set_default_win_rect(WM *wm, Client *c);
void create_title_bar(WM *wm, Client *c);
Rect get_title_area_rect(WM *wm, Client *c);
unsigned int get_typed_clients_n(WM *wm, Area_type type);
unsigned int get_clients_n(WM *wm);
unsigned int get_all_clients_n(WM *wm);
Client *win_to_client(WM *wm, Window win);
void del_client(WM *wm, Client *c, bool is_for_quit);
void del_client_node(Client *c);
void move_resize_client(WM *wm, Client *c, const Delta_rect *d);
void update_frame(WM *wm, unsigned int desktop_n, Client *c);
Client *win_to_iconic_state_client(WM *wm, Window win);
void raise_client(WM *wm, unsigned int desktop_n);
Client *get_next_client(WM *wm, Client *c);
Client *get_prev_client(WM *wm, Client *c);
void move_client(WM *wm, Client *from, Client *to, Area_type type);
void swap_clients(WM *wm, Client *a, Client *b);
int compare_client_order(WM *wm, Client *c1, Client *c2);
bool is_last_typed_client(WM *wm, Client *c, Area_type type);
Client *get_area_head(WM *wm, Area_type type);

#endif
