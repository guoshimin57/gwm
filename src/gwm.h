/* *************************************************************************
 *     gwm.h：與gwm.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef GWM_H
#define GWM_H

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <wchar.h>
#include <dirent.h>
#include <Imlib2.h>
#include <locale.h>
#include <libintl.h>
#include <math.h>
#include <X11/cursorfont.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>
#include <X11/XF86keysym.h>
#include <X11/Xft/Xft.h>
#include <X11/Xproto.h>

#include "misc.h"
#include "widget.h"

#define _(s) gettext(s)

#define NET_WM_STATE_REMOVE 0
#define NET_WM_STATE_ADD    1
#define NET_WM_STATE_TOGGLE 2

#define SHOULD_ADD_STATE(c, act, flag) \
    (act==NET_WM_STATE_ADD || (act==NET_WM_STATE_TOGGLE && !c->win_state.flag))


#define SET_NULL(array, n) for(size_t i=0; i<n; i++) array[i]=NULL
#define UNUSED(x) ((void)(x))
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define MAX(a, b) ((a)>(b) ? (a) : (b))
#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))
#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define FUNC_ARG(var, data) (Func_arg){.var=data}

#define FONT_NAME_MAX 64

#define BUTTON_MASK (ButtonPressMask|ButtonReleaseMask)
#define POINTER_MASK (BUTTON_MASK|ButtonMotionMask)
#define CROSSING_MASK (EnterWindowMask|LeaveWindowMask)
#define ROOT_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    PropertyChangeMask|ButtonPressMask|CROSSING_MASK|ExposureMask|KeyPressMask)

#define DESKTOP(wm) (wm->desktop[get_net_current_desktop()])
#define CUR_FOC_CLI(wm) DESKTOP(wm)->cur_focus_client

typedef struct desktop_tag Desktop;
typedef struct client_tag Client;

typedef struct // 與X相關的信息
{
    Display *display; // 顯示器
    int screen, screen_width, screen_height; // 屏幕號、屏幕寬度和高度
    unsigned int depth; // 色深
    Visual *visual; // 着色類型
    Colormap colormap; // 着色圖
	XModifierKeymap *mod_map; // 功能轉換鍵映射
    XIM xim; // 輸入法
    Window root_win; // 根窗口
    Window hint_win; // 提示窗口
} Xinfo;

enum place_type_tag // 窗口的位置類型
{
    FULLSCREEN_LAYER, ABOVE_LAYER, DOCK_LAYER, FLOAT_LAYER,
    TILE_LAYER_MAIN, TILE_LAYER_SECOND, TILE_LAYER_FIXED,
    BELOW_LAYER, DESKTOP_LAYER, ANY_PLACE, PLACE_TYPE_N=ANY_PLACE
};
typedef enum place_type_tag Place_type;

struct rectangle_tag // 矩形窗口或區域的坐標和尺寸
{
    int x, y; // 坐標
    int w, h; // 尺寸
};
typedef struct rectangle_tag Rect;

typedef struct /* 調整窗口尺寸的信息 */
{
    int dx, dy, dw, dh; /* 分別爲窗口坐標和尺寸的變化量 */
} Delta_rect;

enum layout_tag // 窗口管理器的布局模式
{
    PREVIEW, STACK, TILE,
};
typedef enum layout_tag Layout;

struct rule_tag // 窗口管理器的規則
{// 分別爲客戶窗口的程序類型和程序名稱、標題，NULL或*表示匹配任何字符串
    const char *app_class, *app_name, *title;
    const char *class_alias; // 客戶窗口的類型別名
    Place_type place_type; // 客戶窗口的位置類型
    bool show_titlebar, show_border; // 是否顯示客戶窗口標題欄和邊框
    unsigned int desktop_mask; // 客戶窗口所属虚拟桌面掩碼
};
typedef struct rule_tag Rule;

typedef enum // 窗口疊次序分層類型
{
    DESKTOP_TOP, BELOW_TOP, NORMAL_TOP, FLOAT_TOP, DOCK_TOP, ABOVE_TOP,
    FULLSCREEN_TOP, TOP_WIN_TYPE_N
} Top_win_type;

typedef struct wm_tag WM;
typedef void (*event_handler_type)(WM*, XEvent *); // 事件處理器類型

struct wm_tag // 窗口管理器相關信息
{
    Desktop *desktop[DESKTOP_N]; // 虛擬桌面
    Rect workarea; // 工作區坐標和尺寸
    Window wm_check_win; // WM檢測窗口
    Window top_wins[TOP_WIN_TYPE_N]; // 窗口疊次序分層參照窗口列表，即分層層頂窗口
    Client *clients; // 頭結點
    Strings *wallpapers, *cur_wallpaper; // 壁紙文件列表、当前壁纸文件
    event_handler_type handle_event; // 事件處理器
};

enum direction_tag // 方向
{
    UP, DOWN, LEFT, RIGHT, LEFT2LEFT, LEFT2RIGHT, RIGHT2LEFT, RIGHT2RIGHT,
    UP2UP, UP2DOWN, DOWN2UP, DOWN2DOWN,
};
typedef enum direction_tag Direction;

enum max_way_tag // 窗口最大化的方式
{
    VERT_MAX, HORZ_MAX,
    TOP_MAX, BOTTOM_MAX, LEFT_MAX, RIGHT_MAX, FULL_MAX,
};
typedef enum max_way_tag Max_way;

union func_arg_tag // 函數參數類型
{
    bool resize; // 是否調整窗口尺寸
    bool focus; // 是否聚焦的標志
    char *const *cmd; // 命令字符串
    Direction direction; // 方向
    Layout layout; // 窗口布局模式
    Pointer_act pointer_act; // 窗口操作類型
    Max_way max_way; // 窗口最大化的方式
    int n; // 表示數量
    unsigned int desktop_n; // 虛擬桌面編號，從0開始編號
    Place_type place_type; // 窗口位置類型
    double change_ratio; // 變化率
};
typedef union func_arg_tag Func_arg;

struct keybind_tag // 鍵盤按鍵功能綁定
{
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵
	KeySym keysym; // 要綁定的鍵盤功能轉換鍵
	void (*func)(WM *, XEvent *, Func_arg); // 要綁定的函數
    Func_arg arg; // 要綁定的函數的參數
};
typedef struct keybind_tag Keybind;

struct buttonbind_tag // 定位器按鈕功能綁定
{
    Widget_id widget_id; // 要綁定的構件標識
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵 
    unsigned int button; // 要綁定的定位器按鈕
	void (*func)(WM *, XEvent *, Func_arg); // 要綁定的函數
    Func_arg arg; // 要綁定的函數的參數
};
typedef struct buttonbind_tag Buttonbind;

struct move_info_tag /* 定位器所點擊的窗口位置每次合理移動或調整尺寸所對應的舊、新坐標信息 */
{
    int ox, oy, nx, ny; /* 分別爲舊、新坐標 */
};
typedef struct move_info_tag Move_info;

extern sig_atomic_t run_flag; // 程序運行標志
extern Xinfo xinfo;

#include "client.h"
#include "desktop.h"

#endif
