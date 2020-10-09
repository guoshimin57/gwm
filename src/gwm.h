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

enum place_type_tag
{
    normal, floating, fixed,
};
typedef enum place_type_tag PLACE_TYPE;

struct client_tag
{
    Window win;
    int x, y;
    unsigned int w, h;
    PLACE_TYPE place_type;
    struct client_tag *prev, *next;
};
typedef struct client_tag CLIENT;

enum layout_tag
{
    full, preview, stack, tile,
};
typedef enum layout_tag LAYOUT;

struct status_bar_tag
{
    Window win;
    int x, y;
    unsigned int w, h;
};
typedef struct status_bar_tag STATUS_BAR;

struct wm_tag
{
    Display *display;
    int screen;
    unsigned int screen_width, screen_height;
    unsigned long black, white;
	XModifierKeymap *mod_map;
    Window root_win;
    GC gc, wm_gc;
    CLIENT *clients, *focus_client;
    unsigned int n, /* 除頭結點以外的clients總數 */
        n_float,    /* 懸浮的clients數量 */
        n_fixed,    /* 固定區域的clients數量 */
        n_normal;   /* 除頭結點和以上兩種結點的clients數量 */
    LAYOUT layout;
    XFontSet font_set;
    STATUS_BAR status_bar;
    float main_area_ratio, fixed_area_ratio;
};
typedef struct wm_tag WM;

enum direction_tag /* 窗口、區域操作方向 */
{
    up, down, left, right, /* 窗口移動方向 */
    left2left, left2right, right2left, right2right, /* 左右邊界移動方向 */
    up2up, up2down, down2up, down2down, /* 上下邊界移動方向 */
};
typedef enum direction_tag DIRECTION;

union func_arg_tag
{
    char *const *cmd;
    DIRECTION direction;
    LAYOUT layout;
    bool resize_flag;
};
typedef union func_arg_tag FUNC_ARG;

struct buttonbinds_tag
{
	unsigned int modifier;
    unsigned int button;
	void (*func)(WM *wm, XEvent *e, FUNC_ARG arg);
    FUNC_ARG arg;
};
typedef struct buttonbinds_tag BUTTONBINDS;

struct keybinds_tag
{
	unsigned int modifier;
	KeySym keysym;
	void (*func)(WM *wm, XEvent *e, FUNC_ARG arg);
    FUNC_ARG arg;
};
typedef struct keybinds_tag KEYBINDS;

struct wm_rule_tag
{
    const char *app_class, *app_name;
    PLACE_TYPE place_type;
};
typedef struct wm_rule_tag WM_RULE;

#define WM_KEY Mod4Mask
#define CMD_KEY (Mod4Mask|Mod1Mask)
#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))
#define MOVE_INC 32
#define RESIZE_INC 32
#define STATUS_BAR_HEIGHT 32
#define DEFAULT_MAIN_AREA_RATIO 0.6
#define DEFAULT_FIXED_AREA_RATIO 0.15
#define POINTER_MASK (ButtonPressMask|ButtonReleaseMask|ButtonMotionMask)
#define FONT_SET "*-24-*"

void init_wm(WM *wm);
void set_wm(WM *wm);
int my_x_error_handler(Display *display, XErrorEvent *e);
void create_font_set(WM *wm);
void create_status_bar(WM *wm);
void print_error_msg(Display *display, XErrorEvent *e);
void create_clients(WM *wm);
void *Malloc(size_t size);
int get_state_hint(WM *wm, Window w);
void add_client(WM *wm, Window win);
void update_layout(WM *wm);
void set_full_layout(WM *wm);
void set_preview_layout(WM *wm);
void set_stack_layout(WM *wm);
void set_tile_layout(WM *wm);
void grab_keys(WM *wm);
unsigned int get_num_lock_mask(WM *wm);
void grab_buttons(WM *wm);
void handle_events(WM *wm);
void handle_button_press(WM *wm, XEvent *e);
void handle_config_request(WM *wm, XEvent *e);
void config_managed_win(WM *wm, CLIENT *c);
void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
CLIENT *win_to_client(WM *wm, Window win);
void handle_destroy_notify(WM *wm, XEvent *e);
void del_client(WM *wm, Window win);
void handle_expose(WM *wm, XEvent *eent);
void handle_key_press(WM *wm, XEvent *e);
unsigned int get_valid_mask(WM *wm, unsigned int mask);
unsigned int get_modifier_mask(WM *wm, KeySym key_sym);
void handle_map_request(WM *wm, XEvent *e);
void handle_unmap_notify(WM *wm, XEvent *e);
void handle_property_notify(WM *wm, XEvent *e);
void draw_string_in_center(WM *wm, Drawable drawable, XFontSet font_set, GC gc, int x, int y, unsigned int w, unsigned h, const char *str);
void exec(WM *wm, XEvent *e, FUNC_ARG arg);
void key_move_win(WM *wm, XEvent *e, FUNC_ARG arg);
void prepare_for_move_resize(WM *wm);
void key_resize_win(WM *wm, XEvent *e, FUNC_ARG arg);
void quit_wm(WM *wm, XEvent *e, FUNC_ARG unused);
void close_win(WM *wm, XEvent *e, FUNC_ARG unused);
int send_event(WM *wm, Atom protocol);
void next_win(WM *wm, XEvent *e, FUNC_ARG unused);
void focus_client(WM *wm, CLIENT *c);
void raise_float_wins(WM *wm);
void toggle_float(WM *wm, XEvent *e, FUNC_ARG unused);
void change_layout(WM *wm, XEvent *e, FUNC_ARG arg);
void pointer_move_resize_win(WM *wm, XEvent *e, FUNC_ARG arg);
bool grab_pointer_for_move_resize(WM *wm);
bool query_pointer_for_move_resize(WM *wm, int *x, int *y, Window *win);
void get_rect_sign(WM *wm, int px, int py, bool resize_flag, int *xs, int *ys, int *ws, int *hs);
void apply_rules(WM *wm, CLIENT *c);
void set_default_rect(WM *wm, CLIENT *c);
