/* *************************************************************************
 *     gwm.h：與gwm.c相應的頭文件。
 *     版權 (C) 2020 gsm <406643764@qq.com>
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
#include <locale.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>

#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))
#define CLIENT_BUTTON_N ARRAY_NUM(CLIENT_BUTTON_TEXT)
#define TASKBAR_BUTTON_N ARRAY_NUM(TASKBAR_BUTTON_TEXT)
#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define BUTTON_MASK (ButtonPressMask|ButtonReleaseMask)
#define POINTER_MASK (BUTTON_MASK|ButtonMotionMask)
#define ROOT_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask|PropertyChangeMask|POINTER_MASK|ExposureMask|EnterWindowMask|LeaveWindowMask)
#define TASKBAR_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask|ExposureMask)
#define BUTTON_EVENT_MASK (ButtonPressMask|ExposureMask|EnterWindowMask|LeaveWindowMask)
#define FRAME_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask|ExposureMask|ButtonPressMask|EnterWindowMask|LeaveWindowMask)
#define TITLE_AREA_EVENT_MASK (ButtonPressMask|ExposureMask|EnterWindowMask|LeaveWindowMask)

enum place_type_tag
{
     NORMAL, FIXED, FLOATING, ICONIFY, PLACE_TYPE_N
};
typedef enum place_type_tag Place_type;

enum area_type_tag
{
     MAIN_AREA, SECOND_AREA, FIXED_AREA, FLOATING_AREA, ICONIFY_AREA,
};
typedef enum area_type_tag Area_type;

enum click_type_tag
{
    UNDEFINED, CLICK_WIN, CLICK_ICON, CLICK_FRAME, CLICK_TITLE, CLICK_ROOT,
    TO_MAIN, TO_SECOND, TO_FIX, TO_FLOAT, ICON_WIN, MAX_WIN, CLOSE_WIN,
    TO_FULL, TO_PREVIEW, TO_STACK, TO_TILE, ADJUST_N_MAIN,
    CLICK_CLIENT_BUTTON_BEGIN=TO_MAIN,
    CLICK_CLIENT_BUTTON_END=CLOSE_WIN,
    CLICK_TASKBAR_BUTTON_BEGIN=TO_FULL,
    CLICK_TASKBAR_BUTTON_END=ADJUST_N_MAIN,
};
typedef enum click_type_tag Click_type;

struct rectangle_tag
{
    int x, y;
    unsigned int w, h;
};
typedef struct rectangle_tag Rect;

struct icon_tag
{
    Window win;
    int x, y; /* 無邊框時的坐標 */
    unsigned int w, h;
    Place_type place_type;
    bool is_pixel_bg;
};
typedef struct icon_tag Icon;

struct client_tag
{
    Window win, frame, title_area, buttons[CLIENT_BUTTON_N];
    int x, y;
    unsigned int w, h;
    Place_type place_type;
    char *title_text;
    Icon icon;
    XClassHint class_hint;
    struct client_tag *prev, *next;
};
typedef struct client_tag Client;

enum layout_tag
{
    FULL, PREVIEW, STACK, TILE,
};
typedef enum layout_tag Layout;

enum pointer_act_tag
{
    NO_OP, MOVE, HORIZ_RESIZE, VERT_RESIZE, TOP_LEFT_RESIZE, TOP_RIGHT_RESIZE,
    BOTTOM_LEFT_RESIZE, BOTTOM_RIGHT_RESIZE, ADJUST_LAYOUT_RATIO, POINTER_ACT_N
};
typedef enum pointer_act_tag Pointer_act;

struct taskbar_tag
{
    Window win, buttons[TASKBAR_BUTTON_N], status_win;
    int x, y;
    unsigned int w, h;
    const char *status_text;
};
typedef struct taskbar_tag Taskbar;

struct wm_tag
{
    Display *display;
    int screen;
    unsigned int screen_width, screen_height;
	XModifierKeymap *mod_map;
    Window root_win;
    GC gc;
    Client *clients, *cur_focus_client, *prev_focus_client;
    unsigned int clients_n[PLACE_TYPE_N], 
        n_main_max; /* 主區域可容納的clients數量 */
    Layout cur_layout, prev_layout;
    XFontSet font_set;
    Cursor cursors[POINTER_ACT_N];
    Taskbar taskbar;
    double main_area_ratio, fixed_area_ratio;
};
typedef struct wm_tag WM;

enum direction_tag
{
    UP, DOWN, LEFT, RIGHT, 
    LEFT2LEFT, LEFT2RIGHT, RIGHT2LEFT, RIGHT2RIGHT,
    UP2UP, UP2DOWN, DOWN2UP, DOWN2DOWN,
};
typedef enum direction_tag Direction;

enum align_type_tag
{
    TOP_LEFT, TOP_CENTER, TOP_RIGHT,
    CENTER_LEFT, CENTER, CENTER_RIGHT,
    BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT,
};
typedef enum align_type_tag ALIGN_TYPE;

union func_arg_tag
{
    char *const *cmd;
    Direction direction;
    Layout layout;
    Pointer_act pointer_act;
    int n;
    Area_type area_type;
    double change_ratio;
};
typedef union func_arg_tag Func_arg;

struct buttonbind_tag
{
    Click_type click_type;
	unsigned int modifier;
    unsigned int button;
	void (*func)(WM *wm, XEvent *e, Func_arg arg);
    Func_arg arg;
};
typedef struct buttonbind_tag Buttonbind;

struct keybind_tag
{
	unsigned int modifier;
	KeySym keysym;
	void (*func)(WM *wm, XEvent *e, Func_arg arg);
    Func_arg arg;
};
typedef struct keybind_tag Keybind;

struct wm_rule_tag
{
    const char *app_class, *app_name;
    Place_type place_type;
};
typedef struct wm_rule_tag WM_rule;

void exit_with_msg(const char *msg);
void init_wm(WM *wm);
void set_wm(WM *wm);
int my_x_error_handler(Display *display, XErrorEvent *e);
void create_font_set(WM *wm);
void create_cursors(WM *wm);
void create_cursor(WM *wm, Pointer_act act, unsigned int shape);
void create_taskbar(WM *wm);
void create_status_area(WM *wm);
void print_fatal_msg(Display *display, XErrorEvent *e);
void create_clients(WM *wm);
void *malloc_s(size_t size);
bool is_wm_win(WM *wm, Window win);
void add_client(WM *wm, Window win);
Client *get_area_head(WM *wm, Place_type type);
void update_layout(WM *wm);
void fix_cur_focus_client_rect(WM *wm);
void iconify_all_for_vision(WM *wm);
void deiconify_all_for_vision(WM *wm);
void set_full_layout(WM *wm);
void set_preview_layout(WM *wm);
unsigned int get_clients_n(WM *wm);
void set_tile_layout(WM *wm);
void grab_keys(WM *wm);
unsigned int get_num_lock_mask(WM *wm);
void grab_buttons(WM *wm, Client *c);
void handle_events(WM *wm);
void handle_button_press(WM *wm, XEvent *e);
bool is_func_click(WM *wm, Click_type type, Buttonbind *b, XEvent *e);
bool is_equal_modifier_mask(WM *wm, unsigned int m1, unsigned int m2);
bool is_click_client_in_preview(WM *wm, Click_type type);
void choose_client_in_preview(WM *wm, Client *c);
void handle_config_request(WM *wm, XEvent *e);
void config_managed_client(WM *wm, Client *c);
void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
Client *win_to_client(WM *wm, Window win);
void del_client(WM *wm, Client *c);
void handle_expose(WM *wm, XEvent *e);
bool update_icon_text(WM *wm, Window win);
bool update_taskbar_button_text(WM *wm, Window win);
void handle_key_press(WM *wm, XEvent *e);
unsigned int get_valid_mask(WM *wm, unsigned int mask);
unsigned int get_modifier_mask(WM *wm, KeySym key_sym);
void handle_map_request(WM *wm, XEvent *e);
void handle_unmap_notify(WM *wm, XEvent *e);
void handle_property_notify(WM *wm, XEvent *e);
char *get_text_prop(WM *wm, Window win, Atom atom);
void draw_string(WM *wm, Drawable drawable, unsigned long color, ALIGN_TYPE align, int x, int y, unsigned int w, unsigned h, const char *str);
void get_string_size(WM *wm, const char *str, unsigned int *w, unsigned int *h);
void exec(WM *wm, XEvent *e, Func_arg arg);
void key_move_resize_client(WM *wm, XEvent *e, Func_arg arg);
bool is_valid_move_resize(WM *wm, Client *c, int dx, int dy, int dw, int dh);
void quit_wm(WM *wm, XEvent *e, Func_arg unused);
void close_win(WM *wm, XEvent *e, Func_arg unused);
int send_event(WM *wm, Atom protocol);
void next_client(WM *wm, XEvent *e, Func_arg unused);
void prev_client(WM *wm, XEvent *e, Func_arg unused);
void focus_client(WM *wm, Client *c);
void fix_focus_client(WM *wm);
Client *get_next_client(WM *wm, Client *c);
Client *get_prev_client(WM *wm, Client *c);
bool is_client(WM *wm, Client *c);
void frame_icon(WM *wm, Icon *icon);
void unframe_icon(WM *wm, Icon *icon);
void change_layout(WM *wm, XEvent *e, Func_arg arg);
void update_title_bar_layout(WM *wm);
void pointer_focus_client(WM *wm, XEvent *e, Func_arg arg);
bool grab_pointer(WM *wm, XEvent *e);
void apply_rules(WM *wm, Client *c);
void set_default_rect(WM *wm, Client *c);
void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg);
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg);
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg);
void change_area(WM *wm, XEvent *e, Func_arg arg);
void warp_pointer_for_key_press(WM *wm, Client *c, int event_type);
void to_main_area(WM *wm, Client *c);
bool is_in_main_area(WM *wm, Client *c);
void del_client_node(Client *c);
void add_client_node(Client *head, Client *c);
void to_second_area(WM *wm, Client *c);
Client *get_second_area_head(WM *wm);
void to_fixed_area(WM *wm, Client *c);
void to_floating_area(WM *wm, Client *c);
void pointer_change_area(WM *wm, XEvent *e, Func_arg arg);
int compare_client_order(WM *wm, Client *c1, Client *c2);
void move_client(WM *wm, Client *from, Client *to, Place_type type);
void raise_client(WM *wm);
void frame_client(WM *wm, Client *c);
Rect get_frame_rect(Client *c);
Rect get_title_area_rect(WM *wm, Client *c);
Rect get_button_rect(Client *c, size_t index);
void update_title_area_text(WM *wm, Client *c);
bool update_title_button_text(WM *wm, Client *c, Window win);
void update_status_area_text(WM *wm);
void move_resize_client(WM *wm, Client *c, int dx, int dy, unsigned int dw, unsigned int dh);
void fix_win_rect(Client *c);
void update_frame(WM *wm, Client *c);
void update_win_background(WM *wm, Window win, unsigned long color);
Click_type get_click_type(WM *wm, Window win);
void handle_enter_notify(WM *wm, XEvent *e);
void hint_enter_taskbar_button(WM *wm, Window win);
void hint_enter_client_button(WM *wm, Window win);
void hint_resizing(WM *wm, Window win, int x, int y);
void hint_motion(WM *wm, Window win, int x, int y);
void handle_leave_notify(WM *wm, XEvent *e);
void hint_leave_taskbar_button(WM *wm, Window win);
void hint_leave_client_button(WM *wm, Window win);
void maximize_client(WM *wm, XEvent *e, Func_arg unused);
void iconify(WM *wm, Client *c);
void create_icon(WM *wm, Client *c);
void get_drawable_size(WM *wm, Drawable d, unsigned int *w, unsigned int *h);
void set_icon_position(WM *wm, Client *c);
void key_choose_client(WM *wm, XEvent *e, Func_arg arg);
void pointer_deiconify(WM *wm, XEvent *e, Func_arg arg);
void deiconify(WM *wm, Client *c);
void update_client_n_and_place_type(WM *wm, Client *c, Place_type type);
void pointer_move_client(WM *wm, XEvent *e, Func_arg arg);
void pointer_resize_client(WM *wm, XEvent *e, Func_arg arg);
Pointer_act get_resize_incr(Client *c, int ox, int oy, int nx, int ny, int *dx, int *dy, int *dw, int *dh);
void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg);
bool change_layout_ratio(WM *wm, int ox, int nx);
bool is_main_sec_space(WM *wm, int x);
bool is_main_fix_space(WM *wm, int x);

#endif
