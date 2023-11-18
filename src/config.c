/* *************************************************************************
 *     config.c：用戶配置gwm。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "wm_cfg.h"

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
    {WM_KEY,        XK_F1,        change_place,                {.place_type=TILE_LAYER_MAIN}},
    {WM_KEY,        XK_F2,        change_place,                {.place_type=TILE_LAYER_SECOND}},
    {WM_KEY,        XK_F3,        change_place,                {.place_type=TILE_LAYER_FIXED}},
    {WM_KEY,        XK_F4,        change_place,                {.place_type=FLOAT_LAYER}},
    {WM_KEY,        XK_Return,    choose_client,               {0}},
    {WM_KEY,        XK_Tab,       next_client,                 {0}},
    {WM_SKEY,       XK_Tab,       prev_client,                 {0}},
    {WM_KEY,        XK_c,         close_client,                {0}},
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
    {WM_KEY,        XK_r,         enter_and_run_cmd,           {0}},
    {WM_KEY,        XK_Delete,    quit_wm,                     {0}},
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
    WM_BUTTONBIND,
    DESKTOP_BUTTONBIND(1), 
    DESKTOP_BUTTONBIND(2), 
    DESKTOP_BUTTONBIND(3), 

    /* 構件類型        功能轉換鍵 定位器按鈕 要綁定的函數                函數的參數 */
    {DESKTOP_BUTTON,       WM_KEY, Button2,  close_all_clients,          {0}},
    {CLIENT_WIN,           WM_KEY, Button1,  pointer_move_resize_client, {.resize=false}},
    {CLIENT_WIN,          WM_SKEY, Button1,  pointer_move_resize_client, {.resize=true}},
    {CLIENT_WIN,           WM_KEY, Button2,  pointer_change_place,       {0}},
    {CLIENT_WIN,           WM_KEY, Button3,  pointer_swap_clients,       {0}},
    {CLIENT_WIN,                0, Button3,  choose_client,              {0}},
    {CLIENT_ICON,               0, Button2,  pointer_change_place,       {0}},
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
    {"QQ",                 "qq",                 "QQ",       FLOAT_LAYER,           false,         false,        0},
    {"explorer.exe",       "explorer.exe",       NULL,       FLOAT_LAYER,           false,         false,        0},
    {"Thunder.exe",        "Thunder.exe",        NULL,       FLOAT_LAYER,           true,          true,         0},
    {"Google-chrome",      "google-chrome",      "chrome",   TILE_LAYER_MAIN,       true,          true,         0},
    {"Org.gnome.Nautilus", "org.gnome.Nautilus", "Nautilus", TILE_LAYER_MAIN,       true,          true,         0},
    {0} // 哨兵值，表示結束，切勿刪改之
};

/* 功能：設置字體。
 * 說明：縮放因子爲1.0時，表示正常視力之人所能看清的最小字號（單位爲像素）。
 * 近視之人應按近視程度設置大於1.0的合適值。當font_name等於NULL或""或"*"時，
 * 使用系統默認字體。可通過以下命令查看可用字體：
 *     fc-list -f "%{fullname}\n" :lang=zh | sed 's/,/\n/g' | sort -u
 */
static void config_font(void)
{
    cfg->font_size=get_scale_font_size(2.0);
    cfg->font_name="monospace";
}

/* 功能：設置構件尺寸。
 * 說明：建議以字號爲基準來設置構件大小。標識符定義詳見gwm.h。
 */
static void config_widget_size(void)
{
    cfg->border_width=cfg->font_size/8.0+0.5;
    cfg->title_button_width=get_font_height_by_pad();
    cfg->win_gap=cfg->border_width*2;
    cfg->status_area_width_max=cfg->font_size*30;
    cfg->taskbar_button_width=get_font_height_by_pad()/0.618+0.5;
    cfg->icon_win_width_max=cfg->font_size*15;
    cfg->icon_image_size=cfg->font_size*15;
    cfg->icon_gap=cfg->font_size/2.0+0.5;
    cfg->resize_inc=cfg->font_size;
}

/* 功能：設置與定位器操作類型相對應的光標符號。
 * 說明：定位器操作類型的定義詳見gwm.h:Pointer_act，
 * 光標符號的定義詳見<X11/cursorfont.h>。
 */
static void config_cursor_shape(void)
{
    unsigned int *cursor_shape=cfg->cursor_shape;

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
static void config_widget_color_for_dark(void)
{
    const char **color_name=cfg->widget_color_name[DARK_THEME];

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
    color_name[URGENCY_WIDGET_COLOR]        = "red";
    color_name[ATTENTION_WIDGET_COLOR]      = "Yellow4";
    color_name[ROOT_WIN_COLOR]              = "black";
}

/* 功能：爲中性顏色主題設置構件背景色。
 * 說明：同前。
 */
static void config_widget_color_for_normal(void)
{
    const char **color_name=cfg->widget_color_name[NORMAL_THEME];

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
    color_name[URGENCY_WIDGET_COLOR]        = "red";
    color_name[ATTENTION_WIDGET_COLOR]      = "Yellow4";
    color_name[ROOT_WIN_COLOR]              = "black";
}

/* 功能：爲淺色主題設置構件背景色。
 * 說明：同前。
 */
static void config_widget_color_for_light(void)
{
    const char **color_name=cfg->widget_color_name[LIGHT_THEME];

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
    color_name[URGENCY_WIDGET_COLOR]        = "red";
    color_name[ATTENTION_WIDGET_COLOR]      = "yellow2";
    color_name[ROOT_WIN_COLOR]              = "black";
}

/* 功能：爲深色主題設置構件背景色的不透明度。
 * 說明：構件顏色號的定義詳見gwm.h:Widget_color；不透明度取值範圍爲0~1.0（下同）。
 */
static void config_widget_opacity_for_dark(void)
{
    float *opacity=cfg->widget_opacity[DARK_THEME];

    cfg->global_opacity=0.8; // 全局不透明度
    for(size_t i=0; i<WIDGET_COLOR_N; i++)
        opacity[i]=cfg->global_opacity;

    /* 不使用全局透明度的構件分別設置自己的不透明度：
     *      構件顏色號        不透明度               */
    opacity[TASKBAR_COLOR]  = 0.5;
    opacity[HINT_WIN_COLOR] = 0.9;
    opacity[ROOT_WIN_COLOR] = 1.0;
}

/* 功能：爲中性顏色主題設置構件背景不透明度。
 * 說明：同前。
 */
static void config_widget_opacity_for_normal(void)
{
    float *opacity=cfg->widget_opacity[NORMAL_THEME];

    cfg->global_opacity = 0.8; // 全局不透明度;
    for(size_t i=0; i<WIDGET_COLOR_N; i++)
        opacity[i]=cfg->global_opacity;

    /* 不使用全局透明度的構件分別設置自己的不透明度：
     *      構件顏色號        不透明度               */
    opacity[TASKBAR_COLOR]  = 0.5;
    opacity[HINT_WIN_COLOR] = 0.9;
    opacity[ROOT_WIN_COLOR] = 1.0;
}

/* 功能：爲淺色主題設置構件背景不透明度。
 * 說明：同前。
 */
static void config_widget_opacity_for_light(void)
{
    float *opacity=cfg->widget_opacity[LIGHT_THEME];

    cfg->global_opacity = 0.8; // 全局不透明度
    for(size_t i=0; i<WIDGET_COLOR_N; i++)
        opacity[i]=cfg->global_opacity;

    /* 不使用全局透明度的構件分別設置自己的不透明度：
     *      構件顏色號        不透明度               */
    opacity[TASKBAR_COLOR]  = 0.5;
    opacity[HINT_WIN_COLOR] = 0.9;
    opacity[ROOT_WIN_COLOR] = 1.0;
}

/* 功能：爲各種主題設置構件背景顏色及不透明度。*/
static void config_widget_color_and_opacity(void)
{
    config_widget_color_for_dark();
    config_widget_color_for_normal();
    config_widget_color_for_light();

    config_widget_opacity_for_dark();
    config_widget_opacity_for_normal();
    config_widget_opacity_for_light();
}

/* 功能：爲深色主題設置文字顏色。
 * 說明：文字顏色號的定義詳見gwm.h:Text_color。顏色名說明同前。
 */
static void config_text_color_for_dark(void)
{
    const char **color_name=cfg->text_color_name[DARK_THEME];

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
static void config_text_color_for_normal(void)
{
    const char **color_name=cfg->text_color_name[NORMAL_THEME];

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
static void config_text_color_for_light(void)
{
    const char **color_name=cfg->text_color_name[LIGHT_THEME];

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
static void config_text_color(void)
{
    config_text_color_for_dark();
    config_text_color_for_normal();
    config_text_color_for_light();
}

/* 功能：設置標題按鈕的文字。
 * 說明：標題欄按鈕類型的定義詳見gwm.h:Widget_type。
 */
static void config_title_button_text(void)
{
    /*                    標題欄按鈕類型 按鈕文字 */
    SET_TITLE_BUTTON_TEXT(SECOND_BUTTON, "◁");
    SET_TITLE_BUTTON_TEXT(MAIN_BUTTON,   "▼");
    SET_TITLE_BUTTON_TEXT(FIXED_BUTTON,  "▷");
    SET_TITLE_BUTTON_TEXT(FLOAT_BUTTON,  "△");
    SET_TITLE_BUTTON_TEXT(ICON_BUTTON,   "—");
    SET_TITLE_BUTTON_TEXT(MAX_BUTTON,    "□");
    SET_TITLE_BUTTON_TEXT(CLOSE_BUTTON,  "×");
}

/* 功能：設置任務欄按鈕的文字。
 * 說明：任務欄按鈕類型的定義詳見gwm.h:Widget_type。
 */
static void config_taskbar_button_text(void)
{
    /*                      任務欄按鈕類型   按鈕文字 */
    SET_TASKBAR_BUTTON_TEXT(DESKTOP1_BUTTON, "1");
    SET_TASKBAR_BUTTON_TEXT(DESKTOP2_BUTTON, "2");
    SET_TASKBAR_BUTTON_TEXT(DESKTOP3_BUTTON, "3");
    SET_TASKBAR_BUTTON_TEXT(PREVIEW_BUTTON,  "▦");
    SET_TASKBAR_BUTTON_TEXT(STACK_BUTTON,    "▣");
    SET_TASKBAR_BUTTON_TEXT(TILE_BUTTON,     "▥");
    SET_TASKBAR_BUTTON_TEXT(DESKTOP_BUTTON,  "■");
    SET_TASKBAR_BUTTON_TEXT(ACT_CENTER_ITEM, "^");
}

/* 功能：設置操作中心的文字。
 * 說明：操作中心按鈕類型的定義詳見gwm.h:Widget_type。
 */
static void config_act_center_item_text(void)
{
    /*                       操作中心按鈕類型             按鈕文字 */
    SET_ACT_CENTER_ITEM_TEXT(HELP_BUTTON,              _("幫助"));
    SET_ACT_CENTER_ITEM_TEXT(FILE_BUTTON,              _("文件"));
    SET_ACT_CENTER_ITEM_TEXT(TERM_BUTTON,              _("終端模擬器"));
    SET_ACT_CENTER_ITEM_TEXT(BROWSER_BUTTON,           _("網絡瀏覽器"));

    SET_ACT_CENTER_ITEM_TEXT(GAME_BUTTON,              _("遊戲"));
    SET_ACT_CENTER_ITEM_TEXT(PLAY_START_BUTTON,        _("播放影音"));
    SET_ACT_CENTER_ITEM_TEXT(PLAY_TOGGLE_BUTTON,       _("切換播放狀態"));
    SET_ACT_CENTER_ITEM_TEXT(PLAY_QUIT_BUTTON,         _("關閉影音"));

    SET_ACT_CENTER_ITEM_TEXT(VOLUME_DOWN_BUTTON,       _("减小音量"));
    SET_ACT_CENTER_ITEM_TEXT(VOLUME_UP_BUTTON,         _("增大音量"));
    SET_ACT_CENTER_ITEM_TEXT(VOLUME_MAX_BUTTON,        _("最大音量"));
    SET_ACT_CENTER_ITEM_TEXT(VOLUME_TOGGLE_BUTTON,     _("靜音切換"));

    SET_ACT_CENTER_ITEM_TEXT(MAIN_NEW_BUTTON,          _("暫主區開窗"));
    SET_ACT_CENTER_ITEM_TEXT(SEC_NEW_BUTTON,           _("暫次區開窗"));
    SET_ACT_CENTER_ITEM_TEXT(FIX_NEW_BUTTON,           _("暫固定區開窗"));
    SET_ACT_CENTER_ITEM_TEXT(FLOAT_NEW_BUTTON,         _("暫懸浮層開窗"));

    SET_ACT_CENTER_ITEM_TEXT(N_MAIN_UP_BUTTON,         _("增大主區容量"));
    SET_ACT_CENTER_ITEM_TEXT(N_MAIN_DOWN_BUTTON,       _("减小主區容量"));
    SET_ACT_CENTER_ITEM_TEXT(TITLEBAR_TOGGLE_BUTTON,   _("開關當前窗口標題欄"));
    SET_ACT_CENTER_ITEM_TEXT(CLI_BORDER_TOGGLE_BUTTON, _("開關當前窗口邊框"));

    SET_ACT_CENTER_ITEM_TEXT(CLOSE_ALL_CLIENTS_BUTTON, _("關閉桌面所有窗口"));
    SET_ACT_CENTER_ITEM_TEXT(PRINT_WIN_BUTTON,         _("當前窗口截圖"));
    SET_ACT_CENTER_ITEM_TEXT(PRINT_SCREEN_BUTTON,      _("全屏截圖"));
    SET_ACT_CENTER_ITEM_TEXT(FOCUS_MODE_BUTTON,        _("切換聚焦模式"));

    SET_ACT_CENTER_ITEM_TEXT(COMPOSITOR_BUTTON,        _("開關合成器"));
    SET_ACT_CENTER_ITEM_TEXT(WALLPAPER_BUTTON,         _("切換壁紙"));
    SET_ACT_CENTER_ITEM_TEXT(COLOR_THEME_BUTTON,       _("切換顏色主題"));
    SET_ACT_CENTER_ITEM_TEXT(QUIT_WM_BUTTON,           _("退出gwm"));

    SET_ACT_CENTER_ITEM_TEXT(LOGOUT_BUTTON,            _("注銷"));
    SET_ACT_CENTER_ITEM_TEXT(REBOOT_BUTTON,            _("重啓"));
    SET_ACT_CENTER_ITEM_TEXT(POWEROFF_BUTTON,          _("關機"));
    SET_ACT_CENTER_ITEM_TEXT(RUN_BUTTON,               _("運行"));
}

/* 功能：設置客戶窗口菜單的文字。
 * 說明：客戶窗口菜單項類型的定義詳見gwm.h:Widget_type。
 */
static void config_client_menu_item_text(void)
{
    /*                        客戶窗口菜單項類型       按鈕文字 */
    SET_CLIENT_MENU_ITEM_TEXT(SHADE_BUTTON,         _("卷起/放下"));
    SET_CLIENT_MENU_ITEM_TEXT(VERT_MAX_BUTTON,      _("縱向最大化"));
    SET_CLIENT_MENU_ITEM_TEXT(HORZ_MAX_BUTTON,      _("橫向最大化"));
    SET_CLIENT_MENU_ITEM_TEXT(TOP_MAX_BUTTON,       _("最大化至上半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(BOTTOM_MAX_BUTTON,    _("最大化至下半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(LEFT_MAX_BUTTON,      _("最大化至左半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(RIGHT_MAX_BUTTON,     _("最大化至右半屏"));
    SET_CLIENT_MENU_ITEM_TEXT(FULL_MAX_BUTTON,      _("完全最大化"));
}

/* 功能：設置構件功能提示。
 * 說明：構件類型的定義詳見gwm.h:Widget_type。以下未列出的構件要麼不必顯示提示，
 * 要麼動態變化而不可在此設置。
 */
static void config_tooltip(void)
{
    const char **tooltip=cfg->tooltip;

    /*      構件類型             構件功能提示文字 */
    tooltip[SECOND_BUTTON]   = _("切換到次要區域");
    tooltip[MAIN_BUTTON]     = _("切換到主要區域");
    tooltip[FIXED_BUTTON]    = _("切換到固定區域");
    tooltip[FLOAT_BUTTON]    = _("切換到懸浮層");
    tooltip[ICON_BUTTON]     = _("切換到圖符區域");
    tooltip[MAX_BUTTON]      = _("最大化/還原窗口");
    tooltip[CLOSE_BUTTON]    = _("關閉窗口");
    tooltip[DESKTOP1_BUTTON] = _("切換到虛擬桌面1");
    tooltip[DESKTOP2_BUTTON] = _("切換到虛擬桌面2");
    tooltip[DESKTOP3_BUTTON] = _("切換到虛擬桌面3");
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
static void config_misc(void)
{
    cfg->set_frame_prop=false;
    cfg->show_taskbar=true;
    cfg->taskbar_on_top=false;
    cfg->focus_mode=CLICK_FOCUS;
    cfg->default_layout=TILE;
    cfg->color_theme=DARK_THEME;
    cfg->screen_saver_time_out=600;
    cfg->screen_saver_interval=600;
    cfg->hover_time=300;
    cfg->default_cur_desktop=1;
    cfg->default_n_main_max=1;
    cfg->act_center_col=4;
    cfg->font_pad_ratio=0.25;
    cfg->default_main_area_ratio=0.6;
    cfg->default_fixed_area_ratio=0.15;
    cfg->autostart="~/.config/gwm/autostart.sh";
    cfg->cur_icon_theme="default";
    cfg->screenshot_path="~";
    cfg->screenshot_format="png";
    cfg->wallpaper_paths="/usr/share/backgrounds/fedora-workstation:/usr/share/wallpapers";
    cfg->wallpaper_filename="/usr/share/backgrounds/gwm.png";
    cfg->run_cmd_entry_hint=_("請輸入命令，然後按回車執行");
    cfg->compositor="picom";
    cfg->keybind=keybind;
    cfg->buttonbind=buttonbind;
    cfg->rule=rule;
}

/* =========================== 用戶配置項結束 =========================== */ 

void config(void)
{
    cfg=malloc_s(sizeof(Config));
    SET_NULL(cfg->tooltip, WIDGET_N);

    config_misc();
    config_font();
    config_widget_size();
    config_cursor_shape();
    config_widget_color_and_opacity();
    config_text_color();
    config_title_button_text();
    config_taskbar_button_text();
    config_act_center_item_text();
    config_client_menu_item_text();
    config_tooltip();
}
