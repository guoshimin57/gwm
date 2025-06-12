/* *************************************************************************
 *     gwm.h：定義全局性的類型、宏。
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

#include <X11/Xlib.h>

#define TITLE_BUTTON_N (TITLE_BUTTON_END-TITLE_BUTTON_BEGIN+1)
#define TASKBAR_BUTTON_N (TASKBAR_BUTTON_END-TASKBAR_BUTTON_BEGIN+1)
#define ACT_CENTER_ITEM_N (ACT_CENTER_ITEM_END-ACT_CENTER_ITEM_BEGIN+1)
#define CLIENT_MENU_ITEM_N (CLIENT_MENU_ITEM_END-CLIENT_MENU_ITEM_BEGIN+1)
#define DESKTOP_N (DESKTOP_BUTTON_END-DESKTOP_BUTTON_BEGIN+1)
#define BUTTON_MASK (ButtonPressMask|ButtonReleaseMask)
#define POINTER_MASK (BUTTON_MASK|ButtonMotionMask)
#define CROSSING_MASK (EnterWindowMask|LeaveWindowMask)
#define ROOT_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    PropertyChangeMask|ButtonPressMask|CROSSING_MASK|ExposureMask|KeyPressMask)

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
    NORMAL_LAYER, BELOW_LAYER, DESKTOP_LAYER,
    ANY_LAYER
} Layer;

#define LAYER_N ANY_LAYER

typedef enum // 平鋪層的分區類型
{
    MAIN_AREA, SECOND_AREA, FIXED_AREA,
    ANY_AREA
} Area;

typedef enum // 構件標識
{
    ROOT_WIN, HINT_WIN, RUN_CMD_ENTRY, COLOR_ENTRY,

    CLIENT_FRAME, CLIENT_WIN, TITLE_LOGO, TITLEBAR,

    SECOND_BUTTON, MAIN_BUTTON, FIXED_BUTTON,
    ICON_BUTTON, MAX_BUTTON, CLOSE_BUTTON,

    CLIENT_MENU,
    SHADE_BUTTON, VERT_MAX_BUTTON, HORZ_MAX_BUTTON, TOP_MAX_BUTTON,
    BOTTOM_MAX_BUTTON, LEFT_MAX_BUTTON, RIGHT_MAX_BUTTON, FULL_MAX_BUTTON,
    FULLSCREEN_BUTTON, ABOVE_BUTTON, BELOW_BUTTON,

    TASKBAR, ICONBAR, CLIENT_ICON, STATUSBAR,
    DESKTOP0_BUTTON, DESKTOP1_BUTTON, DESKTOP2_BUTTON, 
    STACK_BUTTON, TILE_BUTTON, DESKTOP_BUTTON,
    ACT_CENTER_ITEM,

    ACT_CENTER, 
    HELP_BUTTON, FILE_BUTTON, TERM_BUTTON, BROWSER_BUTTON, 
    GAME_BUTTON, PLAY_START_BUTTON, PLAY_TOGGLE_BUTTON, PLAY_QUIT_BUTTON,
    VOLUME_DOWN_BUTTON, VOLUME_UP_BUTTON, VOLUME_MAX_BUTTON, VOLUME_TOGGLE_BUTTON,
    N_MAIN_UP_BUTTON, N_MAIN_DOWN_BUTTON,
    CLOSE_ALL_CLIENTS_BUTTON, PRINT_WIN_BUTTON, PRINT_SCREEN_BUTTON, FOCUS_MODE_BUTTON,
    COMPOSITOR_BUTTON, WALLPAPER_BUTTON, COLOR_BUTTON, QUIT_WM_BUTTON, LOGOUT_BUTTON,
    REBOOT_BUTTON, POWEROFF_BUTTON, RUN_BUTTON,

    UNUSED_WIDGET_ID,

    WIDGET_N=UNUSED_WIDGET_ID,
    TITLE_BUTTON_BEGIN=SECOND_BUTTON, TITLE_BUTTON_END=CLOSE_BUTTON,
    TASKBAR_BUTTON_BEGIN=DESKTOP0_BUTTON, TASKBAR_BUTTON_END=ACT_CENTER_ITEM,
    LAYOUT_BUTTON_BEGIN=STACK_BUTTON, LAYOUT_BUTTON_END=TILE_BUTTON, 
    DESKTOP_BUTTON_BEGIN=DESKTOP0_BUTTON, DESKTOP_BUTTON_END=DESKTOP2_BUTTON,
    ACT_CENTER_ITEM_BEGIN=HELP_BUTTON, ACT_CENTER_ITEM_END=RUN_BUTTON,
    CLIENT_MENU_ITEM_BEGIN=SHADE_BUTTON, CLIENT_MENU_ITEM_END=BELOW_BUTTON, 
} Widget_id;

typedef enum // 定位器操作類型
{
    NO_OP, CHOOSE, MOVE, SWAP, CHANGE, TOP_RESIZE, BOTTOM_RESIZE, LEFT_RESIZE,
    RIGHT_RESIZE, TOP_LEFT_RESIZE, TOP_RIGHT_RESIZE, BOTTOM_LEFT_RESIZE,
    BOTTOM_RIGHT_RESIZE, LAYOUT_RESIZE, POINTER_ACT_N
} Pointer_act;

typedef struct // 窗口管理器的規則
{// 分別爲客戶窗口的程序類型和程序名稱、標題，NULL或*表示匹配任何字符串
    const char *app_class, *app_name, *title;
    const char *class_alias; // 客戶窗口的類型別名
    Layer layer; // 客戶窗口的層
    Area area; // 客戶窗口的區
    unsigned int desktop_mask; // 客戶窗口所属虚拟桌面掩碼
} Rule;

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

#endif
