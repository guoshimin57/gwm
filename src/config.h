/* *************************************************************************
 *     config.h：gwm的配置文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>

#define WM_KEY Mod4Mask // 窗口管理器的基本功能轉換鍵
#define WM_SKEY (WM_KEY|ShiftMask) // 與WM_KEY功能相關功能轉換鍵，通常表示相反
#define CMD_KEY (WM_KEY|Mod1Mask) // 與系統命令相關功能的轉換鍵
#define SYS_KEY (WM_KEY|ControlMask) // 與系統相關的功能轉換鍵

#define DEFAULT_CUR_DESKTOP 1 // 默認的當前桌面
#define DEFAULT_FOCUS_MODE CLICK_FOCUS // 默認的聚焦模式
#define DEFAULT_LAYOUT TILE // 默認的窗口布局模式
#define DEFAULT_AREA_TYPE MAIN_AREA // 新打開的窗口的默認區域類型
#define DEFAULT_MAIN_AREA_RATIO 0.6 // 默認的主區域比例
#define DEFAULT_FIXED_AREA_RATIO 0.15 // 默認的固定區域比例
#define DEFAULT_N_MAIN_MAX 1 // 默認的主區域最大窗口數量
#define AUTOSTART "~/.config/gwm/autostart.sh" // 在gwm剛啓動時執行的腳本

#define TO_STR(value) _TO_STR(value) // 把數字轉換爲字符串常量
#define _TO_STR(value) # value 

#define ROUND(value) ((int)((value)+0.5)) // 把浮點數四舍五入後轉換爲整數

#define DEFAULT_FONT_PIXEL_SIZE 24 // 默認字體大小，單位爲像素
#define TITLE_FONT_PIXEL_SIZE  DEFAULT_FONT_PIXEL_SIZE // 標題欄的字體大小，單位爲像素
#define CMD_CENTER_FONT_PIXEL_SIZE  DEFAULT_FONT_PIXEL_SIZE // 操作中心的字體大小，單位爲像素
#define TASKBAR_FONT_PIXEL_SIZE  DEFAULT_FONT_PIXEL_SIZE // 任務欄的字體大小，單位爲像素
#define ENTRY_FONT_PIXEL_SIZE  DEFAULT_FONT_PIXEL_SIZE // 輸入構件的字體大小，單位爲像素
#define RESIZE_WIN_FONT_PIXEL_SIZE  DEFAULT_FONT_PIXEL_SIZE // 調整尺寸的提示窗口的字體大小，單位爲像素
#define BORDER_WIDTH ROUND(DEFAULT_FONT_PIXEL_SIZE/8.0) // 窗口框架边框的宽度，单位为像素
#define TITLE_BAR_HEIGHT ROUND(TITLE_FONT_PIXEL_SIZE*4/3.0) // 窗口標題欄的高度，單位爲像素
#define TITLE_BUTTON_WIDTH TITLE_BAR_HEIGHT // 窗口按鈕的寬度，單位爲像素
#define TITLE_BUTTON_HEIGHT TITLE_BUTTON_WIDTH // 窗口按鈕的高度，單位爲像素
#define WIN_GAP BORDER_WIDTH // 窗口間隔，單位爲像素
#define STATUS_AREA_WIDTH_MAX TASKBAR_FONT_PIXEL_SIZE*30 // 任務欄狀態區域的最大寬度
#define TASKBAR_HEIGHT ROUND(TASKBAR_FONT_PIXEL_SIZE*4/3.0) // 狀態欄的高度，單位爲像素
#define TASKBAR_BUTTON_WIDTH TASKBAR_FONT_PIXEL_SIZE*2 // 任務欄按鈕的寬度，單位爲像素
#define TASKBAR_BUTTON_HEIGHT TASKBAR_HEIGHT // 任務欄按鈕的高度，單位爲像素
#define ICON_BORDER_WIDTH ROUND(TASKBAR_FONT_PIXEL_SIZE/32.0) // 縮微窗口边框的宽度，单位为像素
#define ICON_HEIGHT (TASKBAR_HEIGHT-ICON_BORDER_WIDTH*2) // 縮微化窗口的高度，單位爲像素
#define ICONS_SPACE ROUND(ICON_HEIGHT/2.0) // 縮微化窗口的間隔，單位爲像素
#define CMD_CENTER_ITEM_WIDTH (CMD_CENTER_FONT_PIXEL_SIZE*7) // 操作中心按鈕的寬度，單位爲像素
#define CMD_CENTER_ITEM_HEIGHT ROUND(CMD_CENTER_FONT_PIXEL_SIZE*1.5) // 操作中心按鈕的高度，單位爲像素
#define CMD_CENTER_COL 4 // 操作中心按鈕列數
#define ENTRY_TEXT_INDENT ROUND(ENTRY_FONT_PIXEL_SIZE/4.0)
#define RUN_CMD_ENTRY_WIDTH (ENTRY_FONT_PIXEL_SIZE*15+ENTRY_TEXT_INDENT*2) // 運行命令的輸入構件的寬度，單位爲像素
#define RUN_CMD_ENTRY_HEIGHT ROUND(ENTRY_FONT_PIXEL_SIZE*4/3.0) // 運行命令的輸入構件的寬度，單位爲像素
#define RESIZE_WIN_WIDTH (RESIZE_WIN_FONT_PIXEL_SIZE*10) // 調整尺寸的提示窗口的寬度，單位爲像素
#define RESIZE_WIN_HEIGHT ROUND(RESIZE_WIN_FONT_PIXEL_SIZE*4/3.0) // 調整尺寸的提示窗口的高度，單位爲像素
#define MOVE_RESIZE_INC 8 // 移動窗口、調整窗口尺寸的步進值，單位爲像素。僅當窗口未有效設置尺寸特性時才使用它。

#define RUN_CMD_ENTRY_HINT L"請輸入命令，然後按回車執行"
#define DEFAULT_FONT_NAME "monospace:pixelsize="TO_STR(DEFAULT_FONT_PIXEL_SIZE) // 默認字體名

#define FONT_NAME (const char *[]) /* 本窗口管理器所使用的字庫名稱列表 */                       \
{/* 注意：每增加一種不同的字體，就會增加2M左右的內存佔用 */                                     \
    [TITLE_AREA_FONT]       = "monospace:pixelsize="TO_STR(TITLE_FONT_PIXEL_SIZE),              \
    [TITLE_BUTTON_FONT]     = "monospace:pixelsize="TO_STR(TITLE_FONT_PIXEL_SIZE),              \
    [CMD_CENTER_FONT]       = "monospace:pixelsize="TO_STR(CMD_CENTER_FONT_PIXEL_SIZE),         \
    [TASKBAR_BUTTON_FONT]   = "monospace:pixelsize="TO_STR(TASKBAR_FONT_PIXEL_SIZE),            \
    [ICON_CLASS_FONT]       = "monospace:pixelsize="TO_STR(TASKBAR_FONT_PIXEL_SIZE),            \
    [ICON_TITLE_FONT]       = "monospace:pixelsize="TO_STR(TASKBAR_FONT_PIXEL_SIZE),            \
    [STATUS_AREA_FONT]      = "monospace:pixelsize="TO_STR(TASKBAR_FONT_PIXEL_SIZE),            \
    [ENTRY_FONT]            = "monospace:pixelsize="TO_STR(TASKBAR_FONT_PIXEL_SIZE),            \
    [RESIZE_WIN_FONT]       = "monospace:pixelsize="TO_STR(RESIZE_WIN_FONT_PIXEL_SIZE),         \
}

#define WIDGET_COLOR_NAME (const char *[]) /* 構件顏色名 */                        \
{                                                                                  \
    [NORMAL_BORDER_COLOR]         = "grey31",       /* 普通邊框的顏色名         */ \
    [CURRENT_BORDER_COLOR]        = "dodgerblue",   /* 當前邊框的顏色名         */ \
    [NORMAL_TITLE_AREA_COLOR]     = "grey31",       /* 普通標題區域的顏色名     */ \
    [CURRENT_TITLE_AREA_COLOR]    = "dodgerblue",   /* 當前標題區域的顏色名     */ \
    [NORMAL_TITLE_BUTTON_COLOR]   = "grey31",       /* 普通標題按鈕的顏色名     */ \
    [CURRENT_TITLE_BUTTON_COLOR]  = "dodgerblue",   /* 當前標題按鈕的顏色名     */ \
    [ENTERED_NORMAL_BUTTON_COLOR] = "darkorange",   /* 所進入的普通按鈕的顏色名 */ \
    [ENTERED_CLOSE_BUTTON_COLOR]  = "red",          /* 所進入的關閉按鈕的顏色名 */ \
    [NORMAL_TASKBAR_BUTTON_COLOR] = "grey21",       /* 普通任務欄按鈕的顏色名   */ \
    [CHOSEN_TASKBAR_BUTTON_COLOR] = "deepskyblue4", /* 選中的任務欄按鈕的顏色名 */ \
    [CMD_CENTER_COLOR]            = "grey21",       /* 操作中心的顏色名         */ \
    [ICON_COLOR]                  = "grey11",       /* 圖標的顏色名             */ \
    [ICON_AREA_COLOR]             = "grey11",       /* 圖標區域的顏色名         */ \
    [STATUS_AREA_COLOR]           = "grey21",       /* 狀態區域的顏色名         */ \
    [ENTRY_COLOR]                 = "white",        /* 單行文本輸入框的顏色名   */ \
    [RESIZE_WIN_COLOR]            = "grey21",        /* 調整尺寸提示窗口的顏色名 */ \
}

#define TEXT_COLOR_NAME (const char *[]) /* 文本顏色名 */                          \
{                                                                                  \
    [TITLE_AREA_TEXT_COLOR]      = "white",     /* 標題區域文本的顏色名       */   \
    [TITLE_BUTTON_TEXT_COLOR]    = "white",     /* 標題按鈕文本的顏色名       */   \
    [TASKBAR_BUTTON_TEXT_COLOR]  = "white",     /* 任務欄按鈕文本的顏色名     */   \
    [STATUS_AREA_TEXT_COLOR]     = "white",     /* 狀態區域文本的顏色名       */   \
    [ICON_CLASS_TEXT_COLOR]      = "rosybrown", /* 圖標中程序類型文本的顏色名 */   \
    [ICON_TITLE_TEXT_COLOR]      = "white",     /* 圖標中標題文本的顏色名     */   \
    [CMD_CENTER_ITEM_TEXT_COLOR] = "white",     /* 操作中心菜單項文本的顏色名 */   \
    [ENTRY_TEXT_COLOR]           = "black",     /* 輸入構件文本的顏色名 */         \
    [HINT_TEXT_COLOR]            = "grey61",    /* 用於提示的文本的顏色名 */       \
    [RESIZE_WIN_TEXT_COLOR]      = "white",     /* 調整尺寸提示窗口文本的顏色名 */ \
}

#define TITLE_BUTTON_TEXT (const char *[]) /* 窗口標題欄按鈕的標籤（從左至右）*/ \
/* 切換至主區域 切換至次區域 切換至固定區 切換至懸浮態 縮微化 最大化 關閉 */     \
{   "主",        "次",        "固",         "浮",  "-",   "□", "×" }

#define TASKBAR_BUTTON_TEXT (const char *[]) /* 任務欄按鈕的標籤（從左至右） */  \
{/* 依次爲各虛擬桌面標籤 */ \
    "1",   "2",   "3",   \
/* 切換至全屏模式 切換至概覽模式 切換至堆疊模式 切換至平鋪模式 切換桌面可見性 打開操作中心*/ \
    "□",           "▦",         "▣",          "▥",          "■",        "^",    \
}

#define CMD_CENTER_ITEM_TEXT (const char *[]) /* 操作中心按鈕的標籤（從左至右，從上至下） */  \
{\
    "幫助",         "文件",         "終端模擬器",   "網絡瀏覽器",   \
    "播放影音",     "切換播放狀態", "關閉影音",     "减小音量",     \
    "增大音量",     "最大音量",     "靜音切換",     "暫主區開窗",   \
    "暫次區開窗",   "暫固定區開窗", "暫懸浮區開窗", "暫縮微區開窗", \
    "增大主區容量", "减小主區容量", "切換聚焦模式", "退出gwm",      \
    "注銷",         "重啓",         "關機",         "運行",         \
}

#define CURSOR_SHAPE (unsigned int []) /* 定位器相關的光標字體 */  \
{                                                     \
    XC_left_ptr,            /* NO_OP */               \
    XC_fleur,               /* MOVE */                \
    XC_top_side,            /* TOP_RESIZE */          \
    XC_bottom_side,         /* BOTTOM_RESIZE */       \
    XC_left_side,           /* LEFT_RESIZE */         \
    XC_right_side,          /* RIGHT_RESIZE */        \
    XC_top_left_corner,     /* TOP_LEFT_RESIZE */     \
    XC_top_right_corner,    /* TOP_RIGHT_RESIZE */    \
    XC_bottom_left_corner,  /* BOTTOM_LEFT_RESIZE */  \
    XC_bottom_right_corner, /* BOTTOM_RIGHT_RESIZE */ \
    XC_sb_h_double_arrow,   /* ADJUST_LAYOUT_RATIO */ \
}

#define HELP "lxterminal -e 'man gwm' || xfce4-terminal -e 'man gwm' || xterm -e 'man gwm'"
#define FILE_MANAGER "xdg-open ~"
#define GAME "wesnoth || flatpak run org.wesnoth.Wesnoth"
#define BROWSER "xdg-open http:"
#define TERMINAL "lxterminal || xfce4-terminal || gnome-terminal || konsole5 || xterm"
#define TOGGLE_PROCESS_STATE(process) "{ pid=$(pgrep -f '"process"'); " \
    "ps -o stat $pid | tail -n +2 | grep T > /dev/null ; } " \
    "&& kill -CONT $pid || kill -STOP $pid > /dev/null 2>&1"
#define PLAY_START "mplayer -shuffle ~/music/*"
#define PLAY_TOGGLE TOGGLE_PROCESS_STATE(PLAY_START)
#define PLAY_QUIT "kill -KILL $(pgrep -f '"PLAY_START"')"
#define VOLUME_DOWN "amixer -q sset Master 10%-"
#define VOLUME_UP "amixer -q sset Master 10%+"
#define VOLUME_MAX "amixer -q sset Master 100%"
#define VOLUME_TOGGLE "amixer -q sset Master toggle"
#define LIGHT_DOWN "light -U 5"
#define LIGHT_UP "light -A 5"
#define LOGOUT "pkill -9 'startgwm|gwm'"

/* 與虛擬桌面相關的按鍵功能綁定。n=0僅作用於attach_to_all_desktops */
#define DESKTOP_KEYBIND(key, n)                                           \
/*  功能轉換鍵            鍵符號 要綁定的函數(詳見gwm.h) 函數的參數 */    \
    {WM_KEY|ShiftMask,      key, focus_desktop,          {.desktop_n=n}}, \
    {WM_KEY,	            key, move_to_desktop,        {.desktop_n=n}}, \
    {WM_KEY|Mod1Mask,	    key, all_move_to_desktop,    {.desktop_n=n}}, \
    {ControlMask,           key, change_to_desktop,      {.desktop_n=n}}, \
    {ControlMask|Mod1Mask,  key, all_change_to_desktop,  {.desktop_n=n}}, \
    {Mod1Mask,              key, attach_to_desktop,      {.desktop_n=n}}, \
    {Mod1Mask|ShiftMask,    key, all_attach_to_desktop,  {.desktop_n=n}}, \
    {ShiftMask|ControlMask, key, attach_to_all_desktops, {.desktop_n=n}}

#define KEYBIND (Keybind []) /* 按鍵功能綁定。有的鍵盤同按多鍵會衝突，故組合鍵宜盡量少，*/ \
{/* 功能轉換鍵  鍵符號           要綁定的函數(詳見gwm.h)      函數的參數 */                \
    {0, XF86XK_MonBrightnessDown,exec,                        SH_CMD(LIGHT_DOWN)},         \
    {0, XF86XK_MonBrightnessUp,  exec,                        SH_CMD(LIGHT_UP)},           \
    {0,	        XK_F1,           exec,                        SH_CMD(HELP)},               \
    {CMD_KEY,	XK_f,            exec,                        SH_CMD(FILE_MANAGER)},       \
    {CMD_KEY,	XK_g,            exec,                        SH_CMD(GAME)},               \
    {CMD_KEY,	XK_q,            exec,                        SH_CMD("qq")},               \
    {CMD_KEY,	XK_s,            exec,                        SH_CMD("stardict")},         \
    {CMD_KEY,	XK_t,            exec,                        SH_CMD(TERMINAL)},           \
    {CMD_KEY,	XK_w,            exec,                        SH_CMD(BROWSER)},            \
    {CMD_KEY, 	XK_F1,           exec,                        SH_CMD(PLAY_START)},         \
    {CMD_KEY, 	XK_F2,           exec,                        SH_CMD(PLAY_TOGGLE)},        \
    {CMD_KEY, 	XK_F3,           exec,                        SH_CMD(PLAY_QUIT)},          \
    {SYS_KEY,	XK_d,            enter_and_run_cmd,           {0}},                        \
    {SYS_KEY, 	XK_F1,           exec,                        SH_CMD(VOLUME_DOWN)},        \
    {SYS_KEY, 	XK_F2,           exec,                        SH_CMD(VOLUME_UP)},          \
    {SYS_KEY, 	XK_F3,           exec,                        SH_CMD(VOLUME_MAX)},         \
    {SYS_KEY, 	XK_F4,           exec,                        SH_CMD(VOLUME_TOGGLE)},      \
    {SYS_KEY, 	XK_l,            exec,                        SH_CMD(LOGOUT)},             \
    {SYS_KEY, 	XK_p,            exec,                        SH_CMD("poweroff")},         \
    {SYS_KEY, 	XK_r,            exec,                        SH_CMD("reboot")},           \
    {WM_KEY, 	XK_Delete,       quit_wm,                     {0}},                        \
    {WM_KEY, 	XK_k,            key_move_resize_client,      {.direction=UP}},            \
    {WM_KEY, 	XK_j,            key_move_resize_client,      {.direction=DOWN}},          \
    {WM_KEY, 	XK_h,            key_move_resize_client,      {.direction=LEFT}},          \
    {WM_KEY, 	XK_l,            key_move_resize_client,      {.direction=RIGHT}},         \
    {WM_KEY, 	XK_Up,           key_move_resize_client,      {.direction=UP2UP}},         \
    {WM_SKEY, 	XK_Up,           key_move_resize_client,      {.direction=UP2DOWN}},       \
    {WM_KEY, 	XK_Down,         key_move_resize_client,      {.direction=DOWN2DOWN}},     \
    {WM_SKEY, 	XK_Down,         key_move_resize_client,      {.direction=DOWN2UP}},       \
    {WM_KEY, 	XK_Left,         key_move_resize_client,      {.direction=LEFT2LEFT}},     \
    {WM_SKEY, 	XK_Left,         key_move_resize_client,      {.direction=LEFT2RIGHT}},    \
    {WM_KEY, 	XK_Right,        key_move_resize_client,      {.direction=RIGHT2RIGHT}},   \
    {WM_SKEY, 	XK_Right,        key_move_resize_client,      {.direction=RIGHT2LEFT}},    \
    {WM_KEY, 	XK_F1,           change_area,                 {.area_type=MAIN_AREA}},     \
    {WM_KEY, 	XK_F2,           change_area,                 {.area_type=SECOND_AREA}},   \
    {WM_KEY, 	XK_F3,           change_area,                 {.area_type=FIXED_AREA}},    \
    {WM_KEY, 	XK_F4,           change_area,                 {.area_type=FLOATING_AREA}}, \
    {WM_KEY, 	XK_F5,           change_area,                 {.area_type=ICONIFY_AREA}},  \
    {WM_SKEY, 	XK_F1,           change_default_area_type,    {.area_type=MAIN_AREA}},     \
    {WM_SKEY, 	XK_F2,           change_default_area_type,    {.area_type=SECOND_AREA}},   \
    {WM_SKEY, 	XK_F3,           change_default_area_type,    {.area_type=FIXED_AREA}},    \
    {WM_SKEY, 	XK_F4,           change_default_area_type,    {.area_type=FLOATING_AREA}}, \
    {WM_SKEY, 	XK_F5,           change_default_area_type,    {.area_type=ICONIFY_AREA}},  \
    {WM_KEY, 	XK_Return,       choose_client,               {0}},                        \
    {WM_KEY, 	XK_Tab,          next_client,                 {0}},                        \
    {WM_SKEY,	XK_Tab,          prev_client,                 {0}},                        \
    {WM_KEY, 	XK_b,            toggle_border_visibility,    {0}},                        \
    {WM_KEY, 	XK_c,            close_client,                {0}},                        \
    {WM_SKEY, 	XK_c,            close_all_clients,           {0}},                        \
    {WM_KEY, 	XK_d,            iconify_all_clients,         {0}},                        \
    {WM_SKEY,	XK_d,            deiconify_all_clients,       {0}},                        \
    {WM_KEY, 	XK_e,            toggle_focus_mode,           {0}},                        \
    {WM_KEY, 	XK_f,            change_layout,               {.layout=FULL}},             \
    {WM_KEY, 	XK_p,            change_layout,               {.layout=PREVIEW}},          \
    {WM_KEY, 	XK_s,            change_layout,               {.layout=STACK}},            \
    {WM_KEY, 	XK_t,            change_layout,               {.layout=TILE}},             \
    {WM_SKEY, 	XK_t,            toggle_title_bar_visibility, {0}},                        \
    {WM_KEY, 	XK_i,            adjust_n_main_max,           {.n=1}},                     \
    {WM_SKEY,	XK_i,            adjust_n_main_max,           {.n=-1}},                    \
    {WM_KEY, 	XK_m,            adjust_main_area_ratio,      {.change_ratio=0.01}},       \
    {WM_SKEY,	XK_m,            adjust_main_area_ratio,      {.change_ratio=-0.01}},      \
    {WM_KEY, 	XK_x,            adjust_fixed_area_ratio,     {.change_ratio=0.01}},       \
    {WM_SKEY,	XK_x,            adjust_fixed_area_ratio,     {.change_ratio=-0.01}},      \
    {WM_KEY,	XK_Page_Down,    next_desktop,                {0}},                        \
    {WM_KEY,	XK_Page_Up,      prev_desktop,                {0}},                        \
    DESKTOP_KEYBIND(XK_0, 0),                                                              \
    DESKTOP_KEYBIND(XK_1, 1), /* 注：我的鍵盤按super+左shift+1鍵時產生多鍵衝突 */          \
    DESKTOP_KEYBIND(XK_2, 2),                                                              \
    DESKTOP_KEYBIND(XK_3, 3),                                                              \
}

#define DESKTOPN_BUTTON(n) DESKTOP ## n ##_BUTTON /* 獲取虛擬桌面按鈕類型 */

#define DESKTOP_BUTTONBIND(n)                                                              \
/*  桌面按鈕類型                    功能轉換鍵  定位器按鈕 要綁定的函數       函數的參數 */\
    {DESKTOPN_BUTTON(n),                    0,    Button1, focus_desktop,          {0}},   \
    {DESKTOPN_BUTTON(n),          ControlMask,    Button1, change_to_desktop,      {0}},   \
    {DESKTOPN_BUTTON(n), Mod1Mask|ControlMask,    Button1, all_change_to_desktop,  {0}},   \
    {DESKTOPN_BUTTON(n),                    0,    Button2, attach_to_desktop,      {0}},   \
    {DESKTOPN_BUTTON(n),             Mod1Mask,    Button2, all_attach_to_desktop,  {0}},   \
    {DESKTOPN_BUTTON(n),            ShiftMask,    Button2, attach_to_all_desktops, {0}},   \
    {DESKTOPN_BUTTON(n),                    0,    Button3, move_to_desktop,        {0}},   \
    {DESKTOPN_BUTTON(n),             Mod1Mask,    Button3, all_move_to_desktop,    {0}}

#define BUTTONBIND (Buttonbind []) /* 按鈕功能綁定 */                                               \
{/* 控件類型             能轉換鍵 定位器按鈕 要綁定的函數              函數的參數 */                \
    {FULL_BUTTON,              0, Button1, change_layout,              {.layout=FULL}},             \
    {PREVIEW_BUTTON,           0, Button1, change_layout,              {.layout=PREVIEW}},          \
    {STACK_BUTTON,             0, Button1, change_layout,              {.layout=STACK}},            \
    {TILE_BUTTON,              0, Button1, change_layout,              {.layout=TILE}},             \
    {DESKTOP_BUTTON,           0, Button1, iconify_all_clients,        {0}},                        \
    {TITLE_AREA,               0, Button1, pointer_move_resize_client, {.resize=false}},            \
    {MAIN_BUTTON,              0, Button1, change_area,                {.area_type=MAIN_AREA}},     \
    {SECOND_BUTTON,            0, Button1, change_area,                {.area_type=SECOND_AREA}},   \
    {FIXED_BUTTON,             0, Button1, change_area,                {.area_type=FIXED_AREA}},    \
    {FLOAT_BUTTON,             0, Button1, change_area,                {.area_type=FLOATING_AREA}}, \
    {ICON_BUTTON,              0, Button1, change_area,                {.area_type=ICONIFY_AREA}},  \
    {MAX_BUTTON,               0, Button1, maximize_client,            {0}},                        \
    {CLOSE_BUTTON,             0, Button1, close_client,               {0}},                        \
    {CLIENT_WIN,               0, Button1, choose_client,              {0}},                        \
    {CLIENT_WIN,          WM_KEY, Button1, pointer_move_resize_client, {.resize=false}},            \
    {CLIENT_WIN,         WM_SKEY, Button1, pointer_move_resize_client, {.resize=true}},             \
    {CLIENT_FRAME,             0, Button1, pointer_move_resize_client, {.resize=true}},             \
    {CLIENT_ICON,              0, Button1, change_area,                {.area_type=PREV_AREA}},     \
    {ROOT_WIN,                 0, Button1, adjust_layout_ratio,        {0}},                        \
    {DESKTOP_BUTTON,           0, Button2, close_all_clients,          {0}},                        \
    {TITLE_AREA,               0, Button2, pointer_change_area,        {0}},                        \
    {CLIENT_WIN,          WM_KEY, Button2, pointer_change_area,        {0}},                        \
    {CLIENT_ICON,              0, Button2, close_client,               {0}},                        \
    {DESKTOP_BUTTON,           0, Button3, deiconify_all_clients,      {0}},                        \
    {TITLE_AREA,               0, Button3, pointer_swap_clients,       {0}},                        \
    {CLIENT_WIN,               0, Button3, NULL,                       {0}},                        \
    {CLIENT_WIN,          WM_KEY, Button3, pointer_swap_clients,       {0}},                        \
    {CMD_CENTER_ITEM,          0, Button1, open_cmd_center,            {0}},                        \
    {HELP_BUTTON,              0, Button1, exec,                       SH_CMD(HELP)},               \
    {FILE_BUTTON,              0, Button1, exec,                       SH_CMD(FILE_MANAGER)},       \
    {TERM_BUTTON,              0, Button1, exec,                       SH_CMD(TERMINAL)},           \
    {BROWSER_BUTTON,           0, Button1, exec,                       SH_CMD(BROWSER)},            \
    {PLAY_START_BUTTON,        0, Button1, exec,                       SH_CMD(PLAY_START)},         \
    {PLAY_TOGGLE_BUTTON,       0, Button1, exec,                       SH_CMD(PLAY_TOGGLE)},        \
    {PLAY_QUIT_BUTTON,         0, Button1, exec,                       SH_CMD(PLAY_QUIT)},          \
    {VOLUME_DOWN_BUTTON,       0, Button1, exec,                       SH_CMD(VOLUME_DOWN)},        \
    {VOLUME_UP_BUTTON,         0, Button1, exec,                       SH_CMD(VOLUME_UP)},          \
    {VOLUME_MAX_BUTTON,        0, Button1, exec,                       SH_CMD(VOLUME_MAX)},         \
    {VOLUME_TOGGLE_BUTTON,     0, Button1, exec,                       SH_CMD(VOLUME_TOGGLE)},      \
    {MAIN_NEW_BUTTON,          0, Button1, change_default_area_type,   {.area_type=MAIN_AREA}},     \
    {SEC_NEW_BUTTON,           0, Button1, change_default_area_type,   {.area_type=SECOND_AREA}},   \
    {FIX_NEW_BUTTON,           0, Button1, change_default_area_type,   {.area_type=FIXED_AREA}},    \
    {FLOAT_NEW_BUTTON,         0, Button1, change_default_area_type,   {.area_type=FLOATING_AREA}}, \
    {ICON_NEW_BUTTON,          0, Button1, change_default_area_type,   {.area_type=ICONIFY_AREA}},  \
    {N_MAIN_UP_BUTTON,         0, Button1, adjust_n_main_max,          {.n=1}},                     \
    {N_MAIN_DOWN_BUTTON,       0, Button1, adjust_n_main_max,          {.n=-1}},                    \
    {FOCUS_MODE_BUTTON,        0, Button1, toggle_focus_mode,          {0}},                        \
    {QUIT_WM_BUTTON,           0, Button1, quit_wm,                    {0}},                        \
    {LOGOUT_BUTTON,            0, Button1, exec,                       SH_CMD(LOGOUT)},             \
    {REBOOT_BUTTON,            0, Button1, exec,                       SH_CMD("reboot")},           \
    {POWEROFF_BUTTON,          0, Button1, exec,                       SH_CMD("poweroff")},         \
    {RUN_BUTTON,               0, Button1, enter_and_run_cmd,          {0}},                        \
    DESKTOP_BUTTONBIND(1),                                                                          \
    DESKTOP_BUTTONBIND(2),                                                                          \
    DESKTOP_BUTTONBIND(3),                                                                          \
}

#define RULE (Rule []) /* 窗口管理器對窗口的管理規則 */                                                           \
{/* 可通過xprop命令查看客戶程序類型和客戶程序名稱。其結果表示爲：                                                 \
        WM_CLASS(STRING) = "客戶程序名稱", "客戶程序類型"                                                         \
    客戶程序類型           客戶程序名稱 客戶程序的類型別名 窗口放置位置       標題欄高度        邊框寬度    桌面*/\
    {"Qq",                 "qq",                 "QQ",     FIXED_AREA,        0,                0,            0}, \
    {"explorer.exe",       "explorer.exe",       NULL,     FLOATING_AREA,     0,                0,            0}, \
    {"Thunder.exe",        "Thunder.exe",        NULL,     FLOATING_AREA,     TITLE_BAR_HEIGHT, BORDER_WIDTH, 0}, \
    {"Google-chrome",      "google-chrome",      "chrome", DEFAULT_AREA_TYPE, TITLE_BAR_HEIGHT, BORDER_WIDTH, 0}, \
    {"Org.gnome.Nautilus", "org.gnome.Nautilus", "文件",   DEFAULT_AREA_TYPE, TITLE_BAR_HEIGHT, BORDER_WIDTH, 0}, \
}

#endif
