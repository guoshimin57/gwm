/* *************************************************************************
 *     gwm.h：與gwm.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
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
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xproto.h>
#include "gwm.h"
#include "misc.h"
#include "taskbar.h"

#define NET_WM_STATE_REMOVE 0
#define NET_WM_STATE_ADD    1
#define NET_WM_STATE_TOGGLE 2

#define SHOULD_ADD_STATE(c, act, flag) \
    (act==NET_WM_STATE_ADD || (act==NET_WM_STATE_TOGGLE && !c->win_state.flag))

#define SH_CMD(cmd_str) {.cmd=(char *const []){"/bin/sh", "-c", cmd_str, NULL}}
#define FUNC_ARG(var, data) (Func_arg){.var=data}

#define BUTTON_MASK (ButtonPressMask|ButtonReleaseMask)
#define POINTER_MASK (BUTTON_MASK|ButtonMotionMask)
#define CROSSING_MASK (EnterWindowMask|LeaveWindowMask)
#define ROOT_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    PropertyChangeMask|ButtonPressMask|CROSSING_MASK|ExposureMask|KeyPressMask)

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
} Xinfo;

typedef enum // 窗口的位置類型，屏幕分爲多層，普通層還分爲多個區域
{
    FULLSCREEN_LAYER, DOCK_LAYER, ABOVE_LAYER, NORMAL_LAYER,
    MAIN_AREA, SECOND_AREA, FIXED_AREA, // 普通層的各個區域
    BELOW_LAYER, DESKTOP_LAYER,
    ANY_PLACE
} Place;

#define LAYER_N 6

typedef struct rectangle_tag // 矩形窗口或區域的坐標和尺寸
{
    int x, y; // 坐標
    int w, h; // 尺寸
} Rect;

typedef struct /* 調整窗口尺寸的信息 */
{
    int dx, dy, dw, dh; /* 分別爲窗口坐標和尺寸的變化量 */
} Delta_rect;

typedef enum // 窗口管理器的布局模式
{
    STACK, TILE,
} Layout;

struct rule_tag // 窗口管理器的規則
{// 分別爲客戶窗口的程序類型和程序名稱、標題，NULL或*表示匹配任何字符串
    const char *app_class, *app_name, *title;
    const char *class_alias; // 客戶窗口的類型別名
    Place place; // 客戶窗口的位置類型
    unsigned int desktop_mask; // 客戶窗口所属虚拟桌面掩碼
};
typedef struct rule_tag Rule;

typedef void (*Event_handler)(XEvent *); // 事件處理器類型

struct move_info_tag /* 定位器所點擊的窗口位置每次合理移動或調整尺寸所對應的舊、新坐標信息 */
{
    int ox, oy, nx, ny; /* 分別爲舊、新坐標 */
};
typedef struct move_info_tag Move_info;

extern sig_atomic_t run_flag; // 程序運行標志
extern Event_handler event_handler; // 事件處理器
extern Xinfo xinfo;

#endif
