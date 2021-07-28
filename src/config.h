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

#define DEFAULT_LAYOUT TILE /* 默認的窗口布局模式 */
#define DEFAULT_MAIN_AREA_RATIO 0.6 /* 默認的主區域比例 */
#define DEFAULT_FIXED_AREA_RATIO 0.15 /* 默認的固定區域比例 */
#define DEFAULT_N_MAIN_MAX 1 /* 默認的主區域最大窗口數量 */
#define AUTOSTART "~/.config/gwm/autostart.sh" /* 在gwm剛啓動時執行的腳本 */

#define GREY11 0x1c1c1c         /* 灰色的一種 */
#define GREY21 0x363636         /* 灰色的一種 */
#define CORNFLOWERBLUE 0x6495ed /* 矢車菊藍 */
#define DODGERBLUE 0x1e90ff     /* 道奇藍 */
#define RED 0xff0000            /* 紅色 */
#define WHITE 0xffffff          /* 白色 */
#define BLACK 0x000000          /* 黑色 */

#define NORMAL_FRAME_COLOR CORNFLOWERBLUE /* 非當前窗口的框架顏色 */
#define CURRENT_FRAME_COLOR DODGERBLUE /* 當前窗口的框架顏色 */
#define NORMAL_BORDER_COLOR CORNFLOWERBLUE /* 非當前窗口的边框顏色 */
#define CURRENT_BORDER_COLOR DODGERBLUE /* 當前窗口的边框顏色 */
#define NORMAL_TITLE_AREA_COLOR CORNFLOWERBLUE /* 非當前窗口的標題區域顏色 */
#define CURRENT_TITLE_AREA_COLOR DODGERBLUE /* 當前窗口的標題區域顏色 */
#define NORMAL_BUTTON_COLOR CORNFLOWERBLUE /* 非當前窗口的按鈕顏色 */
#define CURRENT_BUTTON_COLOR DODGERBLUE /* 當前窗口的按鈕顏色 */
#define ENTERED_NORMAL_BUTTON_COLOR (~DODGERBLUE) /* 定位器進入非當前窗口按鈕時按鈕的顏色 */
#define ENTERED_CLOSE_BUTTON_COLOR RED /* 定位器進入窗口關閉按鈕時按鈕的顏色 */
#define TITLE_TEXT_COLOR WHITE /* 窗口標題欄文字的顏色 */
#define BUTTON_TEXT_COLOR WHITE /* 窗口按鈕文字的顏色 */

#define TASKBAR_COLOR GREY21 /* 任務欄的顏色 */
#define STATUS_AREA_COLOR GREY21 /*狀態區域背景色 */
#define STATUS_AREA_TEXT_COLOR WHITE /*狀態區域前景色 */
#define TASKBAR_BUTTON_COLOR GREY11 /* 任務欄按鈕的顏色 */
#define ENTERED_TASKBAR_BUTTON_COLOR (~DODGERBLUE) /* 定位器進入任務欄按鈕時按鈕的顏色 */
#define TASKBAR_BUTTON_TEXT_COLOR WHITE  /* 任務欄按鈕文字的顏色 */

#define FONT_SET "*-24-*"
#define MOVE_RESIZE_INC 32 /* 調整窗口的步進值，單位爲像素 */
#define TITLE_BAR_HEIGHT 32 /* 窗口標題欄高度，單位爲像素 */
#define BORDER_WIDTH 4 /* 窗口边框宽度，单位为像素 */
#define TASKBAR_HEIGHT 32 /* 狀態欄高度，單位爲像素 */
#define TASKBAR_BUTTON_WIDTH 32 /* 任務欄按鈕的寬度，單位爲像素 */
#define TASKBAR_BUTTON_HEIGHT 32 /* 任務欄按鈕的高度，單位爲像素 */
#define CLIENT_BUTTON_WIDTH 28 /* 窗口按鈕的寬度，單位爲像素 */
#define CLIENT_BUTTON_HEIGHT 28 /* 窗口按鈕的高度，單位爲像素 */
#define WINS_SPACE 4 /* 窗口間隔，單位爲像素 */

#define CLIENT_BUTTON_TEXT (const char *[])  /* 客戶窗口的按鈕標籤 */ \
{"主", "次", "固", "浮", "-", "□", "×"}
#define TASKBAR_BUTTON_TEXT (const char *[]) /* 任務欄按鈕標籤 */ \
{"全", "概", "堆", "平", "主"}

#define KEYBINDS (KEYBIND []) /* 按鍵功能綁定 */                                      \
{/* 功能轉換鍵   鍵符號          要綁定的函數             函數的參數 */               \
    {CMD_KEY, XK_t,            exec,                    SH_CMD("lxterminal")},        \
    {CMD_KEY, XK_f,            exec,                    SH_CMD("xdg-open ~")},        \
    {CMD_KEY, XK_w,            exec,                    SH_CMD("xwininfo -wm >log")}, \
    {CMD_KEY, XK_p,            exec,                    SH_CMD("dmenu_run")},         \
    {CMD_KEY, XK_q,            exec,                    SH_CMD("qq")},                \
    {CMD_KEY, XK_s,            exec,                    SH_CMD("stardict")},          \
    {WM_KEY,  XK_Up,           key_move_resize_client,  {.direction=UP}},             \
    {WM_KEY,  XK_Down,         key_move_resize_client,  {.direction=DOWN}},           \
    {WM_KEY,  XK_Left,         key_move_resize_client,  {.direction=LEFT}},           \
    {WM_KEY,  XK_Right,        key_move_resize_client,  {.direction=RIGHT}},          \
    {WM_KEY,  XK_bracketleft,  key_move_resize_client,  {.direction=UP2UP}},          \
    {WM_KEY,  XK_bracketright, key_move_resize_client,  {.direction=UP2DOWN}},        \
    {WM_KEY,  XK_semicolon,    key_move_resize_client,  {.direction=DOWN2UP}},        \
    {WM_KEY,  XK_quoteright,   key_move_resize_client,  {.direction=DOWN2DOWN}},      \
    {WM_KEY,  XK_9,            key_move_resize_client,  {.direction=LEFT2LEFT}},      \
    {WM_KEY,  XK_0,            key_move_resize_client,  {.direction=LEFT2RIGHT}},     \
    {WM_KEY,  XK_minus,        key_move_resize_client,  {.direction=RIGHT2LEFT}},     \
    {WM_KEY,  XK_equal,        key_move_resize_client,  {.direction=RIGHT2RIGHT}},    \
    {WM_KEY,  XK_Delete,       quit_wm,                 {0}},                         \
    {WM_KEY,  XK_c,            close_win,               {0}},                         \
    {WM_KEY,  XK_Tab,          next_client,             {0}},                         \
    {WM_SKEY, XK_Tab,          prev_client,             {0}},                         \
    {WM_KEY,  XK_f,            change_layout,           {.layout=FULL}},              \
    {WM_KEY,  XK_p,            change_layout,           {.layout=PREVIEW}},           \
    {WM_KEY,  XK_s,            change_layout,           {.layout=STACK}},             \
    {WM_KEY,  XK_t,            change_layout,           {.layout=TILE}},              \
    {WM_KEY,  XK_i,            adjust_n_main_max,       {.n=1}},                      \
    {WM_SKEY, XK_i,            adjust_n_main_max,       {.n=-1}},                     \
    {WM_KEY,  XK_m,            adjust_main_area_ratio,  {.change_ratio=0.01}},        \
    {WM_SKEY, XK_m,            adjust_main_area_ratio,  {.change_ratio=-0.01}},       \
    {WM_KEY,  XK_x,            adjust_fixed_area_ratio, {.change_ratio=0.01}},        \
    {WM_SKEY, XK_x,            adjust_fixed_area_ratio, {.change_ratio=-0.01}},       \
    {WM_KEY,  XK_F1,           change_area,             {.area_type=MAIN_AREA}},      \
    {WM_KEY,  XK_F2,           change_area,             {.area_type=SECOND_AREA}},    \
    {WM_KEY,  XK_F3,           change_area,             {.area_type=FIXED_AREA}},     \
    {WM_KEY,  XK_F4,           change_area,             {.area_type=FLOATING_AREA}},  \
    {WM_KEY,  XK_F5,           change_area,             {.area_type=ICONIFY_AREA}},   \
    {WM_KEY,  XK_Return,       deiconify,               {0}},                         \
}

#define BUTTONBINDS (BUTTONBIND []) /* 按鈕功能綁定 */                                         \
{ /* 點擊類型   功能轉換鍵  定位器按鈕  要綁定的函數                函數的參數 */              \
    {TO_FULL,       0,       Button1, change_layout,              {.layout=FULL}},             \
    {TO_PREVIEW,    0,       Button1, change_layout,              {.layout=PREVIEW}},          \
    {TO_STACK,      0,       Button1, change_layout,              {.layout=STACK}},            \
    {TO_TILE,       0,       Button1, change_layout,              {.layout=TILE}},             \
    {ADJUST_N_MAIN, 0,       Button1, adjust_n_main_max,          {.n=1}},                     \
    {ADJUST_N_MAIN, 0,       Button3, adjust_n_main_max,          {.n=-1}},                    \
    {CLICK_TITLE,   0,       Button1, pointer_move_client,        {0}},                        \
    {CLICK_TITLE,   0,       Button3, pointer_change_area,        {0}},                        \
    {TO_MAIN,       0,       Button1, change_area,                {.area_type=MAIN_AREA}},     \
    {TO_SECOND,     0,       Button1, change_area,                {.area_type=SECOND_AREA}},   \
    {TO_FIX,        0,       Button1, change_area,                {.area_type=FIXED_AREA}},    \
    {TO_FLOAT,      0,       Button1, change_area,                {.area_type=FLOATING_AREA}}, \
    {ICON_WIN,      0,       Button1, change_area,                {.area_type=ICONIFY_AREA}},  \
    {MAX_WIN,       0,       Button1, maximize_client,            {0}},                        \
    {CLOSE_WIN,     0,       Button1, close_win,                  {0}},                        \
    {CLICK_WIN,     0,       Button1, pointer_focus_client,       {0}},                        \
    {CLICK_WIN,     0,       Button3, pointer_focus_client,       {0}},                        \
    {CLICK_FRAME,   0,       Button1, pointer_resize_client,      {0}},                        \
    {CLICK_ICON,    0,       Button1, deiconify,                  {0}},                        \
    {CLICK_ROOT,    0,       Button1, adjust_layout_ratio,        {0}},                        \
}

#define RULES (WM_RULE []) /* 窗口管理器對窗口的管理規則 */     \
{ /* 可通過xprop命令查看客戶程序名稱和客戶程序實例名稱。        \
     其結果表示爲：                                             \
        WM_CLASS(STRING) = "客戶程序實例名稱", "客戶程序名稱"   \
    客戶程序名稱   客戶程序實例名稱    窗口放置位置 */          \
    {"Qq",          "qq",               FIXED},                 \
    {"explorer.exe","explorer.exe",     FLOATING},              \
    {"Thunder.exe", "Thunder.exe",      FLOATING},              \
}

#endif
