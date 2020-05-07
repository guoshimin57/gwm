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
#include <stdlib.h>
#include <stdbool.h>
#include <locale.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xproto.h>

struct client_tag
{
    Window win;
    int x, y;
    unsigned int w, h;
    bool is_float;
    struct client_tag *prev, *next;
};
typedef struct client_tag CLIENT;

enum layout_tag
{
    full, grid, stack,
};
typedef enum layout_tag LAYOUT;

struct wm_tag
{
    Display *display;
    int screen;
    unsigned int screen_width, screen_height;
    unsigned long black, white;
	XModifierKeymap *mod_map;
    Window root_win;
    GC root_gc;
    CLIENT *clients, *focus_client;
    unsigned int n, n_nonfloat; /* 依次爲clients總數量、非懸浮的clients的數量 */
    LAYOUT layout;
};
typedef struct wm_tag WM;

enum direction_tag /* 窗口、區域操作方向 */
{
    up, down, left, right, /* 窗口移動方向 */
    left2left, left2right, right2left, right2right, /* 左右邊界移動方向 */
    up2up, up2down, down2up, down2down, /* 上下邊界移動方向 */
};
typedef enum direction_tag DIRECTION;

union keybinds_func_arg_tag
{
    char *const *cmd;
    DIRECTION direction;
    LAYOUT layout;
};
typedef union keybinds_func_arg_tag KB_FUNC_ARG;

struct keybinds_tag
{
	unsigned int modifier;
	KeySym keysym;
	void (*func)(WM *wm, KB_FUNC_ARG arg);
    KB_FUNC_ARG arg;
};
typedef struct keybinds_tag KEYBINDS;

#define WM_KEY Mod4Mask
#define CMD_KEY (Mod4Mask|Mod1Mask)
#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))
#define STACK_INDENT 32
#define MOVE_INC 32
#define RESIZE_INC 32

void init_wm(WM *wm);
int my_x_error_handler(Display *display, XErrorEvent *e);
void print_error_msg(Display *display, XErrorEvent *e);
void init_wm_struct(WM *wm);
void create_clients(WM *wm);
void *Malloc(size_t size);
int get_state_hint(WM *wm, Window w);
void add_client(WM *wm, Window win);
void update_layout(WM *wm);
void set_full_layout(WM *wm);
void set_grid_layout(WM *wm);
void set_stack_layout(WM *wm);
void grab_keys(WM *wm);
void handle_events(WM *wm);
void handle_config_request(WM *wm, XEvent *e);
void config_managed_win(WM *wm, CLIENT *c);
void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
CLIENT *win_to_client(WM *wm, Window win);
void handle_destroy_notify(WM *wm, XEvent *e);
void del_client(WM *wm, Window win);
void handle_key_press(WM *wm, XEvent *e);
unsigned int get_valid_mask(WM *wm, unsigned int mask);
unsigned int get_modifier_mask(WM *wm, KeySym key_sym);
void handle_map_request(WM *wm, XEvent *e);
void handle_unmap_notify(WM *wm, XEvent *e);
void exec(WM *wm, KB_FUNC_ARG arg);
void key_move_win(WM *wm, KB_FUNC_ARG arg);
void prepare_for_move_resize(WM *wm);
void key_resize_win(WM *wm, KB_FUNC_ARG arg);
void quit_wm(WM *wm, KB_FUNC_ARG unused);
void close_win(WM *wm, KB_FUNC_ARG unused);
int send_event(WM *wm, Atom protocol);
void next_win(WM *wm, KB_FUNC_ARG unused);
void toggle_float(WM *wm, KB_FUNC_ARG unused);
void change_layout(WM *wm, KB_FUNC_ARG arg);
