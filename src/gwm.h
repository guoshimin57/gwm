/* *************************************************************************
 *     gwm.h：與gwm.c相應的頭文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef GWM_H
#define GWM_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <locale.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>

#define ICCCM_NAMES (const char *[]) \
{\
    "WM_PROTOCOLS", "WM_DELETE_WINDOW", \
}
#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))
#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define FUNC_ARG(var, data) (Func_arg){.var=data}

#define TITLE_BUTTON_N ARRAY_NUM(TITLE_BUTTON_TEXT)
#define TASKBAR_BUTTON_N ARRAY_NUM(TASKBAR_BUTTON_TEXT)
#define CMD_CENTER_BUTTON_N ARRAY_NUM(CMD_CENTER_BUTTON_TEXT)

#define BUTTON_MASK (ButtonPressMask|ButtonReleaseMask)
#define POINTER_MASK (BUTTON_MASK|ButtonMotionMask)
#define CROSSING_MASK (EnterWindowMask|LeaveWindowMask)
#define ROOT_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    PropertyChangeMask|POINTER_MASK|PointerMotionMask|ExposureMask|CROSSING_MASK)
#define BUTTON_EVENT_MASK (ButtonPressMask|ExposureMask|CROSSING_MASK)
#define FRAME_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    ExposureMask|ButtonPressMask|CROSSING_MASK)
#define TITLE_AREA_EVENT_MASK (ButtonPressMask|ExposureMask|CROSSING_MASK)

#define TITLE_BUTTON_INDEX(type) ((type)-TITLE_BUTTON_BEGIN)
#define IS_TITLE_BUTTON(type) \
    ((type)>=TITLE_BUTTON_BEGIN && (type)<=TITLE_BUTTON_END)

#define TASKBAR_BUTTON_INDEX(type) ((type)-TASKBAR_BUTTON_BEGIN)
#define IS_TASKBAR_BUTTON(type) \
    ((type)>=TASKBAR_BUTTON_BEGIN && (type)<=TASKBAR_BUTTON_END)

#define CMD_CENTER_BUTTON_INDEX(type) ((type)-CMD_CENTER_BUTTON_BEGIN)
#define IS_CMD_CENTER_BUTTON(type) \
    ((type)>=CMD_CENTER_BUTTON_BEGIN && (type)<=CMD_CENTER_BUTTON_END)

enum focus_mode_tag // 窗口聚焦模式
{
    ENTER_FOCUS, CLICK_FOCUS,
};
typedef enum focus_mode_tag Focus_mode;

enum area_type_tag // 窗口的區域類型
{
     MAIN_AREA, SECOND_AREA, FIXED_AREA, FLOATING_AREA, ICONIFY_AREA,
     AREA_TYPE_N, PREV_AREA, ROOT_AREA,
};
typedef enum area_type_tag Area_type;

enum widget_type_tag // 構件類型
{
    UNDEFINED, ROOT_WIN, STATUS_AREA,
    CLIENT_WIN, CLIENT_FRAME, TITLE_AREA, CLIENT_ICON,
    MAIN_BUTTON, SECOND_BUTTON, FIXED_BUTTON, FLOAT_BUTTON,
    ICON_BUTTON, MAX_BUTTON, CLOSE_BUTTON,
    FULL_BUTTON, PREVIEW_BUTTON, STACK_BUTTON, TILE_BUTTON, DESKTOP_BUTTON,
    CMD_CENTER_BUTTON,
    HELP_BUTTON, FILE_BUTTON, TERM_BUTTON, BROWSER_BUTTON, 
    PLAY_START_BUTTON, PLAY_TOGGLE_BUTTON, PLAY_QUIT_BUTTON,
    VOLUME_DOWN_BUTTON, VOLUME_UP_BUTTON, VOLUME_MAX_BUTTON, VOLUME_TOGGLE_BUTTON,
    MAIN_NEW_BUTTON, SEC_NEW_BUTTON, FIX_NEW_BUTTON, FLOAT_NEW_BUTTON,
    ICON_NEW_BUTTON, N_MAIN_UP_BUTTON, N_MAIN_DOWN_BUTTON, FOCUS_MODE_BUTTON,
    QUIT_WM_BUTTON, LOGOUT_BUTTON, REBOOT_BUTTON, POWEROFF_BUTTON,
    RUN_BUTTON,
    TITLE_BUTTON_BEGIN=MAIN_BUTTON, TITLE_BUTTON_END=CLOSE_BUTTON,
    TASKBAR_BUTTON_BEGIN=FULL_BUTTON, TASKBAR_BUTTON_END=CMD_CENTER_BUTTON,
    CMD_CENTER_BUTTON_BEGIN=HELP_BUTTON, CMD_CENTER_BUTTON_END=RUN_BUTTON,
};
typedef enum widget_type_tag Widget_type;

struct rectangle_tag // 矩形窗口的坐標和尺寸
{
    int x, y; // 坐標
    unsigned int w, h; // 尺寸
};
typedef struct rectangle_tag Rect;

struct icon_tag // 縮微窗口相關信息
{
    Window win; // 縮微窗口
    int x, y; // 無邊框時縮微窗口的坐標
    unsigned int w, h; // 無邊框時縮微窗口的尺寸
    Area_type area_type; // 窗口微縮之前的區域類型
    bool is_short_text; // 是否只爲縮微窗口顯示簡短的文字
};
typedef struct icon_tag Icon;

struct client_tag // 客戶窗口相關信息
{   /* 分別爲客戶窗口、父窗口、標題區、標題區按鈕 */
    Window win, frame, title_area, buttons[TITLE_BUTTON_N];
    int x, y; // win的橫、縱坐標
    unsigned int w, h, border_w, title_bar_h; // win的寬、高、邊框寬、標題欄高
    Area_type area_type; // 區域類型
    char *title_text; // 標題的文字
    Icon *icon; // 圖符信息
    const char *class_name; // 客戶窗口的程序類型名
    XClassHint class_hint; // 客戶窗口的程序類型提示
    struct client_tag *prev, *next; // 分別爲前、後節點
};
typedef struct client_tag Client;

enum layout_tag // 窗口管理器的布局模式
{
    FULL, PREVIEW, STACK, TILE,
};
typedef enum layout_tag Layout;

enum pointer_act_tag // 定位器操作類型
{
    NO_OP, MOVE, TOP_RESIZE, BOTTOM_RESIZE, LEFT_RESIZE, RIGHT_RESIZE,
    TOP_LEFT_RESIZE, TOP_RIGHT_RESIZE, BOTTOM_LEFT_RESIZE, BOTTOM_RIGHT_RESIZE,
    ADJUST_LAYOUT_RATIO, POINTER_ACT_N
};
typedef enum pointer_act_tag Pointer_act;

struct taskbar_tag // 窗口管理器的任務欄
{
    /* 分別爲任務欄的窗口、按鈕、縮微區域、狀態區域 */
    Window win, buttons[TASKBAR_BUTTON_N], icon_area, status_area;
    int x, y; // win的坐標
    unsigned int w, h, status_area_w; // win的尺寸和狀態區域的寬度
    char *status_text; // 狀態區域要顯示的文字
};
typedef struct taskbar_tag Taskbar;

struct cmd_center_tag // 操作中心
{
    Window win, buttons[CMD_CENTER_BUTTON_N]; // 窗口和按鈕
};
typedef struct cmd_center_tag Cmd_center;

enum icccm_atom_tag // icccm規範的標識符
{
    WM_PROTOCOLS, WM_DELETE_WINDOW, ICCCM_ATOMS_N
};
typedef enum icccm_atom_tag Icccm_atom;

struct wm_tag // 窗口管理器相關信息
{
    Display *display; // 顯示器
    int screen; // 屏幕
    unsigned int screen_width, screen_height; // 屏幕寬度、高度
	XModifierKeymap *mod_map; // 功能轉換鍵映射
    Window root_win; // 根窗口
    GC gc; // 窗口管理器的圖形信息
    Atom icccm_atoms[ICCCM_ATOMS_N]; // icccm規範的標識符
    Client *clients, *cur_focus_client, *prev_focus_client; // 分別爲頭結點、當前聚焦結點、前一個聚焦結點
    unsigned int clients_n[AREA_TYPE_N], n_main_max; // 分別爲客戶窗口總數、主區域可容納的客戶窗口數量
    Layout cur_layout, prev_layout; // 分別爲當前布局模式和前一個布局模式
    Area_type default_area_type; // 默認的窗口區域類型
    Focus_mode focus_mode; // 窗口聚焦模式
    XFontSet font_set; // 窗口管理器用到的字體集
    Cursor cursors[POINTER_ACT_N]; // 光標
    Taskbar taskbar; // 任務欄
    Cmd_center cmd_center; // 操作中心
    double main_area_ratio, fixed_area_ratio; // 分別爲主要和固定區域屏佔比
};
typedef struct wm_tag WM;

enum direction_tag // 方向
{
    UP, DOWN, LEFT, RIGHT, 
    LEFT2LEFT, LEFT2RIGHT, RIGHT2LEFT, RIGHT2RIGHT,
    UP2UP, UP2DOWN, DOWN2UP, DOWN2DOWN,
};
typedef enum direction_tag Direction;

enum align_type_tag // 文字對齊方式
{
    TOP_LEFT, TOP_CENTER, TOP_RIGHT,
    CENTER_LEFT, CENTER, CENTER_RIGHT,
    BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT,
};
typedef enum align_type_tag ALIGN_TYPE;

union func_arg_tag // 函數參數類型
{
    char *const *cmd; // 命令字符串
    Direction direction; // 方向
    Layout layout; // 窗口布局模式
    Pointer_act pointer_act; // 窗口操作類型
    int n; // 數量
    Area_type area_type; // 窗口區域類型
    double change_ratio; // 變化率
};
typedef union func_arg_tag Func_arg;

struct buttonbind_tag // 定位器按鈕功能綁定
{
    Widget_type widget_type; // 要綁定的構件類型
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵 
    unsigned int button; // 要綁定的定位器按鈕
	void (*func)(WM *wm, XEvent *e, Func_arg arg); // 要綁定的函數
    Func_arg arg; // 要綁定的函數的參數
};
typedef struct buttonbind_tag Buttonbind;

struct keybind_tag // 鍵盤按鍵功能綁定
{
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵
	KeySym keysym; // 要綁定的鍵盤功能轉換鍵
	void (*func)(WM *wm, XEvent *e, Func_arg arg); // 要綁定的函數
    Func_arg arg; // 要綁定的函數的參數
};
typedef struct keybind_tag Keybind;

struct rule_tag // 窗口管理器的規則
{
    const char *app_class, *app_name; // 分別爲客戶窗口的程序類型和程序名稱
    const char *class_alias; // 客戶窗口的類型別名
    Area_type area_type; // 客戶窗口的區域類型
    unsigned int border_w, title_bar_h; // 客戶窗口邊框寬度和標題欄高度
};
typedef struct rule_tag Rule;

struct move_info_tag /* 定位器舊、新坐標信息 */
{
    int ox, oy, nx, ny; /* 分別爲定位器舊、新坐標 */
};
typedef struct move_info_tag Move_info;

struct delta_rect_tag /* 調整窗口尺寸的信息 */
{
    int dx, dy, dw, dh; /* 分別爲窗口坐標和尺寸的變化量 */
};
typedef struct delta_rect_tag Delta_rect;

struct string_format_tag // 字符串格式
{
    Rect r; // 坐標和尺寸信息
    ALIGN_TYPE align; // 對齊方式
    unsigned long fg, bg; // 前景色、背景色
};
typedef struct string_format_tag String_format;

void set_signals(void);
void exit_with_perror(const char *s);
void exit_with_msg(const char *msg);
void init_wm(WM *wm);
void set_wm(WM *wm);
int x_fatal_handler(Display *display, XErrorEvent *e);
void set_icccm_atoms(WM *wm);
void create_font_set(WM *wm);
void create_cursors(WM *wm);
void create_taskbar(WM *wm);
void create_taskbar_buttons(WM *wm);
void create_icon_area(WM *wm);
void create_status_area(WM *wm);
void create_cmd_center(WM *wm);
void print_fatal_msg(Display *display, XErrorEvent *e);
void create_clients(WM *wm);
void *malloc_s(size_t size);
bool is_wm_win(WM *wm, Window win);
void add_client(WM *wm, Window win);
Client *get_area_head(WM *wm, Area_type type);
void update_layout(WM *wm);
void fix_cur_focus_client_rect(WM *wm);
void set_full_layout(WM *wm);
void set_preview_layout(WM *wm);
unsigned int get_clients_n(WM *wm);
void set_tile_layout(WM *wm);
void grab_keys(WM *wm);
unsigned int get_num_lock_mask(WM *wm);
void grab_buttons(WM *wm, Client *c);
void handle_events(WM *wm);
void handle_button_press(WM *wm, XEvent *e);
void focus_clicked_client(WM *wm, Window win);
bool is_func_click(WM *wm, Widget_type type, Buttonbind *b, XEvent *e);
void choose_client(WM *wm, XEvent *e, Func_arg arg);
bool is_equal_modifier_mask(WM *wm, unsigned int m1, unsigned int m2);
bool is_click_client_in_preview(WM *wm, Widget_type type);
void handle_config_request(WM *wm, XEvent *e);
void config_managed_client(WM *wm, Client *c);
void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
Client *win_to_client(WM *wm, Window win);
void del_client(WM *wm, Client *c);
void handle_expose(WM *wm, XEvent *e);
void update_icon_text(WM *wm, Window win);
void update_taskbar_button_text(WM *wm, size_t index);
void update_cmd_center_button_text(WM *wm, size_t index);
void handle_key_press(WM *wm, XEvent *e);
unsigned int get_valid_mask(WM *wm, unsigned int mask);
unsigned int get_modifier_mask(WM *wm, KeySym key_sym);
void handle_map_request(WM *wm, XEvent *e);
void handle_unmap_notify(WM *wm, XEvent *e);
void handle_motion_notify(WM *wm, XEvent *e);
void handle_property_notify(WM *wm, XEvent *e);
char *get_text_prop(WM *wm, Window win, Atom atom);
char *copy_string(const char *s);
void draw_string(WM *wm, Drawable d, const char *str, const String_format *f);
void get_string_size(WM *wm, const char *str, unsigned int *w, unsigned int *h);
void exec(WM *wm, XEvent *e, Func_arg arg);
void key_move_resize_client(WM *wm, XEvent *e, Func_arg arg);
bool is_valid_move_resize(WM *wm, Client *c, Delta_rect *d);
void quit_wm(WM *wm, XEvent *e, Func_arg unused);
void close_client(WM *wm, XEvent *e, Func_arg unused);
void close_all_clients(WM *wm, XEvent *e, Func_arg unused);
int send_event(WM *wm, Atom protocol, Client *c);
void next_client(WM *wm, XEvent *e, Func_arg unused);
void prev_client(WM *wm, XEvent *e, Func_arg unused);
void focus_client(WM *wm, Client *c);
void update_focus_client_pointer(WM *wm, Client *c);
Client *get_next_nonicon_client(WM *wm, Client *c);
Client *get_prev_nonicon_client(WM *wm, Client *c);
bool is_client(WM *wm, Client *c);
bool is_icon_client(WM *wm, Client *c);
void change_layout(WM *wm, XEvent *e, Func_arg arg);
void update_title_bar_layout(WM *wm);
void pointer_focus_client(WM *wm, XEvent *e, Func_arg arg);
bool grab_pointer(WM *wm, XEvent *e);
void apply_rules(WM *wm, Client *c);
bool have_rule(Rule *r, Client *c);
void set_default_rect(WM *wm, Client *c);
void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg);
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg);
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg);
void change_area(WM *wm, XEvent *e, Func_arg arg);
void del_client_node(Client *c);
void add_client_node(Client *head, Client *c);
void pointer_change_area(WM *wm, XEvent *e, Func_arg arg);
int compare_client_order(WM *wm, Client *c1, Client *c2);
void move_client(WM *wm, Client *from, Client *to, Area_type type);
bool move_client_node(WM *wm, Client *from, Client *to, Area_type type);
void fix_area_type(WM *wm);
void pointer_swap_clients(WM *wm, XEvent *e, Func_arg unused);
void swap_clients(WM *wm, Client *a, Client *b);
void raise_client(WM *wm);
void frame_client(WM *wm, Client *c);
void create_title_bar(WM *wm, Client *c);
Rect get_frame_rect(Client *c);
Rect get_title_area_rect(WM *wm, Client *c);
Rect get_button_rect(Client *c, size_t index);
void update_title_area_text(WM *wm, Client *c);
void update_title_button_text(WM *wm, Client *c, size_t index);
void update_status_area_text(WM *wm);
void move_resize_client(WM *wm, Client *c, const Delta_rect *d);
void fix_win_rect_for_frame(WM *wm);
bool should_fix_win_rect(WM *wm, Client *c);
void update_frame(WM *wm, Client *c);
void update_win_background(WM *wm, Window win, unsigned long color);
Widget_type get_widget_type(WM *wm, Window win);
void handle_enter_notify(WM *wm, XEvent *e);
void hint_enter_taskbar_button(WM *wm, Widget_type type);
void hint_enter_cmd_center_button(WM *wm, Widget_type type);
void hint_enter_title_button(WM *wm, Client *c, Widget_type type);
void hint_resize_client(WM *wm, Client *c, int x, int y);
bool is_layout_adjust_area(WM *wm, Window win, int x);
void handle_leave_notify(WM *wm, XEvent *e);
void hint_leave_taskbar_button(WM *wm, Widget_type type);
void hint_leave_cmd_center_button(WM *wm, Widget_type type);
void hint_leave_title_button(WM *wm, Client *c, Widget_type type);
void maximize_client(WM *wm, XEvent *e, Func_arg unused);
void iconify(WM *wm, Client *c);
void create_icon(WM *wm, Client *c);
bool have_same_class_icon_client(WM *wm, Client *c);
void update_icon_area(WM *wm);
void deiconify(WM *wm, Client *c);
void del_icon(WM *wm, Client *c);
void update_area_type(WM *wm, Client *c, Area_type type);
void pointer_move_client(WM *wm, XEvent *e, Func_arg arg);
void pointer_resize_client(WM *wm, XEvent *e, Func_arg arg);
Pointer_act get_resize_act(Client *c, const Move_info *m);
Delta_rect get_delta_rect(Client *c, const Move_info *m);
void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg);
bool change_layout_ratio(WM *wm, int ox, int nx);
bool is_main_sec_gap(WM *wm, int x);
bool is_main_fix_gap(WM *wm, int x);
void iconify_all_clients(WM *wm, XEvent *e, Func_arg arg);
void deiconify_all_clients(WM *wm, XEvent *e, Func_arg arg);
Client *win_to_iconic_state_client(WM *wm, Window win);
void change_default_area_type(WM *wm, XEvent *e, Func_arg arg);
void clear_zombies(int unused);
void toggle_focus_mode(WM *wm, XEvent *e, Func_arg arg);
void open_cmd_center(WM *wm, XEvent *e, Func_arg arg);
void toggle_border_visibility(WM *wm, XEvent *e, Func_arg arg);
void toggle_title_bar_visibility(WM *wm, XEvent *e, Func_arg arg);

#endif
