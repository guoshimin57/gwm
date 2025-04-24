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
#include "grab.h"
#include "func.h"
#include "layout.h"
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
#define CMD(cmd_str) {.cmd=SH_CMD(cmd_str)}

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
static const Keybind keybinds[] =
{
/*  功能轉換鍵掩碼  鍵符號        要綁定的函數     函數的參數 */         
    {0,             XK_F1,        exec,            CMD(HELP)},
    {0, XF86XK_MonBrightnessDown, exec,            CMD(LIGHT_DOWN)},
    {0, XF86XK_MonBrightnessUp,   exec,            CMD(LIGHT_UP)},
    {CMD_KEY,       XK_f,         exec,            CMD(FILE_MANAGER)},
    {CMD_KEY,       XK_g,         exec,            CMD(GAME)},
    {CMD_KEY,       XK_q,         exec,            CMD("qq")},
    {CMD_KEY,       XK_t,         exec,            CMD(TERMINAL)},
    {CMD_KEY,       XK_w,         exec,            CMD(BROWSER)},
    {CMD_KEY,       XK_F1,        exec,            CMD(PLAY_START)},
    {CMD_KEY,       XK_F2,        exec,            CMD(PLAY_TOGGLE)},
    {CMD_KEY,       XK_F3,        exec,            CMD(PLAY_QUIT)},
    {SYS_KEY,       XK_F1,        exec,            CMD(VOLUME_DOWN)},
    {SYS_KEY,       XK_F2,        exec,            CMD(VOLUME_UP)},
    {SYS_KEY,       XK_F3,        exec,            CMD(VOLUME_MAX)},
    {SYS_KEY,       XK_F4,        exec,            CMD(VOLUME_TOGGLE)},
    {SYS_KEY,       XK_l,         exec,            CMD(LOGOUT)},
    {SYS_KEY,       XK_p,         exec,            CMD("poweroff")},
    {SYS_KEY,       XK_r,         exec,            CMD("reboot")},
    {WM_KEY,        XK_k,         move_up,         {0}},
    {WM_KEY,        XK_j,         move_down,       {0}},
    {WM_KEY,        XK_h,         move_left,       {0}},
    {WM_KEY,        XK_l,         move_right,      {0}},
    {WM_KEY,        XK_Up,        fall_height,     {0}},
    {WM_KEY,        XK_Down,      rise_height,     {0}},
    {WM_KEY,        XK_Left,      fall_width,      {0}},
    {WM_KEY,        XK_Right,     rise_width,      {0}},
    {WM_KEY,        XK_F1,        to_main_area,    {0}},
    {WM_KEY,        XK_F2,        to_second_area,  {0}},
    {WM_KEY,        XK_F3,        to_fixed_area,   {0}},
    {WM_KEY,        XK_F4,        to_stack_layer,  {0}},
    {WM_KEY,        XK_f,         fullscreen,      {0}},
    {WM_KEY,        XK_a,         to_above_layer,  {0}},
    {WM_KEY,        XK_b,         to_below_layer,  {0}},
    {WM_KEY,        XK_Return,    choose,          {0}},
    {WM_KEY,        XK_Tab,       next,            {0}},
    {WM_SKEY,       XK_Tab,       prev,            {0}},
    {WM_KEY,        XK_c,         quit,            {0}},
    {WM_KEY,        XK_s,         stack,           {0}},
    {WM_KEY,        XK_t,         tile,            {0}},
    {WM_KEY,        XK_i,         rise_main_n,     {0}},
    {WM_SKEY,       XK_i,         fall_main_n,     {0}},
    {WM_KEY,        XK_m,         rise_main_area,  {0}},
    {WM_SKEY,       XK_m,         fall_main_area,  {0}},
    {WM_KEY,        XK_x,         rise_fixed_area, {0}},
    {WM_SKEY,       XK_x,         fall_fixed_area, {0}},
    {WM_KEY,        XK_Page_Down, next_desktop,    {0}},
    {WM_KEY,        XK_Page_Up,   prev_desktop,    {0}},
    {0,             XK_Print,     print_screen,    {0}},
    {WM_KEY,        XK_Print,     print_win,       {0}},
    {WM_KEY,        XK_r,         run_cmd,         {0}},
    {WM_KEY,        XK_Delete,    quit_wm,         {0}},
    DESKTOP_KEYBIND(XK_0, ~0),                                                    
    DESKTOP_KEYBIND(XK_1, 0), // 注：我的鍵盤按super+左shift+1鍵時產生多鍵衝突
    DESKTOP_KEYBIND(XK_2, 1),                                                     
    DESKTOP_KEYBIND(XK_3, 2),                                                     
    {0} // 哨崗值，勿刪
};

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
static const Buttonbind buttonbinds[] =
{                                                                                         
    /* 構件標識        功能轉換鍵 定位器按鈕 要綁定的函數         函數的參數 */          
    {DESKTOP_BUTTON,       WM_KEY, Button2,  quit_all,            {0}},
    {CLIENT_WIN,           WM_KEY, Button1,  move,                {0}},
    {CLIENT_WIN,          WM_SKEY, Button1,  resize,              {0}},
    {CLIENT_WIN,           WM_KEY, Button2,  change_place,        {0}},
    {CLIENT_WIN,           WM_KEY, Button3,  swap,                {0}},
    {CLIENT_WIN,                0, Button3,  choose,              {0}},
    {CLIENT_ICON,               0, Button2,  change_place,        {0}},
    {CLIENT_ICON,          WM_KEY, Button2,  quit,                {0}},
    {CLIENT_ICON,               0, Button3,  swap,                {0}},
    {STACK_BUTTON,              0, Button1,  stack,               {0}},
    {TILE_BUTTON,               0, Button1,  tile,                {0}},
    {DESKTOP_BUTTON,            0, Button1,  show_desktop,        {0}},
    {ACT_CENTER_ITEM,           0, Button1,  start,               {0}},
    {HELP_BUTTON,               0, Button1,  exec,                CMD(HELP)},
    {FILE_BUTTON,               0, Button1,  exec,                CMD(FILE_MANAGER)},
    {TERM_BUTTON,               0, Button1,  exec,                CMD(TERMINAL)},
    {BROWSER_BUTTON,            0, Button1,  exec,                CMD(BROWSER)},
    {GAME_BUTTON,               0, Button1,  exec,                CMD(GAME)},
    {PLAY_START_BUTTON,         0, Button1,  exec,                CMD(PLAY_START)},
    {PLAY_TOGGLE_BUTTON,        0, Button1,  exec,                CMD(PLAY_TOGGLE)},
    {PLAY_QUIT_BUTTON,          0, Button1,  exec,                CMD(PLAY_QUIT)},
    {VOLUME_DOWN_BUTTON,        0, Button1,  exec,                CMD(VOLUME_DOWN)},
    {VOLUME_UP_BUTTON,          0, Button1,  exec,                CMD(VOLUME_UP)},
    {VOLUME_MAX_BUTTON,         0, Button1,  exec,                CMD(VOLUME_MAX)},
    {VOLUME_TOGGLE_BUTTON,      0, Button1,  exec,                CMD(VOLUME_TOGGLE)},
    {N_MAIN_UP_BUTTON,          0, Button1,  rise_main_n,         {0}},
    {N_MAIN_DOWN_BUTTON,        0, Button1,  fall_main_n,         {0}},
    {CLOSE_ALL_CLIENTS_BUTTON,  0, Button1,  quit_all,            {0}},
    {PRINT_WIN_BUTTON,          0, Button1,  print_win,           {0}},
    {PRINT_SCREEN_BUTTON,       0, Button1,  print_screen,        {0}},
    {FOCUS_MODE_BUTTON,         0, Button1,  toggle_focus_mode,   {0}},
    {COMPOSITOR_BUTTON,         0, Button1,  toggle_compositor,   {0}},
    {WALLPAPER_BUTTON,          0, Button1,  switch_wallpaper,    {0}},
    {COLOR_BUTTON,              0, Button1,  set_color,           {0}},
    {QUIT_WM_BUTTON,            0, Button1,  quit_wm,             {0}},
    {LOGOUT_BUTTON,             0, Button1,  exec,                CMD(LOGOUT)},
    {REBOOT_BUTTON,             0, Button1,  exec,                CMD("reboot")},
    {POWEROFF_BUTTON,           0, Button1,  exec,                CMD("poweroff")},
    {RUN_BUTTON,                0, Button1,  run_cmd,             {0}},
    {TITLE_LOGO,                0, Button1,  open_client_menu,    {0}},
    {VERT_MAX_BUTTON,           0, Button1,  vmax,                {0}},
    {HORZ_MAX_BUTTON,           0, Button1,  hmax,                {0}},
    {TOP_MAX_BUTTON,            0, Button1,  tmax,                {0}},
    {BOTTOM_MAX_BUTTON,         0, Button1,  bmax,                {0}},
    {LEFT_MAX_BUTTON,           0, Button1,  lmax,                {0}},
    {RIGHT_MAX_BUTTON,          0, Button1,  rmax,                {0}},
    {FULL_MAX_BUTTON,           0, Button1,  max,                 {0}},
    {FULLSCREEN_BUTTON,         0, Button1,  fullscreen,          {0}},
    {ABOVE_BUTTON,              0, Button1,  to_above_layer,      {0}},
    {BELOW_BUTTON,              0, Button1,  to_below_layer,      {0}},
    {ROOT_WIN,                  0, Button1,  adjust_layout_ratio, {0}},
    {MAIN_BUTTON,               0, Button1,  to_main_area,        {0}},
    {SECOND_BUTTON,             0, Button1,  to_second_area,      {0}},
    {FIXED_BUTTON,              0, Button1,  to_fixed_area,       {0}},
    {FLOAT_BUTTON,              0, Button1,  to_stack_layer,      {0}},
    {ICON_BUTTON,               0, Button1,  mini,                {0}},
    {MAX_BUTTON,                0, Button1,  toggle_max_restore,  {0}},
    {SHADE_BUTTON,              0, Button1,  toggle_shade,        {0}},
    {CLOSE_BUTTON,              0, Button1,  quit,                {0}},
    {TITLEBAR,                  0, Button1,  move,                {0}},
    {TITLEBAR,                  0, Button2,  change_place,        {0}},
    {TITLEBAR,                  0, Button3,  swap,                {0}},
    {CLIENT_WIN,                0, Button1,  choose,              {0}},
    {CLIENT_FRAME,              0, Button1,  resize,              {0}},
    {CLIENT_ICON,               0, Button1,  deiconify,           {0}},
    DESKTOP_BUTTONBIND(0),
    DESKTOP_BUTTONBIND(1),
    DESKTOP_BUTTONBIND(2),
    {0} // 哨崗值，勿刪
};

#endif
