/* *************************************************************************
 *     config.c：配置gwm。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

/* =========================== 以下爲非用戶配置項 =========================== */ 
#define TOGGLE_PROCESS_STATE(process) /* 切換進程狀態 */ \
    "{ pid=$(ps -C '"process"' -o pid=); " \
    "ps -o stat= $pid | head -n1 | grep T > /dev/null ; } " \
    "&& kill -CONT $pid || kill -STOP $pid > /dev/null 2>&1"
#define DESKTOPN_BUTTON(n) DESKTOP ## n ##_BUTTON // 獲取虛擬桌面按鈕類型
#define SET_FONT(wm, type, family, size) /* 設置字體 */ \
    sprintf(wm->cfg->font_name[type], "%s:pixelsize=%u", \
        family, wm->cfg->font_size[type]=size)
#define SET_TITLE_BUTTON_TEXT(wm, type, text) /* 設置標題按鈕文字 */ \
    wm->cfg->title_button_text[WIDGET_INDEX(type, TITLE_BUTTON)]=text
#define SET_TASKBAR_BUTTON_TEXT(wm, type, text) /* 設置任務欄按鈕文字 */ \
    wm->cfg->taskbar_button_text[WIDGET_INDEX(type, TASKBAR_BUTTON)]=text
#define SET_ACT_CENTER_ITEM_TEXT(wm, type, text) /* 設置操作中心菜單項文字 */ \
    wm->cfg->act_center_item_text[WIDGET_INDEX(type, ACT_CENTER_ITEM)]=text
#define SET_CLIENT_MENU_ITEM_TEXT(wm, type, text) /* 設置客戶窗口菜單項文字 */ \
    wm->cfg->client_menu_item_text[WIDGET_INDEX(type, CLIENT_MENU_ITEM)]=text


/* =========================== 以下爲用戶配置項 =========================== */ 

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

/* 功能：設置與虛擬桌面相關的按鍵功能綁定。
 * 說明：邏輯功能轉換鍵掩碼的定義詳見<X11/X.h>，用xmodmap(1)命令可查看與功能
 * 轉換鍵符號的對應關系，當功能轉換鍵爲0時，表示不綁定任何功能轉換鍵。下同。
 * 鍵符號的定義詳見<X11/keysymdef.h>和<X11/XF86keysym.h>。下同。
 * 可綁定的函數詳見func.h，相應的函數參數詳見gwm.h。下同。
 * n=0表示所有虛擬桌面，僅適用於attach_to_all_desktops。
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

/* 功能：設置按鍵功能綁定。
 * 說明：Keybind的定義詳見gwm.h。
 */
static const Keybind keybind[] =
{
/*  功能轉換鍵掩碼  鍵符號        要綁定的函數                 函數的參數 */
    {0,             XK_F1,        exec,                        SH_CMD(HELP)},
    {0, XF86XK_MonBrightnessDown, exec,                        SH_CMD(LIGHT_DOWN)},
    {0, XF86XK_MonBrightnessUp,   exec,                        SH_CMD(LIGHT_UP)},
    {CMD_KEY,       XK_f,         exec,                        SH_CMD(FILE_MANAGER)},
    {CMD_KEY,       XK_g,         exec,                        SH_CMD(GAME)},
    {CMD_KEY,       XK_q,         exec,                        SH_CMD("qq")},
    {CMD_KEY,       XK_t,         exec,                        SH_CMD(TERMINAL)},
    {CMD_KEY,       XK_w,         exec,                        SH_CMD(BROWSER)},
    {CMD_KEY,       XK_F1,        exec,                        SH_CMD(PLAY_START)},
    {CMD_KEY,       XK_F2,        exec,                        SH_CMD(PLAY_TOGGLE)},
    {CMD_KEY,       XK_F3,        exec,                        SH_CMD(PLAY_QUIT)},
    {SYS_KEY,       XK_F1,        exec,                        SH_CMD(VOLUME_DOWN)},
    {SYS_KEY,       XK_F2,        exec,                        SH_CMD(VOLUME_UP)},
    {SYS_KEY,       XK_F3,        exec,                        SH_CMD(VOLUME_MAX)},
    {SYS_KEY,       XK_F4,        exec,                        SH_CMD(VOLUME_TOGGLE)},
    {SYS_KEY,       XK_l,         exec,                        SH_CMD(LOGOUT)},
    {SYS_KEY,       XK_p,         exec,                        SH_CMD("poweroff")},
    {SYS_KEY,       XK_r,         exec,                        SH_CMD("reboot")},
    {WM_KEY,        XK_k,         key_move_resize_client,      {.direction=UP}},
    {WM_KEY,        XK_j,         key_move_resize_client,      {.direction=DOWN}},
    {WM_KEY,        XK_h,         key_move_resize_client,      {.direction=LEFT}},
    {WM_KEY,        XK_l,         key_move_resize_client,      {.direction=RIGHT}},
    {WM_KEY,        XK_Up,        key_move_resize_client,      {.direction=UP2UP}},
    {WM_SKEY,       XK_Up,        key_move_resize_client,      {.direction=UP2DOWN}},
    {WM_KEY,        XK_Down,      key_move_resize_client,      {.direction=DOWN2DOWN}},
    {WM_SKEY,       XK_Down,      key_move_resize_client,      {.direction=DOWN2UP}},
    {WM_KEY,        XK_Left,      key_move_resize_client,      {.direction=LEFT2LEFT}},
    {WM_SKEY,       XK_Left,      key_move_resize_client,      {.direction=LEFT2RIGHT}},
    {WM_KEY,        XK_Right,     key_move_resize_client,      {.direction=RIGHT2RIGHT}},
    {WM_SKEY,       XK_Right,     key_move_resize_client,      {.direction=RIGHT2LEFT}},
    {WM_KEY,        XK_F1,        change_area,                 {.area_type=MAIN_AREA}},
    {WM_KEY,        XK_F2,        change_area,                 {.area_type=SECOND_AREA}},
    {WM_KEY,        XK_F3,        change_area,                 {.area_type=FIXED_AREA}},
    {WM_KEY,        XK_F4,        change_area,                 {.area_type=FLOATING_AREA}},
    {WM_KEY,        XK_F5,        change_area,                 {.area_type=ICONIFY_AREA}},
    {WM_KEY,        XK_Return,    choose_client,               {0}},
    {WM_KEY,        XK_Tab,       next_client,                 {0}},
    {WM_SKEY,       XK_Tab,       prev_client,                 {0}},
    {WM_KEY,        XK_c,         close_client,                {0}},
    {WM_KEY,        XK_f,         change_layout,               {.layout=FULL}},
    {WM_KEY,        XK_p,         change_layout,               {.layout=PREVIEW}},
    {WM_KEY,        XK_s,         change_layout,               {.layout=STACK}},
    {WM_KEY,        XK_t,         change_layout,               {.layout=TILE}},
    {WM_KEY,        XK_i,         adjust_n_main_max,           {.n=1}},
    {WM_SKEY,       XK_i,         adjust_n_main_max,           {.n=-1}},
    {WM_KEY,        XK_m,         adjust_main_area_ratio,      {.change_ratio=0.01}},
    {WM_SKEY,       XK_m,         adjust_main_area_ratio,      {.change_ratio=-0.01}},
    {WM_KEY,        XK_x,         adjust_fixed_area_ratio,     {.change_ratio=0.01}},
    {WM_SKEY,       XK_x,         adjust_fixed_area_ratio,     {.change_ratio=-0.01}},
    {WM_KEY,        XK_Page_Down, next_desktop,                {0}},
    {WM_KEY,        XK_Page_Up,   prev_desktop,                {0}},
    {0,             XK_Print,     print_screen,                {0}},
    {WM_KEY,        XK_Print,     print_win,                   {0}},
    DESKTOP_KEYBIND(XK_0, 0),
    DESKTOP_KEYBIND(XK_1, 1), /* 注：我的鍵盤按super+左shift+1鍵時產生多鍵衝突 */
    DESKTOP_KEYBIND(XK_2, 2),
    DESKTOP_KEYBIND(XK_3, 3),
    {0} // 哨兵值，表示結束，切勿刪改之
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

/* 功能：設置定位器按鈕功能綁定。
 * 說明：Buttonbind的定義詳見gwm.h。
 */
static const Buttonbind buttonbind[] =
{
    DESKTOP_BUTTONBIND(1), 
    DESKTOP_BUTTONBIND(2), 
    DESKTOP_BUTTONBIND(3), 

    /* 構件類型        功能轉換鍵 定位器按鈕 要綁定的函數                函數的參數 */
    {FULL_BUTTON,               0, Button1,  change_layout,              {.layout=FULL}},
    {PREVIEW_BUTTON,            0, Button1,  change_layout,              {.layout=PREVIEW}},
    {STACK_BUTTON,              0, Button1,  change_layout,              {.layout=STACK}},
    {TILE_BUTTON,               0, Button1,  change_layout,              {.layout=TILE}},
    {DESKTOP_BUTTON,            0, Button1,  show_desktop,               {0}},
    {DESKTOP_BUTTON,       WM_KEY, Button2,  close_all_clients,          {0}},
    {ACT_CENTER_ITEM,           0, Button1,  open_act_center,            {0}},
    {HELP_BUTTON,               0, Button1,  exec,                       SH_CMD(HELP)},
    {FILE_BUTTON,               0, Button1,  exec,                       SH_CMD(FILE_MANAGER)},
    {TERM_BUTTON,               0, Button1,  exec,                       SH_CMD(TERMINAL)},
    {BROWSER_BUTTON,            0, Button1,  exec,                       SH_CMD(BROWSER)},
    {PLAY_START_BUTTON,         0, Button1,  exec,                       SH_CMD(PLAY_START)},
    {PLAY_TOGGLE_BUTTON,        0, Button1,  exec,                       SH_CMD(PLAY_TOGGLE)},
    {PLAY_QUIT_BUTTON,          0, Button1,  exec,                       SH_CMD(PLAY_QUIT)},
    {VOLUME_DOWN_BUTTON,        0, Button1,  exec,                       SH_CMD(VOLUME_DOWN)},
    {VOLUME_UP_BUTTON,          0, Button1,  exec,                       SH_CMD(VOLUME_UP)},
    {VOLUME_MAX_BUTTON,         0, Button1,  exec,                       SH_CMD(VOLUME_MAX)},
    {VOLUME_TOGGLE_BUTTON,      0, Button1,  exec,                       SH_CMD(VOLUME_TOGGLE)},
    {MAIN_NEW_BUTTON,           0, Button1,  change_default_area_type,   {.area_type=MAIN_AREA}},
    {SEC_NEW_BUTTON,            0, Button1,  change_default_area_type,   {.area_type=SECOND_AREA}},
    {FIX_NEW_BUTTON,            0, Button1,  change_default_area_type,   {.area_type=FIXED_AREA}},
    {FLOAT_NEW_BUTTON,          0, Button1,  change_default_area_type,   {.area_type=FLOATING_AREA}},
    {ICON_NEW_BUTTON,           0, Button1,  change_default_area_type,   {.area_type=ICONIFY_AREA}},
    {N_MAIN_UP_BUTTON,          0, Button1,  adjust_n_main_max,          {.n=1}},
    {N_MAIN_DOWN_BUTTON,        0, Button1,  adjust_n_main_max,          {.n=-1}},
    {TITLEBAR_TOGGLE_BUTTON,    0, Button1,  toggle_titlebar_visibility, {0}},
    {CLI_BORDER_TOGGLE_BUTTON,  0, Button1,  toggle_border_visibility,   {0}},
    {CLOSE_ALL_CLIENTS_BUTTON,  0, Button1,  close_all_clients,          {0}},
    {PRINT_WIN_BUTTON,          0, Button1,  print_win,                  {0}},
    {PRINT_SCREEN_BUTTON,       0, Button1,  print_screen,               {0}},
    {FOCUS_MODE_BUTTON,         0, Button1,  toggle_focus_mode,          {0}},
    {COMPOSITOR_BUTTON,         0, Button1,  toggle_compositor,          {0}},
    {WALLPAPER_BUTTON,          0, Button1,  switch_wallpaper,           {0}},
    {COLOR_THEME_BUTTON,        0, Button1,  switch_color_theme,         {0}},
    {QUIT_WM_BUTTON,            0, Button1,  quit_wm,                    {0}},
    {LOGOUT_BUTTON,             0, Button1,  exec,                       SH_CMD(LOGOUT)},
    {REBOOT_BUTTON,             0, Button1,  exec,                       SH_CMD("reboot")},
    {POWEROFF_BUTTON,           0, Button1,  exec,                       SH_CMD("poweroff")},
    {RUN_BUTTON,                0, Button1,  enter_and_run_cmd,          {0}},
    {TITLE_LOGO,                0, Button1,  open_client_menu,           {0}},
    {IN_SITU_VERT_MAX_BUTTON,   0, Button1,  maximize_client,            {.max_way=IN_SITU_VERT_MAX}},
    {IN_SITU_HORZ_MAX_BUTTON,   0, Button1,  maximize_client,            {.max_way=IN_SITU_HORZ_MAX}},
    {TOP_MAX_BUTTON,            0, Button1,  maximize_client,            {.max_way=TOP_MAX}},
    {BOTTOM_MAX_BUTTON,         0, Button1,  maximize_client,            {.max_way=BOTTOM_MAX}},
    {LEFT_MAX_BUTTON,           0, Button1,  maximize_client,            {.max_way=LEFT_MAX}},
    {RIGHT_MAX_BUTTON,          0, Button1,  maximize_client,            {.max_way=RIGHT_MAX}},
    {FULL_MAX_BUTTON,           0, Button1,  maximize_client,            {.max_way=FULL_MAX}},
    {ROOT_WIN,                  0, Button1,  adjust_layout_ratio,        {0}},
    {MAIN_BUTTON,               0, Button1,  change_area,                {.area_type=MAIN_AREA}},
    {SECOND_BUTTON,             0, Button1,  change_area,                {.area_type=SECOND_AREA}},
    {FIXED_BUTTON,              0, Button1,  change_area,                {.area_type=FIXED_AREA}},
    {FLOAT_BUTTON,              0, Button1,  change_area,                {.area_type=FLOATING_AREA}},
    {ICON_BUTTON,               0, Button1,  change_area,                {.area_type=ICONIFY_AREA}},
    {MAX_BUTTON,                0, Button1,  maximize_client,            {.max_way=FULL_MAX}},
    {CLOSE_BUTTON,              0, Button1,  close_client,               {0}},
    {TITLE_AREA,                0, Button1,  pointer_move_resize_client, {.resize=false}},
    {TITLE_AREA,                0, Button2,  pointer_change_area,        {0}},
    {TITLE_AREA,                0, Button3,  pointer_swap_clients,       {0}},
    {CLIENT_WIN,                0, Button1,  choose_client,              {0}},
    {CLIENT_WIN,           WM_KEY, Button1,  pointer_move_resize_client, {.resize=false}},
    {CLIENT_WIN,          WM_SKEY, Button1,  pointer_move_resize_client, {.resize=true}},
    {CLIENT_WIN,           WM_KEY, Button2,  pointer_change_area,        {0}},
    {CLIENT_WIN,           WM_KEY, Button3,  pointer_swap_clients,       {0}},
    {CLIENT_WIN,                0, Button3,  choose_client,              {0}},
    {CLIENT_FRAME,              0, Button1,  pointer_move_resize_client, {.resize=true}},
    {CLIENT_ICON,               0, Button1,  change_area,                {.area_type=PREV_AREA}},
    {CLIENT_ICON,               0, Button2,  pointer_change_area,        {0}},
    {CLIENT_ICON,          WM_KEY, Button2,  close_client,               {0}},
    {CLIENT_ICON,               0, Button3,  pointer_swap_clients,       {0}},
    {0} // 哨兵值，表示結束，切勿刪改之
};

/* 功能：設置窗口管理器規則。
 * 說明：Rule的定義詳見gwm.h。可通過xprop命令查看客戶程序類型和客戶程序名稱。其結果表示爲：
 *     WM_CLASS(STRING) = "客戶程序名稱", "客戶程序類型"
 * 一個窗口可以歸屬多個桌面，桌面從1開始編號，桌面n的掩碼計算公式：1<<(n-1)。
 * 譬如，桌面1的掩碼是1<<(1-1)，即1；桌面2的掩碼是1<<(2-1)，即2；1&2即3表示窗口歸屬桌面1和2。
 * 若掩碼爲0，表示窗口歸屬默認桌面。
 */
static const Rule rule[] =
{
    /* 客戶程序類型        客戶程序名稱 客戶程序的類型別名   窗口放置位置(詳gwm.h)  是否顯示標題欄 是否顯示邊框 桌面掩碼 */
    {"Qq",                 "qq",                 "QQ",       FIXED_AREA,            false,         false,        0},
    {"explorer.exe",       "explorer.exe",       NULL,       FLOATING_AREA,         false,         false,        0},
    {"Thunder.exe",        "Thunder.exe",        NULL,       FLOATING_AREA,         true,          true,         0},
    {"Google-chrome",      "google-chrome",      "chrome",   MAIN_AREA,             true,          true,         0},
    {"Org.gnome.Nautilus", "org.gnome.Nautilus", "Nautilus", MAIN_AREA,             true,          true,         0},
    {0} // 哨兵值，表示結束，切勿刪改之
};

/* 功能：設置字體。
 * 說明：字體類型（*_FONT）的定義詳見：gwm.h:Font_type。每增加一種不同的字體，
 * 就會增加2M左右的內存佔用。可通過以下命令查看可用字體：
 *         fc-list -f "%{fullname}\n" :lang=zh | sed 's/,/\n/g' | sort -u
 *     縮放因子爲1.0時，表示正常視力之人所能看清的最小字號（單位爲像素）。
 * 近視之人應按近視程度設置大於1.0的合適值。
 */
static void config_font(WM *wm)
{
    int size=get_scale_font_size(wm, 2.0);

    /*           字體類型       字體系列     字號 */
    SET_FONT(wm, DEFAULT_FONT,  "monospace", size);
    SET_FONT(wm, TITLEBAR_FONT, "monospace", size);
    SET_FONT(wm, MENU_FONT,     "monospace", size);
    SET_FONT(wm, TASKBAR_FONT,  "monospace", size);
    SET_FONT(wm, CLASS_FONT,    "monospace", size);
    SET_FONT(wm, ENTRY_FONT,    "monospace", size);
    SET_FONT(wm, HINT_FONT,     "monospace", size);
}

/* 功能：設置構件尺寸。
 * 說明：建議以字號爲基準來設置構件大小。標識符定義詳見gwm.h。
 */
static void config_widget_size(WM *wm)
{
    Config *c=wm->cfg;
    c->border_width=c->font_size[DEFAULT_FONT]/8.0+0.5;
    c->title_button_width=get_font_height_by_pad(wm, TITLEBAR_FONT);
    c->win_gap=c->border_width*2;
    c->status_area_width_max=c->font_size[TASKBAR_FONT]*30;
    c->taskbar_button_width=get_font_height_by_pad(wm, TASKBAR_FONT)/0.618+0.5;
    c->icon_win_width_max=c->font_size[TASKBAR_FONT]*15;
    c->icon_gap=c->font_size[TASKBAR_FONT]/2.0+0.5;
    c->resize_inc=c->font_size[DEFAULT_FONT];
}

/* 功能：設置與定位器操作類型相對應的光標符號。
 * 說明：定位器操作類型的定義詳見gwm.h:Pointer_act，
 * 光標符號的定義詳見<X11/cursorfont.h>。
 */
static void config_cursor_shape(WM *wm)
{
    unsigned int *cursor_shape=wm->cfg->cursor_shape;

    /*           定位器操作類型         光標符號 */
    cursor_shape[NO_OP]               = XC_left_ptr;
    cursor_shape[CHOOSE]              = XC_hand2;
    cursor_shape[MOVE]                = XC_fleur;
    cursor_shape[SWAP]                = XC_exchange;
    cursor_shape[CHANGE]              = XC_target;
    cursor_shape[TOP_RESIZE]          = XC_top_side;
    cursor_shape[BOTTOM_RESIZE]       = XC_bottom_side;
    cursor_shape[LEFT_RESIZE]         = XC_left_side;
    cursor_shape[RIGHT_RESIZE]        = XC_right_side;
    cursor_shape[TOP_LEFT_RESIZE]     = XC_top_left_corner;
    cursor_shape[TOP_RIGHT_RESIZE]    = XC_top_right_corner;
    cursor_shape[BOTTOM_LEFT_RESIZE]  = XC_bottom_left_corner;
    cursor_shape[BOTTOM_RIGHT_RESIZE] = XC_bottom_right_corner;
    cursor_shape[ADJUST_LAYOUT_RATIO] = XC_sb_h_double_arrow;
}

/* 功能：爲深色主題設置構件背景色。
 * 說明：構件顏色號的定義詳見gwm.h:Widget_color。顏色名詳見rgb.txt（此文件的位
 * 置因系統而異，可用locate rgb.txt搜索），也可以用十六進制顏色說明，格式爲以下
 * 之一（下同）：
 *     #RGB、#RRGGBB、#RRRGGGBBB、#RRRRGGGGBBBB。
 */
static void config_widget_color_for_dark(WM *wm)
{
    const char **color_name=wm->cfg->widget_color_name[DARK_THEME];

    /*         構件顏色號                     顏色名 */
    color_name[NORMAL_BORDER_COLOR]         = "grey11";
    color_name[CURRENT_BORDER_COLOR]        = "grey31";
    color_name[NORMAL_TITLEBAR_COLOR]       = "grey11";
    color_name[CURRENT_TITLEBAR_COLOR]      = "grey31";
    color_name[ENTERED_NORMAL_BUTTON_COLOR] = "DarkOrange";
    color_name[ENTERED_CLOSE_BUTTON_COLOR]  = "red";
    color_name[CHOSEN_BUTTON_COLOR]         = "DeepSkyBlue4";
    color_name[MENU_COLOR]                  = "grey31";
    color_name[TASKBAR_COLOR]               = "grey21";
    color_name[ENTRY_COLOR]                 = "white";
    color_name[HINT_WIN_COLOR]              = "grey81";
    color_name[ROOT_WIN_COLOR]              = "black";
}

/* 功能：爲中性顏色主題設置構件背景色。
 * 說明：同前。
 */
static void config_widget_color_for_normal(WM *wm)
{
    const char **color_name=wm->cfg->widget_color_name[NORMAL_THEME];

    /*         構件顏色號                     顏色名 */
    color_name[NORMAL_BORDER_COLOR]         = "grey31";
    color_name[CURRENT_BORDER_COLOR]        = "DodgerBlue";
    color_name[NORMAL_TITLEBAR_COLOR]       = "grey31";
    color_name[CURRENT_TITLEBAR_COLOR]      = "DodgerBlue";
    color_name[ENTERED_NORMAL_BUTTON_COLOR] = "DarkOrange";
    color_name[ENTERED_CLOSE_BUTTON_COLOR]  = "red";
    color_name[CHOSEN_BUTTON_COLOR]         = "DeepSkyBlue4";
    color_name[MENU_COLOR]                  = "grey31";
    color_name[TASKBAR_COLOR]               = "grey21";
    color_name[ENTRY_COLOR]                 = "white";
    color_name[HINT_WIN_COLOR]              = "grey31";
    color_name[ROOT_WIN_COLOR]              = "black";
}

/* 功能：爲淺色主題設置構件背景色。
 * 說明：同前。
 */
static void config_widget_color_for_light(WM *wm)
{
    const char **color_name=wm->cfg->widget_color_name[LIGHT_THEME];

    /*         構件顏色號                     顏色名 */
    color_name[NORMAL_BORDER_COLOR]         = "grey61";
    color_name[CURRENT_BORDER_COLOR]        = "grey91";
    color_name[NORMAL_TITLEBAR_COLOR]       = "grey61";
    color_name[CURRENT_TITLEBAR_COLOR]      = "grey91";
    color_name[ENTERED_NORMAL_BUTTON_COLOR] = "white";
    color_name[ENTERED_CLOSE_BUTTON_COLOR]  = "red";
    color_name[CHOSEN_BUTTON_COLOR]         = "LightSkyBlue";
    color_name[MENU_COLOR]                  = "grey61";
    color_name[TASKBAR_COLOR]               = "grey81";
    color_name[ENTRY_COLOR]                 = "black";
    color_name[HINT_WIN_COLOR]              = "grey31";
    color_name[ROOT_WIN_COLOR]              = "black";
}

/* 功能：爲深色主題設置構件背景色的不透明度。
 * 說明：構件顏色號的定義詳見gwm.h:Widget_color；不透明度取值範圍爲0~1.0（下同）。
 */
static void config_widget_opacity_for_dark(WM *wm)
{
    float *opacity=wm->cfg->widget_opacity[DARK_THEME];

    wm->cfg->global_opacity=0.8; // 全局不透明度
    for(size_t i=0; i<WIDGET_COLOR_N; i++)
        opacity[i]=wm->cfg->global_opacity;

    /* 不使用全局透明度的構件分別設置自己的不透明度：
     *      構件顏色號        不透明度               */
    opacity[TASKBAR_COLOR]  = 0.5;
    opacity[HINT_WIN_COLOR] = 0.9;
    opacity[ROOT_WIN_COLOR] = 1.0;
}

/* 功能：爲中性顏色主題設置構件背景不透明度。
 * 說明：同前。
 */
static void config_widget_opacity_for_normal(WM *wm)
{
    float *opacity=wm->cfg->widget_opacity[NORMAL_THEME];

    wm->cfg->global_opacity = 0.8; // 全局不透明度;
    for(size_t i=0; i<WIDGET_COLOR_N; i++)
        opacity[i]=wm->cfg->global_opacity;

    /* 不使用全局透明度的構件分別設置自己的不透明度：
     *      構件顏色號        不透明度               */
    opacity[TASKBAR_COLOR]  = 0.5;
    opacity[HINT_WIN_COLOR] = 0.9;
    opacity[ROOT_WIN_COLOR] = 1.0;
}

/* 功能：爲淺色主題設置構件背景不透明度。
 * 說明：同前。
 */
static void config_widget_opacity_for_light(WM *wm)
{
    float *opacity=wm->cfg->widget_opacity[LIGHT_THEME];

    wm->cfg->global_opacity = 0.8; // 全局不透明度
    for(size_t i=0; i<WIDGET_COLOR_N; i++)
        opacity[i]=wm->cfg->global_opacity;

    /* 不使用全局透明度的構件分別設置自己的不透明度：
     *      構件顏色號        不透明度               */
    opacity[TASKBAR_COLOR]  = 0.5;
    opacity[HINT_WIN_COLOR] = 0.9;
    opacity[ROOT_WIN_COLOR] = 1.0;
}

/* 功能：爲各種主題設置構件背景顏色及不透明度。*/
static void config_widget_color_and_opacity(WM *wm)
{
    config_widget_color_for_dark(wm);
    config_widget_color_for_normal(wm);
    config_widget_color_for_light(wm);

    config_widget_opacity_for_dark(wm);
    config_widget_opacity_for_normal(wm);
    config_widget_opacity_for_light(wm);
}

/* 功能：爲深色主題設置文字顏色。
 * 說明：文字顏色號的定義詳見gwm.h:Text_color。顏色名說明同前。
 */
static void config_text_color_for_dark(WM *wm)
{
    const char **color_name=wm->cfg->text_color_name[DARK_THEME];

    /*         文字顏色號                     顏色名 */
    color_name[NORMAL_TITLEBAR_TEXT_COLOR]  = "grey71";
    color_name[CURRENT_TITLEBAR_TEXT_COLOR] = "LightGreen";
    color_name[TASKBAR_TEXT_COLOR]          = "white";
    color_name[CLASS_TEXT_COLOR]            = "RosyBrown";
    color_name[MENU_TEXT_COLOR]             = "white";
    color_name[ENTRY_TEXT_COLOR]            = "black";
    color_name[HINT_TEXT_COLOR]             = "SkyBlue4";
}

/* 功能：爲默認顏色主題設置文字顏色。
 * 說明：同前。
 */
static void config_text_color_for_normal(WM *wm)
{
    const char **color_name=wm->cfg->text_color_name[NORMAL_THEME];

    /*         文字顏色號                     顏色名 */
    color_name[NORMAL_TITLEBAR_TEXT_COLOR]  = "grey71";
    color_name[CURRENT_TITLEBAR_TEXT_COLOR] = "white";
    color_name[TASKBAR_TEXT_COLOR]          = "white";
    color_name[CLASS_TEXT_COLOR]            = "RosyBrown";
    color_name[MENU_TEXT_COLOR]             = "white";
    color_name[ENTRY_TEXT_COLOR]            = "black";
    color_name[HINT_TEXT_COLOR]             = "grey61";
}

/* 功能：爲淺色主題設置文字顏色。
 * 說明：同前。
 */
static void config_text_color_for_light(WM *wm)
{
    const char **color_name=wm->cfg->text_color_name[LIGHT_THEME];

    /*         文字顏色號                     顏色名 */
    color_name[NORMAL_TITLEBAR_TEXT_COLOR]  = "grey31";
    color_name[CURRENT_TITLEBAR_TEXT_COLOR] = "black";
    color_name[TASKBAR_TEXT_COLOR]          = "black";
    color_name[CLASS_TEXT_COLOR]            = "RosyBrown";
    color_name[MENU_TEXT_COLOR]             = "black";
    color_name[ENTRY_TEXT_COLOR]            = "white";
    color_name[HINT_TEXT_COLOR]             = "grey61";
}

/* 功能：爲各種主題設置文字顏色。*/
static void config_text_color(WM *wm)
{
    config_text_color_for_dark(wm);
    config_text_color_for_normal(wm);
    config_text_color_for_light(wm);
}

/* 功能：設置標題按鈕的文字。
 * 說明：標題欄按鈕類型的定義詳見gwm.h:Widget_type。
 */
static void config_title_button_text(WM *wm)
{
    /*                        標題欄按鈕類型 按鈕文字 */
    SET_TITLE_BUTTON_TEXT(wm, SECOND_BUTTON, "◁");
    SET_TITLE_BUTTON_TEXT(wm, MAIN_BUTTON,   "▼");
    SET_TITLE_BUTTON_TEXT(wm, FIXED_BUTTON,  "▷");
    SET_TITLE_BUTTON_TEXT(wm, FLOAT_BUTTON,  "△");
    SET_TITLE_BUTTON_TEXT(wm, ICON_BUTTON,   "—");
    SET_TITLE_BUTTON_TEXT(wm, MAX_BUTTON,    "□");
    SET_TITLE_BUTTON_TEXT(wm, CLOSE_BUTTON,  "×");
}

/* 功能：設置任務欄按鈕的文字。
 * 說明：任務欄按鈕類型的定義詳見gwm.h:Widget_type。
 */
static void config_taskbar_button_text(WM *wm)
{
    /*                          任務欄按鈕類型   按鈕文字 */
    SET_TASKBAR_BUTTON_TEXT(wm, DESKTOP1_BUTTON, "1");
    SET_TASKBAR_BUTTON_TEXT(wm, DESKTOP2_BUTTON, "2");
    SET_TASKBAR_BUTTON_TEXT(wm, DESKTOP3_BUTTON, "3");
    SET_TASKBAR_BUTTON_TEXT(wm, FULL_BUTTON,     "□");
    SET_TASKBAR_BUTTON_TEXT(wm, PREVIEW_BUTTON,  "▦");
    SET_TASKBAR_BUTTON_TEXT(wm, STACK_BUTTON,    "▣");
    SET_TASKBAR_BUTTON_TEXT(wm, TILE_BUTTON,     "▥");
    SET_TASKBAR_BUTTON_TEXT(wm, DESKTOP_BUTTON,  "■");
    SET_TASKBAR_BUTTON_TEXT(wm, ACT_CENTER_ITEM, "^");
}

/* 功能：設置操作中心的文字。
 * 說明：操作中心按鈕類型的定義詳見gwm.h:Widget_type。
 */
static void config_act_center_item_text(WM *wm)
{
    /*                           操作中心按鈕類型             按鈕文字 */
    SET_ACT_CENTER_ITEM_TEXT(wm, HELP_BUTTON,              _("幫助"));
    SET_ACT_CENTER_ITEM_TEXT(wm, FILE_BUTTON,              _("文件"));
    SET_ACT_CENTER_ITEM_TEXT(wm, TERM_BUTTON,              _("終端模擬器"));
    SET_ACT_CENTER_ITEM_TEXT(wm, BROWSER_BUTTON,           _("網絡瀏覽器"));

    SET_ACT_CENTER_ITEM_TEXT(wm, PLAY_START_BUTTON,        _("播放影音"));
    SET_ACT_CENTER_ITEM_TEXT(wm, PLAY_TOGGLE_BUTTON,       _("切換播放狀態"));
    SET_ACT_CENTER_ITEM_TEXT(wm, PLAY_QUIT_BUTTON,         _("關閉影音"));
    SET_ACT_CENTER_ITEM_TEXT(wm, VOLUME_DOWN_BUTTON,       _("减小音量"));

    SET_ACT_CENTER_ITEM_TEXT(wm, VOLUME_UP_BUTTON,         _("增大音量"));
    SET_ACT_CENTER_ITEM_TEXT(wm, VOLUME_MAX_BUTTON,        _("最大音量"));
    SET_ACT_CENTER_ITEM_TEXT(wm, VOLUME_TOGGLE_BUTTON,     _("靜音切換"));
    SET_ACT_CENTER_ITEM_TEXT(wm, MAIN_NEW_BUTTON,          _("暫主區開窗"));

    SET_ACT_CENTER_ITEM_TEXT(wm, SEC_NEW_BUTTON,           _("暫次區開窗"));
    SET_ACT_CENTER_ITEM_TEXT(wm, FIX_NEW_BUTTON,           _("暫固定區開窗"));
    SET_ACT_CENTER_ITEM_TEXT(wm, FLOAT_NEW_BUTTON,         _("暫懸浮區開窗"));
    SET_ACT_CENTER_ITEM_TEXT(wm, ICON_NEW_BUTTON,          _("暫縮微區開窗"));

    SET_ACT_CENTER_ITEM_TEXT(wm, N_MAIN_UP_BUTTON,         _("增大主區容量"));
    SET_ACT_CENTER_ITEM_TEXT(wm, N_MAIN_DOWN_BUTTON,       _("减小主區容量"));
    SET_ACT_CENTER_ITEM_TEXT(wm, TITLEBAR_TOGGLE_BUTTON,   _("開關當前窗口標題欄"));
    SET_ACT_CENTER_ITEM_TEXT(wm, CLI_BORDER_TOGGLE_BUTTON, _("開關當前窗口邊框"));
    SET_ACT_CENTER_ITEM_TEXT(wm, CLOSE_ALL_CLIENTS_BUTTON, _("關閉桌面所有窗口"));
    SET_ACT_CENTER_ITEM_TEXT(wm, PRINT_WIN_BUTTON,         _("當前窗口截圖"));
    SET_ACT_CENTER_ITEM_TEXT(wm, PRINT_SCREEN_BUTTON,      _("全屏截圖"));
    SET_ACT_CENTER_ITEM_TEXT(wm, FOCUS_MODE_BUTTON,        _("切換聚焦模式"));
    SET_ACT_CENTER_ITEM_TEXT(wm, COMPOSITOR_BUTTON,        _("開關合成器"));
    SET_ACT_CENTER_ITEM_TEXT(wm, WALLPAPER_BUTTON,         _("切換壁紙"));
    SET_ACT_CENTER_ITEM_TEXT(wm, COLOR_THEME_BUTTON,       _("切換顏色主題"));
    SET_ACT_CENTER_ITEM_TEXT(wm, QUIT_WM_BUTTON,           _("退出gwm"));

    SET_ACT_CENTER_ITEM_TEXT(wm, LOGOUT_BUTTON,            _("注銷"));
    SET_ACT_CENTER_ITEM_TEXT(wm, REBOOT_BUTTON,            _("重啓"));
    SET_ACT_CENTER_ITEM_TEXT(wm, POWEROFF_BUTTON,          _("關機"));
    SET_ACT_CENTER_ITEM_TEXT(wm, RUN_BUTTON,               _("運行"));
}

/* 功能：設置客戶窗口菜單的文字。
 * 說明：客戶窗口菜單項類型的定義詳見gwm.h:Widget_type。
 */
static void config_client_menu_item_text(WM *wm)
{
    /*                            客戶窗口菜單項類型          按鈕文字 */
    SET_CLIENT_MENU_ITEM_TEXT(wm, IN_SITU_VERT_MAX_BUTTON,  _("原位縱向最大化"));
    SET_CLIENT_MENU_ITEM_TEXT(wm, IN_SITU_HORZ_MAX_BUTTON,  _("原位橫向最大化"));
    SET_CLIENT_MENU_ITEM_TEXT(wm, TOP_MAX_BUTTON,           _("最大化至上半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(wm, BOTTOM_MAX_BUTTON,        _("最大化至下半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(wm, LEFT_MAX_BUTTON,          _("最大化至左半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(wm, RIGHT_MAX_BUTTON,         _("最大化至右半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(wm, FULL_MAX_BUTTON,          _("完全最大化"));
}

/* 功能：設置構件功能提示。
 * 說明：構件類型的定義詳見gwm.h:Widget_type。以下未列出的構件要麼不必顯示提示，
 * 要麼動態變化而不可在此設置。
 */
static void config_tooltip(WM *wm)
{
    const char **tooltip=wm->cfg->tooltip;

    /*      構件類型             構件功能提示文字 */
    tooltip[SECOND_BUTTON]   = _("切換到次要區域");
    tooltip[MAIN_BUTTON]     = _("切換到主要區域");
    tooltip[FIXED_BUTTON]    = _("切換到固定區域");
    tooltip[FLOAT_BUTTON]    = _("切換到懸浮區域");
    tooltip[ICON_BUTTON]     = _("切換到圖符區域");
    tooltip[MAX_BUTTON]      = _("切換到懸浮區域並最大化窗口");
    tooltip[CLOSE_BUTTON]    = _("關閉窗口");
    tooltip[DESKTOP1_BUTTON] = _("切換到虛擬桌面1");
    tooltip[DESKTOP2_BUTTON] = _("切換到虛擬桌面2");
    tooltip[DESKTOP3_BUTTON] = _("切換到虛擬桌面3");
    tooltip[FULL_BUTTON]     = _("切換到全屏模式");
    tooltip[PREVIEW_BUTTON]  = _("切換到預覽模式");
    tooltip[STACK_BUTTON]    = _("切換到堆疊模式");
    tooltip[TILE_BUTTON]     = _("切換到平鋪模式");
    tooltip[DESKTOP_BUTTON]  = _("顯示桌面");
    tooltip[ACT_CENTER_ITEM] = _("打開操作中心");
    tooltip[TITLE_LOGO]      = _("打開窗口菜單");
}

/* 功能：設置其他雜項。
 * 說明：標識符含義詳見config.h。
 */
static void config_misc(WM *wm)
{
    Config *c=wm->cfg;
    c->set_frame_prop=false;
    c->show_taskbar=true;
    c->taskbar_on_top=false;
    c->focus_mode=CLICK_FOCUS;
    c->default_layout=TILE;
    c->default_area_type=MAIN_AREA;
    c->color_theme=DARK_THEME;
    c->screen_saver_time_out=600;
    c->screen_saver_interval=600;
    c->hover_time=300;
    c->default_cur_desktop=1;
    c->default_n_main_max=1;
    c->act_center_col=4;
    c->font_pad_ratio=0.25;
    c->default_main_area_ratio=0.6;
    c->default_fixed_area_ratio=0.15;
    c->autostart="~/.config/gwm/autostart.sh";
    c->cur_icon_theme="default";
    c->screenshot_path="~";
    c->screenshot_format="png";
    c->wallpaper_paths="/usr/share/backgrounds/fedora-workstation:/usr/share/wallpapers";
    c->wallpaper_filename="/usr/share/backgrounds/gwm.png";
    c->run_cmd_entry_hint=_("請輸入命令，然後按回車執行");
    c->compositor="picom";
    c->keybind=keybind;
    c->buttonbind=buttonbind;
    c->rule=rule;
}

/* =========================== 用戶配置項結束 =========================== */ 

void config(WM *wm)
{
    wm->cfg=malloc_s(sizeof(Config));
    SET_NULL(wm->cfg->tooltip, WIDGET_N);

    config_misc(wm);
    config_font(wm);
    config_widget_size(wm);
    config_cursor_shape(wm);
    config_widget_color_and_opacity(wm);
    config_text_color(wm);
    config_title_button_text(wm);
    config_taskbar_button_text(wm);
    config_act_center_item_text(wm);
    config_client_menu_item_text(wm);
    config_tooltip(wm);
}
