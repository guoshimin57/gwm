/* *************************************************************************
 *     bind_cfg.h：配置功能綁定的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef BIND_CFG_H
#define BIND_CFG_H

#include <X11/keysymdef.h>
#include <X11/XF86keysym.h>
#include "func.h"
#include "layout.h"
#include "minimax.h"
#include "mvresize.h"
#include "place.h"

#define WM_KEY Mod4Mask // 窗口管理器的基本功能轉換鍵
#define WM_SKEY (WM_KEY|ShiftMask) // 與WM_KEY功能相關功能轉換鍵，通常表示相反
#define CMD_KEY (WM_KEY|Mod1Mask) // 與系統命令相關功能的轉換鍵
#define SYS_KEY (WM_KEY|ControlMask) // 與系統相關的功能轉換鍵
                                     
#define GAME "wesnoth || flatpak run org.wesnoth.Wesnoth" // 打開遊戲程序
#define PLAY_START "mplayer -shuffle ~/music/*" // 打開影音播放程序
#define PLAY_TOGGLE TOGGLE_PROCESS_STATE(PLAY_START) // 切換影音播放程序啓停狀態
#define PLAY_QUIT "kill -KILL $(ps -C '"PLAY_START"' -o pid=)" // 退出影音播放程序
#define HELP /* 打開gwm手冊 */ \
    "lxterminal -e 'man gwm' || xfce4-terminal -e 'man gwm' || xterm -e 'man gwm'"
#define FILE_MANAGER "xdg-open ~" // 打開偏好的文件管理器
#define BROWSER "xdg-open http://bing.com" // 打開偏好的網絡瀏覽器
#define TERMINAL /* 打開終端模擬器 */ \
    "lxterminal || xfce4-terminal || gnome-terminal || konsole5 || xterm"
#define VOLUME_DOWN "amixer -q sset Master 10%-" // 調低音量
#define VOLUME_UP "amixer -q sset Master 10%+" // 調高音量
#define VOLUME_MAX "amixer -q sset Master 100%" // 調至最大音量
#define VOLUME_TOGGLE "amixer -q sset Master toggle" // 靜音切換
// 對於臺式機顯示器，應使用“xrandr --output HDMI-0 --brightness 0.5”之類的命令代替以下亮度調節命令
#define LIGHT_DOWN "light -U 5" // 調低屏幕亮度
#define LIGHT_UP "light -A 5" // 調高屏幕亮度
#define LOGOUT "pkill -9 'startgwm|gwm'" // 注銷
#define TOGGLE_PROCESS_STATE(process) /* 切換進程狀態 */ \
    "{ pid=$(ps -C '"process"' -o pid=); " \
    "ps -o stat= $pid | head -n1 | grep T > /dev/null ; } " \
    "&& kill -CONT $pid || kill -STOP $pid > /dev/null 2>&1"
#define DESKTOPN_BUTTON(n) DESKTOP ## n ##_BUTTON // 獲取虛擬桌面按鈕類型

/* 功能：設置與虛擬桌面相關的按鍵功能綁定。
 * 說明：邏輯功能轉換鍵掩碼的定義詳見<X11/X.h>，用xmodmap(1)命令可查看與功能
 * 轉換鍵符號的對應關系，當功能轉換鍵爲0時，表示不綁定任何功能轉換鍵。下同。
 * 鍵符號的定義詳見<X11/keysymdef.h>和<X11/XF86keysym.h>。下同。
 * 可綁定的函數詳見func.h，相應的函數參數詳見gwm.h。下同。
 * n=~0表示所有虛擬桌面，僅適用於attach_to_all_desktops。
 * 格式：
 *     功能轉換鍵掩碼     鍵符號 要綁定的函數             函數的參數
 */
#define DESKTOP_KEYBIND(key, n)                                            \
    {WM_KEY|ShiftMask,      key, focus_desktop,           {.desktop_n=n}}, \
    {WM_KEY,	            key, move_to_desktop,         {.desktop_n=n}}, \
    {WM_KEY|Mod1Mask,	    key, all_move_to_desktop,     {.desktop_n=n}}, \
    {ControlMask,           key, change_to_desktop,       {.desktop_n=n}}, \
    {ControlMask|Mod1Mask,  key, all_change_to_desktop,   {.desktop_n=n}}, \
    {Mod1Mask,              key, attach_to_desktop,       {.desktop_n=n}}, \
    {Mod1Mask|ShiftMask,    key, all_attach_to_desktop,   {.desktop_n=n}}, \
    {ShiftMask|ControlMask, key, attach_to_all_desktops,  {.desktop_n=n}}

/* 功能：設置按鍵功能綁定。 */
#define KEYBIND (Keybind []) \
{                                                                                   \
/*  功能轉換鍵掩碼  鍵符號        要綁定的函數              函數的參數 */           \
    {0,             XK_F1,        exec,                    SH_CMD(HELP)},           \
    {0, XF86XK_MonBrightnessDown, exec,                    SH_CMD(LIGHT_DOWN)},     \
    {0, XF86XK_MonBrightnessUp,   exec,                    SH_CMD(LIGHT_UP)},       \
    {CMD_KEY,       XK_f,         exec,                    SH_CMD(FILE_MANAGER)},   \
    {CMD_KEY,       XK_g,         exec,                    SH_CMD(GAME)},           \
    {CMD_KEY,       XK_q,         exec,                    SH_CMD("qq")},           \
    {CMD_KEY,       XK_t,         exec,                    SH_CMD(TERMINAL)},       \
    {CMD_KEY,       XK_w,         exec,                    SH_CMD(BROWSER)},        \
    {CMD_KEY,       XK_F1,        exec,                    SH_CMD(PLAY_START)},     \
    {CMD_KEY,       XK_F2,        exec,                    SH_CMD(PLAY_TOGGLE)},    \
    {CMD_KEY,       XK_F3,        exec,                    SH_CMD(PLAY_QUIT)},      \
    {SYS_KEY,       XK_F1,        exec,                    SH_CMD(VOLUME_DOWN)},    \
    {SYS_KEY,       XK_F2,        exec,                    SH_CMD(VOLUME_UP)},      \
    {SYS_KEY,       XK_F3,        exec,                    SH_CMD(VOLUME_MAX)},     \
    {SYS_KEY,       XK_F4,        exec,                    SH_CMD(VOLUME_TOGGLE)},  \
    {SYS_KEY,       XK_l,         exec,                    SH_CMD(LOGOUT)},         \
    {SYS_KEY,       XK_p,         exec,                    SH_CMD("poweroff")},     \
    {SYS_KEY,       XK_r,         exec,                    SH_CMD("reboot")},       \
    {WM_KEY,        XK_k,         key_move_up,             {0}},                    \
    {WM_KEY,        XK_j,         key_move_down,           {0}},                    \
    {WM_KEY,        XK_h,         key_move_left,           {0}},                    \
    {WM_KEY,        XK_l,         key_move_right,          {0}},                    \
    {WM_KEY,        XK_Up,        key_resize_up2up,        {0}},                    \
    {WM_SKEY,       XK_Up,        key_resize_up2down,      {0}},                    \
    {WM_KEY,        XK_Down,      key_resize_down2down,    {0}},                    \
    {WM_SKEY,       XK_Down,      key_resize_down2up,      {0}},                    \
    {WM_KEY,        XK_Left,      key_resize_left2left,    {0}},                    \
    {WM_SKEY,       XK_Left,      key_resize_left2right,   {0}},                    \
    {WM_KEY,        XK_Right,     key_resize_right2right,  {0}},                    \
    {WM_SKEY,       XK_Right,     key_resize_right2left,   {0}},                    \
    {WM_KEY,        XK_F1,        change_to_main,          {0}},                    \
    {WM_KEY,        XK_F2,        change_to_second,        {0}},                    \
    {WM_KEY,        XK_F3,        change_to_fixed,         {0}},                    \
    {WM_KEY,        XK_F4,        change_to_float,         {0}},                    \
    {WM_KEY,        XK_Return,    choose_client,           {0}},                    \
    {WM_KEY,        XK_Tab,       next_client,             {0}},                    \
    {WM_SKEY,       XK_Tab,       prev_client,             {0}},                    \
    {WM_KEY,        XK_c,         close_client,            {0}},                    \
    {WM_KEY,        XK_p,         change_to_preview,       {0}},                    \
    {WM_KEY,        XK_s,         change_to_stack,         {0}},                    \
    {WM_KEY,        XK_t,         change_to_tile,          {0}},                    \
    {WM_KEY,        XK_i,         increase_main_n,         {0}},                    \
    {WM_SKEY,       XK_i,         decrease_main_n,         {0}},                    \
    {WM_KEY,        XK_m,         key_increase_main_area,  {0}},                    \
    {WM_SKEY,       XK_m,         key_decrease_main_area,  {0}},                    \
    {WM_KEY,        XK_x,         key_increase_fixed_area, {0}},                    \
    {WM_SKEY,       XK_x,         key_decrease_fixed_area, {0}},                    \
    {WM_KEY,        XK_Page_Down, next_desktop,            {0}},                    \
    {WM_KEY,        XK_Page_Up,   prev_desktop,            {0}},                    \
    {0,             XK_Print,     print_screen,            {0}},                    \
    {WM_KEY,        XK_Print,     print_win,               {0}},                    \
    {WM_KEY,        XK_r,         run_cmd,                 {0}},                    \
    {WM_KEY,        XK_Delete,    quit_wm,                 {0}},                    \
    DESKTOP_KEYBIND(XK_0, ~0),                                                      \
    DESKTOP_KEYBIND(XK_1, 0), /* 注：我的鍵盤按super+左shift+1鍵時產生多鍵衝突 */   \
    DESKTOP_KEYBIND(XK_2, 1),                                                       \
    DESKTOP_KEYBIND(XK_3, 2),                                                       \
}

/* 功能：設置與虛擬桌面相關的定位器按鈕功能綁定。
 * 說明：可以用xev(1)命令來檢測定位器按鈕。
 */
#define DESKTOP_BUTTONBIND(n)                                                              \
    /* 虛擬桌面n                    功能轉換鍵  定位器按鈕 要綁定的函數       函數的參數 */\
    {DESKTOPN_BUTTON(n),                    0,    Button1, focus_desktop,          {0}},   \
    {DESKTOPN_BUTTON(n),          ControlMask,    Button1, change_to_desktop,      {0}},   \
    {DESKTOPN_BUTTON(n), Mod1Mask|ControlMask,    Button1, all_change_to_desktop,  {0}},   \
    {DESKTOPN_BUTTON(n),                    0,    Button2, attach_to_desktop,      {0}},   \
    {DESKTOPN_BUTTON(n),             Mod1Mask,    Button2, all_attach_to_desktop,  {0}},   \
    {DESKTOPN_BUTTON(n),            ShiftMask,    Button2, attach_to_all_desktops, {0}},   \
    {DESKTOPN_BUTTON(n),                    0,    Button3, move_to_desktop,        {0}},   \
    {DESKTOPN_BUTTON(n),             Mod1Mask,    Button3, all_move_to_desktop,    {0}}

/* 功能：設置定位器按鈕功能綁定。*/
#define BUTTONBIND (Buttonbind [])  \
{                                                                                          \
    /* 構件標識        功能轉換鍵 定位器按鈕 要綁定的函數          函數的參數 */           \
    {DESKTOP_BUTTON,       WM_KEY, Button2,  close_all_clients,    {0}},                   \
    {CLIENT_WIN,           WM_KEY, Button1,  pointer_move,         {0}},                   \
    {CLIENT_WIN,          WM_SKEY, Button1,  pointer_resize,       {0}},                   \
    {CLIENT_WIN,           WM_KEY, Button2,  pointer_change_place, {0}},                   \
    {CLIENT_WIN,           WM_KEY, Button3,  pointer_swap_clients, {0}},                   \
    {CLIENT_WIN,                0, Button3,  choose_client,        {0}},                   \
    {CLIENT_ICON,               0, Button2,  pointer_change_place, {0}},                   \
    {CLIENT_ICON,          WM_KEY, Button2,  close_client,         {0}},                   \
    {CLIENT_ICON,               0, Button3,  pointer_swap_clients, {0}},                   \
    {PREVIEW_BUTTON,            0, Button1,  change_to_preview,    {0}},                   \
    {STACK_BUTTON,              0, Button1,  change_to_stack,      {0}},                   \
    {TILE_BUTTON,               0, Button1,  change_to_tile,       {0}},                   \
    {DESKTOP_BUTTON,            0, Button1,  show_desktop,         {0}},                   \
    {ACT_CENTER_ITEM,           0, Button1,  open_act_center,      {0}},                   \
    {HELP_BUTTON,               0, Button1,  exec,                 SH_CMD(HELP)},          \
    {FILE_BUTTON,               0, Button1,  exec,                 SH_CMD(FILE_MANAGER)},  \
    {TERM_BUTTON,               0, Button1,  exec,                 SH_CMD(TERMINAL)},      \
    {BROWSER_BUTTON,            0, Button1,  exec,                 SH_CMD(BROWSER)},       \
    {GAME_BUTTON,               0, Button1,  exec,                 SH_CMD(GAME)},          \
    {PLAY_START_BUTTON,         0, Button1,  exec,                 SH_CMD(PLAY_START)},    \
    {PLAY_TOGGLE_BUTTON,        0, Button1,  exec,                 SH_CMD(PLAY_TOGGLE)},   \
    {PLAY_QUIT_BUTTON,          0, Button1,  exec,                 SH_CMD(PLAY_QUIT)},     \
    {VOLUME_DOWN_BUTTON,        0, Button1,  exec,                 SH_CMD(VOLUME_DOWN)},   \
    {VOLUME_UP_BUTTON,          0, Button1,  exec,                 SH_CMD(VOLUME_UP)},     \
    {VOLUME_MAX_BUTTON,         0, Button1,  exec,                 SH_CMD(VOLUME_MAX)},    \
    {VOLUME_TOGGLE_BUTTON,      0, Button1,  exec,                 SH_CMD(VOLUME_TOGGLE)}, \
    {N_MAIN_UP_BUTTON,          0, Button1,  increase_main_n,      {0}},                   \
    {N_MAIN_DOWN_BUTTON,        0, Button1,  decrease_main_n,      {0}},                   \
    {CLOSE_ALL_CLIENTS_BUTTON,  0, Button1,  close_all_clients,    {0}},                   \
    {PRINT_WIN_BUTTON,          0, Button1,  print_win,            {0}},                   \
    {PRINT_SCREEN_BUTTON,       0, Button1,  print_screen,         {0}},                   \
    {FOCUS_MODE_BUTTON,         0, Button1,  toggle_focus_mode,    {0}},                   \
    {COMPOSITOR_BUTTON,         0, Button1,  toggle_compositor,    {0}},                   \
    {WALLPAPER_BUTTON,          0, Button1,  switch_wallpaper,     {0}},                   \
    {COLOR_BUTTON,              0, Button1,  set_color,            {0}},                   \
    {QUIT_WM_BUTTON,            0, Button1,  quit_wm,              {0}},                   \
    {LOGOUT_BUTTON,             0, Button1,  exec,                 SH_CMD(LOGOUT)},        \
    {REBOOT_BUTTON,             0, Button1,  exec,                 SH_CMD("reboot")},      \
    {POWEROFF_BUTTON,           0, Button1,  exec,                 SH_CMD("poweroff")},    \
    {RUN_BUTTON,                0, Button1,  run_cmd,              {0}},                   \
    {TITLE_LOGO,                0, Button1,  open_client_menu,     {0}},                   \
    {VERT_MAX_BUTTON,           0, Button1,  vert_maximize,        {0}},                   \
    {HORZ_MAX_BUTTON,           0, Button1,  horz_maximize,        {0}},                   \
    {TOP_MAX_BUTTON,            0, Button1,  top_maximize,         {0}},                   \
    {BOTTOM_MAX_BUTTON,         0, Button1,  bottom_maximize,      {0}},                   \
    {LEFT_MAX_BUTTON,           0, Button1,  left_maximize,        {0}},                   \
    {RIGHT_MAX_BUTTON,          0, Button1,  right_maximize,       {0}},                   \
    {FULL_MAX_BUTTON,           0, Button1,  full_maximize,        {0}},                   \
    {ROOT_WIN,                  0, Button1,  adjust_layout_ratio,  {0}},                   \
    {MAIN_BUTTON,               0, Button1,  change_to_main,       {0}},                   \
    {SECOND_BUTTON,             0, Button1,  change_to_second,     {0}},                   \
    {FIXED_BUTTON,              0, Button1,  change_to_fixed,      {0}},                   \
    {FLOAT_BUTTON,              0, Button1,  change_to_float,      {0}},                   \
    {ICON_BUTTON,               0, Button1,  minimize,             {0}},                   \
    {MAX_BUTTON,                0, Button1,  max_restore,          {0}},                   \
    {SHADE_BUTTON,              0, Button1,  toggle_shade_client,  {0}},                   \
    {CLOSE_BUTTON,              0, Button1,  close_client,         {0}},                   \
    {TITLEBAR,                  0, Button1,  pointer_move,         {0}},                   \
    {TITLEBAR,                  0, Button2,  pointer_change_place, {0}},                   \
    {TITLEBAR,                  0, Button3,  pointer_swap_clients, {0}},                   \
    {CLIENT_WIN,                0, Button1,  choose_client,        {0}},                   \
    {CLIENT_FRAME,              0, Button1,  pointer_resize,       {0}},                   \
    {CLIENT_ICON,               0, Button1,  deiconify,            {0}},                   \
    DESKTOP_BUTTONBIND(0),                                                                 \
    DESKTOP_BUTTONBIND(1),                                                                 \
    DESKTOP_BUTTONBIND(2),                                                                 \
}

#endif
