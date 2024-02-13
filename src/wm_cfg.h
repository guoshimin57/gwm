/* *************************************************************************
 *     wm_cfg.h：窗口管理器相關配置的頭文件，用戶一般不必修改此配置。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef WM_CFG_H
#define WM_CFG_H

#define TOGGLE_PROCESS_STATE(process) /* 切換進程狀態 */ \
    "{ pid=$(ps -C '"process"' -o pid=); " \
    "ps -o stat= $pid | head -n1 | grep T > /dev/null ; } " \
    "&& kill -CONT $pid || kill -STOP $pid > /dev/null 2>&1"
#define DESKTOPN_BUTTON(n) DESKTOP ## n ##_BUTTON // 獲取虛擬桌面按鈕類型
#define SET_TITLE_BUTTON_TEXT(type, text) /* 設置標題按鈕文字 */ \
    cfg->title_button_text[WIDGET_INDEX(type, TITLE_BUTTON)]=text
#define SET_TASKBAR_BUTTON_TEXT(type, text) /* 設置任務欄按鈕文字 */ \
    cfg->taskbar_button_text[WIDGET_INDEX(type, TASKBAR_BUTTON)]=text
#define SET_ACT_CENTER_ITEM_TEXT(type, text) /* 設置操作中心菜單項文字 */ \
    cfg->act_center_item_text[WIDGET_INDEX(type, ACT_CENTER_ITEM)]=text
#define SET_CLIENT_MENU_ITEM_TEXT(type, text) /* 設置客戶窗口菜單項文字 */ \
    cfg->client_menu_item_text[WIDGET_INDEX(type, CLIENT_MENU_ITEM)]=text

/* 功能：設置應由窗口管理器去設定的定位器相關功能綁定。
 * 說明：通常把單擊的相關功能綁定放在這裏。
 */
#define WM_BUTTONBIND                                                                                       \
    /* 構件類型        功能轉換鍵 定位器按鈕 要綁定的函數                函數的參數 */                      \
    {PREVIEW_BUTTON,            0, Button1,  change_layout,              {.layout=PREVIEW}},                \
    {STACK_BUTTON,              0, Button1,  change_layout,              {.layout=STACK}},                  \
    {TILE_BUTTON,               0, Button1,  change_layout,              {.layout=TILE}},                   \
    {DESKTOP_BUTTON,            0, Button1,  show_desktop,               {0}},                              \
    {ACT_CENTER_ITEM,           0, Button1,  open_act_center,            {0}},                              \
    {HELP_BUTTON,               0, Button1,  exec,                       SH_CMD(HELP)},                     \
    {FILE_BUTTON,               0, Button1,  exec,                       SH_CMD(FILE_MANAGER)},             \
    {TERM_BUTTON,               0, Button1,  exec,                       SH_CMD(TERMINAL)},                 \
    {BROWSER_BUTTON,            0, Button1,  exec,                       SH_CMD(BROWSER)},                  \
    {GAME_BUTTON,               0, Button1,  exec,                       SH_CMD(GAME)},                     \
    {PLAY_START_BUTTON,         0, Button1,  exec,                       SH_CMD(PLAY_START)},               \
    {PLAY_TOGGLE_BUTTON,        0, Button1,  exec,                       SH_CMD(PLAY_TOGGLE)},              \
    {PLAY_QUIT_BUTTON,          0, Button1,  exec,                       SH_CMD(PLAY_QUIT)},                \
    {VOLUME_DOWN_BUTTON,        0, Button1,  exec,                       SH_CMD(VOLUME_DOWN)},              \
    {VOLUME_UP_BUTTON,          0, Button1,  exec,                       SH_CMD(VOLUME_UP)},                \
    {VOLUME_MAX_BUTTON,         0, Button1,  exec,                       SH_CMD(VOLUME_MAX)},               \
    {VOLUME_TOGGLE_BUTTON,      0, Button1,  exec,                       SH_CMD(VOLUME_TOGGLE)},            \
    {MAIN_NEW_BUTTON,           0, Button1,  change_default_place_type,  {.place_type=TILE_LAYER_MAIN}},    \
    {SEC_NEW_BUTTON,            0, Button1,  change_default_place_type,  {.place_type=TILE_LAYER_SECOND}},  \
    {FIX_NEW_BUTTON,            0, Button1,  change_default_place_type,  {.place_type=TILE_LAYER_FIXED}},   \
    {FLOAT_NEW_BUTTON,          0, Button1,  change_default_place_type,  {.place_type=FLOAT_LAYER}},        \
    {N_MAIN_UP_BUTTON,          0, Button1,  adjust_n_main_max,          {.n=1}},                           \
    {N_MAIN_DOWN_BUTTON,        0, Button1,  adjust_n_main_max,          {.n=-1}},                          \
    {TITLEBAR_TOGGLE_BUTTON,    0, Button1,  toggle_titlebar_visibility, {0}},                              \
    {CLI_BORDER_TOGGLE_BUTTON,  0, Button1,  toggle_border_visibility,   {0}},                              \
    {CLOSE_ALL_CLIENTS_BUTTON,  0, Button1,  close_all_clients,          {0}},                              \
    {PRINT_WIN_BUTTON,          0, Button1,  print_win,                  {0}},                              \
    {PRINT_SCREEN_BUTTON,       0, Button1,  print_screen,               {0}},                              \
    {FOCUS_MODE_BUTTON,         0, Button1,  toggle_focus_mode,          {0}},                              \
    {COMPOSITOR_BUTTON,         0, Button1,  toggle_compositor,          {0}},                              \
    {WALLPAPER_BUTTON,          0, Button1,  switch_wallpaper,           {0}},                              \
    {COLOR_THEME_BUTTON,        0, Button1,  switch_color_theme,         {0}},                              \
    {QUIT_WM_BUTTON,            0, Button1,  quit_wm,                    {0}},                              \
    {LOGOUT_BUTTON,             0, Button1,  exec,                       SH_CMD(LOGOUT)},                   \
    {REBOOT_BUTTON,             0, Button1,  exec,                       SH_CMD("reboot")},                 \
    {POWEROFF_BUTTON,           0, Button1,  exec,                       SH_CMD("poweroff")},               \
    {RUN_BUTTON,                0, Button1,  enter_and_run_cmd,          {0}},                              \
    {TITLE_LOGO,                0, Button1,  open_client_menu,           {0}},                              \
    {VERT_MAX_BUTTON,           0, Button1,  maximize,                   {.max_way=VERT_MAX}},              \
    {HORZ_MAX_BUTTON,           0, Button1,  maximize,                   {.max_way=HORZ_MAX}},              \
    {TOP_MAX_BUTTON,            0, Button1,  maximize,                   {.max_way=TOP_MAX}},               \
    {BOTTOM_MAX_BUTTON,         0, Button1,  maximize,                   {.max_way=BOTTOM_MAX}},            \
    {LEFT_MAX_BUTTON,           0, Button1,  maximize,                   {.max_way=LEFT_MAX}},              \
    {RIGHT_MAX_BUTTON,          0, Button1,  maximize,                   {.max_way=RIGHT_MAX}},             \
    {FULL_MAX_BUTTON,           0, Button1,  maximize,                   {.max_way=FULL_MAX}},              \
    {ROOT_WIN,                  0, Button1,  adjust_layout_ratio,        {0}},                              \
    {MAIN_BUTTON,               0, Button1,  change_place,               {.place_type=TILE_LAYER_MAIN}},    \
    {SECOND_BUTTON,             0, Button1,  change_place,               {.place_type=TILE_LAYER_SECOND}},  \
    {FIXED_BUTTON,              0, Button1,  change_place,               {.place_type=TILE_LAYER_FIXED}},   \
    {FLOAT_BUTTON,              0, Button1,  change_place,               {.place_type=FLOAT_LAYER}},        \
    {ICON_BUTTON,               0, Button1,  minimize,                   {0}},                              \
    {MAX_BUTTON,                0, Button1,  max_restore,                {0}},                              \
    {SHADE_BUTTON,              0, Button1,  toggle_shade_client,        {0}},                              \
    {CLOSE_BUTTON,              0, Button1,  close_client,               {0}},                              \
    {TITLE_AREA,                0, Button1,  move_resize,                {.resize=false}},                  \
    {TITLE_AREA,                0, Button2,  pointer_change_place,       {0}},                              \
    {TITLE_AREA,                0, Button3,  pointer_swap_clients,       {0}},                              \
    {CLIENT_WIN,                0, Button1,  choose_client,              {0}},                              \
    {CLIENT_FRAME,              0, Button1,  move_resize,                {.resize=true}},                   \
    {CLIENT_ICON,               0, Button1,  deiconify,                  {0}}

#endif
