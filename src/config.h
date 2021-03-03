/* *************************************************************************
 *     config.h：gwm的配置文件。
 *     版權 (C) 2021 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#define WM_KEY Mod4Mask /* 窗口管理器的基本功能轉換鍵 */
#define WM_SKEY (Mod4Mask|ShiftMask) /* 與WM_KEY相反的功能轉換鍵 */
#define CMD_KEY (Mod4Mask|Mod1Mask) /* 與命令行相關的功能轉換鍵 */
#define MOVE_RESIZE_INC 32 /* 調整窗口的步進值，單位爲像素 */
#define STATUS_BAR_HEIGHT 32 /* 狀態欄高度，單位爲像素 */
#define DEFAULT_LAYOUT TILE /* 默認的窗口布局模式 */
#define DEFAULT_MAIN_AREA_RATIO 0.6 /* 默認的主區域比例 */
#define DEFAULT_FIXED_AREA_RATIO 0.15 /* 默認的固定區域比例 */
#define DEFAULT_N_MAIN_MAX 1 /* 默認的主區域最大窗口數量 */
#define STATUS_BAR_BACKGROUND_COLOR GREY21 /*狀態欄背景色 */
#define STATUS_BAR_FOREGROUND_COLOR 0xffffff /*狀態欄前景色 */

KEYBINDS keybinds_list[]= /* 鍵盤快捷鍵綁定 */
{ /* 功能轉換鍵     鍵符號          要綁定的函數                函數的參數 */
    {CMD_KEY,    XK_t,            exec,                       SH_CMD("lxterminal")},
    {CMD_KEY,    XK_f,            exec,                       SH_CMD("xdg-open ~")},
    {CMD_KEY,    XK_w,            exec,                       SH_CMD("xwininfo -wm >log")},
    {CMD_KEY,    XK_p,            exec,                       SH_CMD("dmenu_run")},
    {CMD_KEY,    XK_q,            exec,                       SH_CMD("qq")},
    {CMD_KEY,    XK_s,            exec,                       SH_CMD("stardict")},
    {WM_KEY,     XK_Up,           key_move_resize_client,     {.direction=UP}},
    {WM_KEY,     XK_Down,         key_move_resize_client,     {.direction=DOWN}},
    {WM_KEY,     XK_Left,         key_move_resize_client,     {.direction=LEFT}},
    {WM_KEY,     XK_Right,        key_move_resize_client,     {.direction=RIGHT}},
    {WM_KEY,     XK_bracketleft,  key_move_resize_client,     {.direction=UP2UP}},
    {WM_KEY,     XK_bracketright, key_move_resize_client,     {.direction=UP2DOWN}},
    {WM_KEY,     XK_semicolon,    key_move_resize_client,     {.direction=DOWN2UP}},
    {WM_KEY,     XK_quoteright,   key_move_resize_client,     {.direction=DOWN2DOWN}},
    {WM_KEY,     XK_9,            key_move_resize_client,     {.direction=LEFT2LEFT}},
    {WM_KEY,     XK_0,            key_move_resize_client,     {.direction=LEFT2RIGHT}},
    {WM_KEY,     XK_minus,        key_move_resize_client,     {.direction=RIGHT2LEFT}},
    {WM_KEY,     XK_equal,        key_move_resize_client,     {.direction=RIGHT2RIGHT}},
    {WM_KEY,     XK_Delete,       quit_wm,                    {0}},
    {WM_KEY,     XK_c,            close_win,                  {0}},
    {WM_KEY,     XK_Tab,          next_win,                   {0}},
    {WM_KEY,     XK_f,            change_layout,              {.layout=FULL}},
    {WM_KEY,     XK_p,            change_layout,              {.layout=PREVIEW}},
    {WM_KEY,     XK_s,            change_layout,              {.layout=STACK}},
    {WM_KEY,     XK_t,            change_layout,              {.layout=TILE}},
    {WM_KEY,     XK_i,            adjust_n_main_max,          {.n=1}},
    {WM_SKEY,    XK_i,            adjust_n_main_max,          {.n=-1}},
    {WM_KEY,     XK_m,            adjust_main_area_ratio,     {.change_ratio=0.01}},
    {WM_SKEY,    XK_m,            adjust_main_area_ratio,     {.change_ratio=-0.01}},
    {WM_KEY,     XK_x,            adjust_fixed_area_ratio,    {.change_ratio=0.01}},
    {WM_SKEY,    XK_x,            adjust_fixed_area_ratio,    {.change_ratio=-0.01}},
    {WM_KEY,     XK_F1,           key_change_area,            {.area_type=MAIN_AREA}},
    {WM_KEY,     XK_F2,           key_change_area,            {.area_type=SECOND_AREA}},
    {WM_KEY,     XK_F3,           key_change_area,            {.area_type=FIXED_AREA}},
    {WM_KEY,     XK_F4,           key_change_area,            {.area_type=FLOATING_AREA}},
};

BUTTONBINDS buttonbinds_list[]= /* 定位器按鈕綁定 */
{ /* 功能轉換鍵 定位器按鈕  要綁定的函數            函數的參數 */
    {0,         Button1, pointer_focus_client,      {0}},
    {0,         Button3, pointer_focus_client,      {0}},
    {WM_KEY,    Button1, pointer_move_resize_client,{.resize_flag=false}},
    {WM_KEY,    Button3, pointer_move_resize_client,{.resize_flag=true}},
    {WM_SKEY,   Button1, pointer_change_area,       {0}},
};

WM_RULE rules[]= /* 窗口管理器對窗口的管理規則 */
{ /* 可通過xprop命令查看客戶程序名稱和客戶程序實例名稱。
     其結果表示爲：
        WM_CLASS(STRING) = "客戶程序實例名稱", "客戶程序名稱"
    客戶程序名稱   客戶程序實例名稱    窗口放置位置 */ 
    {"Stardict",    "stardict",         FLOATING},
    {"Qq",          "qq",               FIXED},
    {"Peek",        "peek",             FLOATING},
};

#endif
