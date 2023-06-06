/* *************************************************************************
 *     gwm.h：與gwm.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
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

#define ICCCM_NAMES (const char *[]) {"WM_PROTOCOLS", "WM_DELETE_WINDOW", "WM_TAKE_FOCUS"}
#define EWMH_NAME (const char *[]) \
{ \
    "_NET_SUPPORTED", "_NET_CLIENT_LIST", "_NET_CLIENT_LIST_STACKING", \
    "_NET_NUMBER_OF_DESKTOPS", "_NET_DESKTOP_GEOMETRY", \
    "_NET_DESKTOP_VIEWPORT", "_NET_CURRENT_DESKTOP", "_NET_DESKTOP_NAMES", \
    "_NET_ACTIVE_WINDOW", "_NET_WORKAREA", "_NET_SUPPORTING_WM_CHECK", \
    "_NET_SHOWING_DESKTOP", "_NET_CLOSE_WINDOW", "_NET_MOVERESIZE_WINDOW", \
    "_NET_WM_MOVERESIZE", "_NET_WM_MOVERESIZE_SIZE_RIGHT", \
    "_NET_RESTACK_WINDOW", "_NET_REQUEST_FRAME_EXTENTS", \
    "_NET_WM_NAME", "_NET_WM_DESKTOP", \
    "_NET_WM_WINDOW_TYPE", \
    "_NET_WM_WINDOW_TYPE_NORMAL", "_NET_WM_WINDOW_TYPE_DIALOG", \
    "_NET_WM_WINDOW_TYPE_UTILITY", \
    "_NET_WM_STATE", "_NET_WM_STATE_MODAL", "_NET_WM_STATE_STICKY", \
    "_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_STATE_MAXIMIZED_HORZ", \
    "_NET_WM_STATE_SHADED", "_NET_WM_STATE_SKIP_TASKBAR", \
    "_NET_WM_STATE_PAGER", "_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_FULLSCREEN", \
    "_NET_WM_STATE_ABOVE", "_NET_WM_STATE_BELOW", \
    "_NET_WM_STATE_DEMANDS_ATTENTION", "_NET_WM_STATE_FOCUSED", \
    "_NET_WM_ICON", \
} 

#define _(s) gettext(s)

#define SET_NULL(array, n) for(size_t i=0; i<n; i++) array[i]=NULL
#define UNUSED(x) ((void)(x))
#define MIN(a, b) ((a)<(b) ? (a) : (b))
#define MAX(a, b) ((a)>(b) ? (a) : (b))
#define SET_DEF_VAL(var, value) ((var) = (var) ? (var) : (value))
#define ARRAY_NUM(a) (sizeof(a)/sizeof(a[0]))
#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define FUNC_ARG(var, data) (Func_arg){.var=data}

#define TITLE_BUTTON_N (TITLE_BUTTON_END-TITLE_BUTTON_BEGIN+1)
#define TASKBAR_BUTTON_N (TASKBAR_BUTTON_END-TASKBAR_BUTTON_BEGIN+1)
#define CMD_CENTER_ITEM_N (CMD_CENTER_ITEM_END-CMD_CENTER_ITEM_BEGIN+1)
#define DESKTOP_N (DESKTOP_BUTTON_END-DESKTOP_BUTTON_BEGIN+1)
#define FONT_NAME_MAX 64


#define BUTTON_MASK (ButtonPressMask|ButtonReleaseMask)
#define POINTER_MASK (BUTTON_MASK|ButtonMotionMask)
#define CROSSING_MASK (EnterWindowMask|LeaveWindowMask)
#define ROOT_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    PropertyChangeMask|ButtonPressMask|CROSSING_MASK|ExposureMask|KeyPressMask)
#define BUTTON_EVENT_MASK (BUTTON_MASK|ExposureMask|CROSSING_MASK)
#define FRAME_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    ExposureMask|ButtonPressMask|CROSSING_MASK|FocusChangeMask)
#define TITLE_AREA_EVENT_MASK (ButtonPressMask|ExposureMask|CROSSING_MASK)
#define ICON_WIN_EVENT_MASK (BUTTON_EVENT_MASK|PointerMotionMask)
#define ENTRY_EVENT_MASK (ButtonPressMask|KeyPressMask|ExposureMask)

#define TITLE_BUTTON_INDEX(type) ((type)-TITLE_BUTTON_BEGIN)
#define IS_TITLE_BUTTON(type) \
    ((type)>=TITLE_BUTTON_BEGIN && (type)<=TITLE_BUTTON_END)

#define TASKBAR_BUTTON_INDEX(type) ((type)-TASKBAR_BUTTON_BEGIN)
#define IS_TASKBAR_BUTTON(type) \
    ((type)>=TASKBAR_BUTTON_BEGIN && (type)<=TASKBAR_BUTTON_END)

#define CMD_CENTER_ITEM_INDEX(type) ((type)-CMD_CENTER_ITEM_BEGIN)
#define IS_CMD_CENTER_ITEM(type) \
    ((type)>=CMD_CENTER_ITEM_BEGIN && (type)<=CMD_CENTER_ITEM_END)

#define DESKTOP(wm) (wm->desktop[wm->cur_desktop-1])
#define CUR_FOC_CLI(wm) DESKTOP(wm)->cur_focus_client

typedef struct config_tag Config;
typedef struct desktop_tag Desktop;
typedef struct taskbar_tag Taskbar;
typedef struct icon_tag Icon;
typedef struct client_tag Client;
typedef struct menu_tag Menu;
typedef struct entry_tag Entry;
typedef struct string_format_tag String_format;

struct file_tag
{
    char *name;
    struct file_tag *next;
};
typedef struct file_tag File;

enum focus_mode_tag // 窗口聚焦模式
{
    ENTER_FOCUS, CLICK_FOCUS,
};
typedef enum focus_mode_tag Focus_mode;

enum area_type_tag // 窗口的區域類型
{
     MAIN_AREA, SECOND_AREA, FIXED_AREA, FLOATING_AREA, ICONIFY_AREA,
     PREV_AREA, ROOT_AREA,
};
typedef enum area_type_tag Area_type;

enum widget_type_tag // 構件類型
{
    UNDEFINED, ROOT_WIN, STATUS_AREA, RUN_CMD_ENTRY,
    CLIENT_WIN, CLIENT_FRAME, TITLE_AREA, HINT_WIN, CLIENT_ICON,

    SECOND_BUTTON, MAIN_BUTTON, FIXED_BUTTON, FLOAT_BUTTON,
    ICON_BUTTON, MAX_BUTTON, CLOSE_BUTTON,

    DESKTOP1_BUTTON, DESKTOP2_BUTTON, DESKTOP3_BUTTON, 

    FULL_BUTTON, PREVIEW_BUTTON, STACK_BUTTON, TILE_BUTTON, DESKTOP_BUTTON,

    CMD_CENTER_ITEM,
    HELP_BUTTON, FILE_BUTTON, TERM_BUTTON, BROWSER_BUTTON, 
    PLAY_START_BUTTON, PLAY_TOGGLE_BUTTON, PLAY_QUIT_BUTTON,
    VOLUME_DOWN_BUTTON, VOLUME_UP_BUTTON, VOLUME_MAX_BUTTON, VOLUME_TOGGLE_BUTTON,
    MAIN_NEW_BUTTON, SEC_NEW_BUTTON, FIX_NEW_BUTTON, FLOAT_NEW_BUTTON,
    ICON_NEW_BUTTON, N_MAIN_UP_BUTTON, N_MAIN_DOWN_BUTTON, FOCUS_MODE_BUTTON,
    QUIT_WM_BUTTON, LOGOUT_BUTTON, REBOOT_BUTTON, POWEROFF_BUTTON, RUN_BUTTON,

    WIDGET_N,
    TITLE_BUTTON_BEGIN=SECOND_BUTTON, TITLE_BUTTON_END=CLOSE_BUTTON,
    TASKBAR_BUTTON_BEGIN=DESKTOP1_BUTTON, TASKBAR_BUTTON_END=CMD_CENTER_ITEM,
    LAYOUT_BUTTON_BEGIN=FULL_BUTTON, LAYOUT_BUTTON_END=TILE_BUTTON, 
    DESKTOP_BUTTON_BEGIN=DESKTOP1_BUTTON, DESKTOP_BUTTON_END=DESKTOP3_BUTTON,
    CMD_CENTER_ITEM_BEGIN=HELP_BUTTON, CMD_CENTER_ITEM_END=RUN_BUTTON,
};
typedef enum widget_type_tag Widget_type;

enum font_type_tag // 字體類型, 按字符顯示位置分類
{
    DEFAULT_FONT, TITLE_BUTTON_FONT, CMD_CENTER_FONT, TASKBAR_BUTTON_FONT,
    CLASS_FONT, TITLE_FONT, STATUS_AREA_FONT, ENTRY_FONT, HINT_FONT,
    FONT_N
};
typedef enum font_type_tag Font_type;

struct rectangle_tag // 矩形窗口或區域的坐標和尺寸
{
    int x, y; // 坐標
    unsigned int w, h; // 尺寸
};
typedef struct rectangle_tag Rect;

enum layout_tag // 窗口管理器的布局模式
{
    FULL, PREVIEW, STACK, TILE,
};
typedef enum layout_tag Layout;

enum pointer_act_tag // 定位器操作類型
{
    NO_OP, CHOOSE, MOVE, SWAP, CHANGE, TOP_RESIZE, BOTTOM_RESIZE, LEFT_RESIZE,
    RIGHT_RESIZE, TOP_LEFT_RESIZE, TOP_RIGHT_RESIZE, BOTTOM_LEFT_RESIZE,
    BOTTOM_RIGHT_RESIZE, ADJUST_LAYOUT_RATIO, POINTER_ACT_N
};
typedef enum pointer_act_tag Pointer_act;

enum icccm_atom_tag // icccm規範的標識符
{
    WM_PROTOCOLS, WM_DELETE_WINDOW, WM_STATE, WM_TAKE_FOCUS, ICCCM_ATOMS_N
};
typedef enum icccm_atom_tag Icccm_atom;

enum ewmh_atom_tag // EWMH規範的標識符
{
    _NET_SUPPORTED, _NET_CLIENT_LIST, _NET_CLIENT_LIST_STACKING,
    _NET_NUMBER_OF_DESKTOPS, _NET_DESKTOP_GEOMETRY, _NET_DESKTOP_VIEWPORT,
    _NET_CURRENT_DESKTOP, _NET_DESKTOP_NAMES, _NET_ACTIVE_WINDOW, _NET_WORKAREA,
    _NET_SUPPORTING_WM_CHECK, _NET_SHOWING_DESKTOP, _NET_CLOSE_WINDOW,
    _NET_MOVERESIZE_WINDOW, _NET_WM_MOVERESIZE, 
    _NET_WM_MOVERESIZE_SIZE_RIGHT,
    _NET_RESTACK_WINDOW,
    _NET_REQUEST_FRAME_EXTENTS, _NET_WM_NAME, _NET_WM_DESKTOP,
    _NET_WM_WINDOW_TYPE, _NET_WM_WINDOW_TYPE_NORMAL, _NET_WM_WINDOW_TYPE_DIALOG,
    _NET_WM_WINDOW_TYPE_UTILITY,
    _NET_WM_STATE, _NET_WM_STATE_MODAL, _NET_WM_STATE_STICKY,
    _NET_WM_STATE_MAXIMIZED_VERT, _NET_WM_STATE_MAXIMIZED_HORZ,
    _NET_WM_STATE_SHADED, _NET_WM_STATE_SKIP_TASKBAR,
    _NET_WM_STATE_PAGER, _NET_WM_STATE_HIDDEN, _NET_WM_STATE_FULLSCREEN,
    _NET_WM_STATE_ABOVE, _NET_WM_STATE_BELOW,
    _NET_WM_STATE_DEMANDS_ATTENTION, _NET_WM_STATE_FOCUSED,
    _NET_WM_ICON, EWMH_ATOM_N
};
typedef enum ewmh_atom_tag Ewmh_atom;

enum widget_color_tag // 構件顏色類型
{
    NORMAL_BORDER_COLOR, CURRENT_BORDER_COLOR,
    NORMAL_TITLE_AREA_COLOR, CURRENT_TITLE_AREA_COLOR,
    NORMAL_TITLE_BUTTON_COLOR, CURRENT_TITLE_BUTTON_COLOR,
    ENTERED_NORMAL_BUTTON_COLOR, ENTERED_CLOSE_BUTTON_COLOR,
    NORMAL_TASKBAR_BUTTON_COLOR, CHOSEN_TASKBAR_BUTTON_COLOR,
    CMD_CENTER_COLOR, ICON_COLOR, ICON_AREA_COLOR, STATUS_AREA_COLOR,
    ENTRY_COLOR, HINT_WIN_COLOR, ROOT_WIN_COLOR,
    WIDGET_COLOR_N 
};
typedef enum widget_color_tag Widget_color;

enum text_color_tag // 文本顏色類型
{
    NORMAL_TITLE_TEXT_COLOR, CURRENT_TITLE_TEXT_COLOR,
    NORMAL_TITLE_BUTTON_TEXT_COLOR, CURRENT_TITLE_BUTTON_TEXT_COLOR,
    TASKBAR_BUTTON_TEXT_COLOR, STATUS_AREA_TEXT_COLOR, CLASS_TEXT_COLOR,
    CMD_CENTER_ITEM_TEXT_COLOR, ENTRY_TEXT_COLOR, HINT_TEXT_COLOR,
    TEXT_COLOR_N 
};
typedef enum text_color_tag Text_color;

enum color_theme_tag // 顏色主題
{
    DARK_THEME, NORMAL_THEME, LIGHT_THEME, COLOR_THEME_N
};
typedef enum color_theme_tag Color_theme;

struct rule_tag // 窗口管理器的規則
{
    const char *app_class, *app_name; // 分別爲客戶窗口的程序類型和程序名稱
    const char *class_alias; // 客戶窗口的類型別名
    Area_type area_type; // 客戶窗口的區域類型
    bool show_title_bar, show_border; // 是否顯示客戶窗口標題欄和邊框
    unsigned int desktop_mask; // 客戶窗口所属虚拟桌面掩碼
};
typedef struct rule_tag Rule;

struct wm_tag // 窗口管理器相關信息
{
    Display *display; // 顯示器
    int screen; // 屏幕
    unsigned int screen_width, screen_height; // 屏幕寬度、高度
    unsigned int cur_desktop; // 當前虛擬桌面編號，從1開始編號
    Desktop *desktop[DESKTOP_N]; // 虛擬桌面
	XModifierKeymap *mod_map; // 功能轉換鍵映射
    Rect workarea; // 工作區坐標和尺寸
    Window root_win, hint_win, wm_check_win; // 根窗口、提示窗口、WM檢測窗口
    GC gc; // 窗口管理器的圖形信息
    Visual *visual; // 着色類型
    Colormap colormap; // 着色圖
    Atom icccm_atoms[ICCCM_ATOMS_N]; // icccm規範的標識符
    Atom ewmh_atom[EWMH_ATOM_N]; // ewmh規範的標識符
    Atom utf8; // utf8字符编码的標識符
    Client *clients; // 頭結點
    XftFont *font[FONT_N]; // 窗口管理器用到的字體
    File *wallpapers, *cur_wallpaper; // 壁紙文件列表、当前壁纸文件
    Cursor cursors[POINTER_ACT_N]; // 光標
    Taskbar *taskbar; // 任務欄
    Menu *cmd_center; // 操作中心
    Entry *run_cmd; // 輸入命令並執行的構件
    XColor widget_color[COLOR_THEME_N][WIDGET_COLOR_N]; // 構件顏色
    XftColor text_color[COLOR_THEME_N][TEXT_COLOR_N]; // 文本顏色
    XIM xim; // 輸入法
    void (*event_handlers[LASTEvent])(struct wm_tag*, XEvent *); // 事件處理器數組
    Config *cfg; // 窗口管理器配置
};
typedef struct wm_tag WM;

enum direction_tag // 方向
{
    UP, DOWN, LEFT, RIGHT, 
    LEFT2LEFT, LEFT2RIGHT, RIGHT2LEFT, RIGHT2RIGHT,
    UP2UP, UP2DOWN, DOWN2UP, DOWN2DOWN,
};
typedef enum direction_tag Direction;

union func_arg_tag // 函數參數類型
{
    bool resize; // 是否調整窗口尺寸
    bool focus; // 是否聚焦的標志
    char *const *cmd; // 命令字符串
    Direction direction; // 方向
    Layout layout; // 窗口布局模式
    Pointer_act pointer_act; // 窗口操作類型
    int n; // 表示數量
    unsigned int desktop_n; // 虛擬桌面編號，從1開始編號
    Area_type area_type; // 窗口區域類型
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
    Widget_type widget_type; // 要綁定的構件類型
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵 
    unsigned int button; // 要綁定的定位器按鈕
	void (*func)(WM *, XEvent *, Func_arg); // 要綁定的函數
    Func_arg arg; // 要綁定的函數的參數
};
typedef struct buttonbind_tag Buttonbind;

enum align_type_tag // 文字對齊方式
{
    TOP_LEFT, TOP_CENTER, TOP_RIGHT,
    CENTER_LEFT, CENTER, CENTER_RIGHT,
    BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT,
};
typedef enum align_type_tag Align_type;

struct move_info_tag /* 定位器所點擊的窗口位置每次合理移動或調整尺寸所對應的舊、新坐標信息 */
{
    int ox, oy, nx, ny; /* 分別爲舊、新坐標 */
};
typedef struct move_info_tag Move_info;

struct delta_rect_tag /* 調整窗口尺寸的信息 */
{
    int dx, dy, dw, dh; /* 分別爲窗口坐標和尺寸的變化量 */
};
typedef struct delta_rect_tag Delta_rect;

extern sig_atomic_t run_flag; // 程序運行標志

#include "client.h"
#include "color.h"
#include "config.h"
#include "desktop.h"
#include "drawable.h"
#include "entry.h"
#include "focus.h"
#include "font.h"
#include "func.h"
#include "grab.h"
#include "handler.h"
#include "hint.h"
#include "icon.h"
#include "init.h"
#include "layout.h"
#include "menu.h"
#include "misc.h"
#include "taskbar.h"

#endif
