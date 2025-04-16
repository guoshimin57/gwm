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
#include <X11/Xlib.h>

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

typedef enum // 窗口放置的層類型
{
    FULLSCREEN_LAYER, DOCK_LAYER, ABOVE_LAYER,
    STACK_LAYER, TILE_LAYER, // 兩者合稱NORMAL_LAYER
    BELOW_LAYER, DESKTOP_LAYER,
    ANY_LAYER
} Layer;

#define LAYER_N ANY_LAYER

typedef enum // 平鋪層的分區類型
{
    MAIN_AREA, SECOND_AREA, FIXED_AREA,
    ANY_AREA
} Area;

typedef struct // 窗口管理器的規則
{// 分別爲客戶窗口的程序類型和程序名稱、標題，NULL或*表示匹配任何字符串
    const char *app_class, *app_name, *title;
    const char *class_alias; // 客戶窗口的類型別名
    Layer layer; // 客戶窗口的層
    Area area; // 客戶窗口的區
    unsigned int desktop_mask; // 客戶窗口所属虚拟桌面掩碼
} Rule;

typedef void (*Event_handler)(XEvent *); // 事件處理器類型

extern sig_atomic_t run_flag; // 程序運行標志
extern Event_handler event_handler; // 事件處理器
extern Xinfo xinfo;

#endif
