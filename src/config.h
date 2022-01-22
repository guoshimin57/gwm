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

#define WM_KEY Mod4Mask // 窗口管理器的基本功能轉換鍵
#define WM_SKEY (Mod4Mask|ShiftMask) // 與WM_KEY功能相反的功能轉換鍵
#define CMD_KEY (Mod4Mask|Mod1Mask) // 與系統命令相關的功能轉換鍵
#define SYS_KEY (Mod4Mask|ControlMask) // 與系統相關的功能轉換鍵

#define DEFAULT_FOCUS_MODE CLICK_FOCUS // 默認的聚焦模式
#define DEFAULT_LAYOUT TILE // 默認的窗口布局模式
#define DEFAULT_AREA_TYPE MAIN_AREA // 新打開的窗口的默認區域類型
#define DEFAULT_MAIN_AREA_RATIO 0.6 // 默認的主區域比例
#define DEFAULT_FIXED_AREA_RATIO 0.15 // 默認的固定區域比例
#define DEFAULT_N_MAIN_MAX 1 // 默認的主區域最大窗口數量
#define AUTOSTART "~/.config/gwm/autostart.sh" // 在gwm剛啓動時執行的腳本
#define FONT_SET "*-24-*" // 本窗口管理器所使用的字符集

#define GREY11 0x1c1c1c         // 灰色的一種
#define GREY21 0x363636         // 灰色的一種
#define GREY31 0x4f4f4f         // 灰色的一種
#define DODGERBLUE 0x1e90ff     // 道奇藍
#define RED 0xff0000            // 紅色
#define WHITE 0xffffff          // 白色
#define ROSYBROWN 0xbc8f8f      // 玫瑰褐

#define NORMAL_FRAME_COLOR GREY31 // 普通窗口框架的顏色
#define CURRENT_FRAME_COLOR DODGERBLUE // 當前窗口框架的顏色
#define NORMAL_BORDER_COLOR GREY31 // 普通窗口边框的顏色
#define CURRENT_BORDER_COLOR DODGERBLUE// 當前窗口边框的顏色

#define NORMAL_TITLE_AREA_COLOR GREY31 // 普通窗口標題區域的顏色
#define CURRENT_TITLE_AREA_COLOR DODGERBLUE// 當前窗口標題區域的顏色
#define NORMAL_TITLE_BUTTON_COLOR GREY31 // 普通窗口標題欄按鈕的顏色
#define CURRENT_TITLE_BUTTON_COLOR DODGERBLUE // 當前窗口標題欄按鈕的顏色
#define ENTERED_TITLE_BUTTON_COLOR (~DODGERBLUE) // 定位器進入窗口標題欄按鈕時按鈕的顏色
#define ENTERED_CLOSE_BUTTON_COLOR RED // 定位器進入窗口標題欄關閉按鈕時按鈕的顏色
#define TITLE_TEXT_COLOR WHITE // 窗口標題欄文字的顏色
#define TITLE_BUTTON_TEXT_COLOR WHITE // 窗口標題欄按鈕文字的顏色

#define NORMAL_ICON_BORDER_COLOR GREY11 // 普通縮微窗口边框的顏色
#define CURRENT_ICON_BORDER_COLOR DODGERBLUE// 當前縮微窗口边框的顏色
#define ICON_BG_COLOR GREY11 // 縮微化窗口的背景色
#define ICON_CLASS_NAME_FG_COLOR ROSYBROWN // 縮微化窗口類型名文字的前景色
#define ICON_CLASS_NAME_BG_COLOR GREY11 // 縮微化窗口類型名文字的背景色
#define ICON_TITLE_TEXT_FG_COLOR WHITE // 縮微化窗口標題文字的前景色
#define ICON_TITLE_TEXT_BG_COLOR GREY11 // 縮微化窗口標題文字的背景色

#define ICON_AREA_COLOR GREY11 // 任務欄縮微區域的顏色
#define STATUS_AREA_COLOR GREY21 // 任務欄狀態區域的背景色
#define STATUS_AREA_TEXT_COLOR WHITE // 任務欄狀態區域的前景色
#define NORMAL_TASKBAR_BUTTON_COLOR GREY21 // 任務欄普通按鈕的顏色
#define ENTERED_TASKBAR_BUTTON_COLOR (~DODGERBLUE) // 定位器進入任務欄按鈕時按鈕的顏色
#define TASKBAR_BUTTON_TEXT_COLOR WHITE  // 任務欄按鈕文字的顏色
#define CMD_CENTER_COLOR GREY21 // 操作中心的背景顏色
#define NORMAL_CMD_CENTER_BUTTON_COLOR GREY21 // 操作中心普通按鈕的顏色
#define ENTERED_CMD_CENTER_BUTTON_COLOR (~DODGERBLUE) // 定位器進入操作中心按鈕時按鈕的顏色
#define CMD_CENTER_BUTTON_TEXT_COLOR WHITE  // 操作中心按鈕文字的顏色

#define BORDER_WIDTH 4 // 窗口框架边框的宽度，单位为像素
#define TITLE_BAR_HEIGHT 32 // 窗口標題欄的高度，單位爲像素
#define TITLE_BUTTON_WIDTH TITLE_BAR_HEIGHT // 窗口按鈕的寬度，單位爲像素
#define TITLE_BUTTON_HEIGHT TITLE_BAR_HEIGHT // 窗口按鈕的高度，單位爲像素
#define WIN_GAP 4 // 窗口間隔，單位爲像素
#define ICON_BORDER_WIDTH 1 // 縮微窗口边框的宽度，单位为像素
#define ICON_HEIGHT 30 // 縮微化窗口的高度，單位爲像素
#define ICONS_SPACE 16 // 縮微化窗口的間隔，單位爲像素
#define STATUS_AREA_WIDTH_MAX 640 // 任務欄狀態區域的最大寬度
#define TASKBAR_HEIGHT 32 // 狀態欄的高度，單位爲像素
#define TASKBAR_BUTTON_WIDTH 32 // 任務欄按鈕的寬度，單位爲像素
#define TASKBAR_BUTTON_HEIGHT 32 // 任務欄按鈕的高度，單位爲像素
#define CMD_CENTER_BUTTON_WIDTH (32*6) // 操作中心按鈕的寬度，單位爲像素
#define CMD_CENTER_BUTTON_HEIGHT 32 // 操作中心按鈕的高度，單位爲像素
#define CMD_CENTER_COL 4 // 操作中心按鈕列數
#define MOVE_RESIZE_INC 32 // 移動窗口、調整窗口尺寸的步進值，單位爲像素

#define TITLE_BUTTON_TEXT (const char *[]) /* 窗口標題欄按鈕的標籤（從左至右）*/ \
/* 切換至主區域 切換至次區域 切換至固定區 切換至懸浮態 縮微化 最大化 關閉 */     \
{       "主",        "次",        "固",          "浮",  "-",   "□", "×" }

#define TASKBAR_BUTTON_TEXT (const char *[]) /* 任務欄按鈕的標籤（從左至右） */  \
/* 切換至全屏模式 切換至概覽模式 切換至堆疊模式 切換至平鋪模式 切換桌面可見性 打開操作中心*/ \
{       "全",           "概",         "堆",          "平",          "■",        "^",}

#define CMD_CENTER_BUTTON_TEXT (const char *[]) /* 操作中心按鈕的標籤（從左至右，從上至下） */  \
{\
    "帮助",         "文件",         "终端模拟器",   "网络浏览器",   \
    "播放影音",     "切换播放状态", "关闭影音",     "减小音量",     \
    "增大音量",     "最大音量",     "静音切换",     "暂主区开窗",   \
    "暂次区开窗",   "暂固定区开窗", "暂悬浮区开窗", "暂缩微区开窗", \
    "增大主区容量", "减小主区容量", "切换聚焦模式", "退出gwm",      \
    "注销",         "重启",         "关机",         "运行",         \
}

#define CURSORS_SHAPE (unsigned int []) /* 定位器相關的光標字體 */  \
{                                                           \
    /* NO_OP */                  XC_left_ptr,               \
    /* MOVE */                   XC_fleur,                  \
    /* TOP_RESIZE */             XC_top_side,               \
    /* BOTTOM_RESIZE */          XC_bottom_side,            \
    /* LEFT_RESIZE */            XC_left_side,              \
    /* RIGHT_RESIZE */           XC_right_side,             \
    /* TOP_LEFT_RESIZE */        XC_top_left_corner,        \
    /* TOP_RIGHT_RESIZE */       XC_top_right_corner,       \
    /* BOTTOM_LEFT_RESIZE */     XC_bottom_left_corner,     \
    /* BOTTOM_RIGHT_RESIZE */    XC_bottom_right_corner,    \
    /* ADJUST_LAYOUT_RATIO */    XC_sb_h_double_arrow,      \
}

#define HELP "lxterminal -e 'man gwm' || xfce4-terminal -e 'man gwm' || xterm -e 'man gwm'"
#define FILE_MANAGER "xdg-open ~"
#define BROWSER "xdg-open http:"
#define TERMINAL "lxterminal || xfce4-terminal || gnome-terminal || konsole5 || xterm"
#define TOGGLE_PROCESS_STATE(process) "{ pid=$(pgrep -f '"process"'); " \
    "ps -o stat $pid | tail -n +2 | grep T > /dev/null ; } " \
    "&& kill -CONT $pid || kill -STOP $pid > /dev/null 2>&1"
#define PLAY_START "mplayer -shuffle ~/music/*"
#define PLAY_TOGGLE TOGGLE_PROCESS_STATE(PLAY_START)
#define PLAY_QUIT "kill -KILL $(pgrep -f '"PLAY_START"')"
#define VOLUME_DOWN "amixer -q sset Master 5%-"
#define VOLUME_UP "amixer -q sset Master 5%+"
#define VOLUME_MAX "amixer -q sset Master 100%"
#define VOLUME_TOGGLE "amixer -q sset Master toggle"
#define LOGOUT "pkill -9 'startgwm|gwm'"
#define RUN "dmenu_run"


#define KEYBINDS (Keybind []) /* 按鍵功能綁定 */                                           \
{/* 功能轉換鍵  鍵符號           要綁定的函數(詳見gwm.h)      函數的參數 */                \
    {0,	        XK_F1,           exec,                        SH_CMD(HELP)},               \
    {CMD_KEY,	XK_f,            exec,                        SH_CMD(FILE_MANAGER)},       \
    {CMD_KEY,	XK_g,            exec,                        SH_CMD("wesnoth")},          \
    {CMD_KEY,	XK_q,            exec,                        SH_CMD("qq")},               \
    {CMD_KEY,	XK_s,            exec,                        SH_CMD("stardict")},         \
    {CMD_KEY,	XK_t,            exec,                        SH_CMD(TERMINAL)},           \
    {CMD_KEY,	XK_w,            exec,                        SH_CMD(BROWSER)},            \
    {CMD_KEY, 	XK_F1,           exec,                        SH_CMD(PLAY_START)},         \
    {CMD_KEY, 	XK_F2,           exec,                        SH_CMD(PLAY_TOGGLE)},        \
    {CMD_KEY, 	XK_F3,           exec,                        SH_CMD(PLAY_QUIT)},          \
    {SYS_KEY,	XK_d,            exec,                        SH_CMD(RUN)},                \
    {SYS_KEY, 	XK_F1,           exec,                        SH_CMD(VOLUME_DOWN)},        \
    {SYS_KEY, 	XK_F2,           exec,                        SH_CMD(VOLUME_UP)},          \
    {SYS_KEY, 	XK_F3,           exec,                        SH_CMD(VOLUME_MAX)},         \
    {SYS_KEY, 	XK_F4,           exec,                        SH_CMD(VOLUME_TOGGLE)},      \
    {SYS_KEY, 	XK_l,            exec,                        SH_CMD(LOGOUT)},             \
    {SYS_KEY, 	XK_p,            exec,                        SH_CMD("poweroff")},         \
    {SYS_KEY, 	XK_r,            exec,                        SH_CMD("reboot")},           \
    {WM_KEY, 	XK_Delete,       quit_wm,                     {0}},                        \
    {WM_KEY, 	XK_Up,           key_move_resize_client,      {.direction=UP}},            \
    {WM_KEY, 	XK_Down,         key_move_resize_client,      {.direction=DOWN}},          \
    {WM_KEY, 	XK_Left,         key_move_resize_client,      {.direction=LEFT}},          \
    {WM_KEY, 	XK_Right,        key_move_resize_client,      {.direction=RIGHT}},         \
    {WM_KEY, 	XK_bracketleft,  key_move_resize_client,      {.direction=UP2UP}},         \
    {WM_KEY, 	XK_bracketright, key_move_resize_client,      {.direction=UP2DOWN}},       \
    {WM_KEY, 	XK_semicolon,    key_move_resize_client,      {.direction=DOWN2UP}},       \
    {WM_KEY, 	XK_quoteright,   key_move_resize_client,      {.direction=DOWN2DOWN}},     \
    {WM_KEY, 	XK_9,            key_move_resize_client,      {.direction=LEFT2LEFT}},     \
    {WM_KEY, 	XK_0,            key_move_resize_client,      {.direction=LEFT2RIGHT}},    \
    {WM_KEY, 	XK_minus,        key_move_resize_client,      {.direction=RIGHT2LEFT}},    \
    {WM_KEY, 	XK_equal,        key_move_resize_client,      {.direction=RIGHT2RIGHT}},   \
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
    {WM_KEY, 	XK_o,            open_cmd_center,             {0}},                        \
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
}

#define BUTTONBINDS (Buttonbind []) /* 按鈕功能綁定 */                                          \
{/* 控件類型         能轉換鍵 定位器按鈕 要綁定的函數              函數的參數 */                \
    {FULL_BUTTON,          0,   Button1, change_layout,            {.layout=FULL}},             \
    {PREVIEW_BUTTON,       0,   Button1, change_layout,            {.layout=PREVIEW}},          \
    {STACK_BUTTON,         0,   Button1, change_layout,            {.layout=STACK}},            \
    {TILE_BUTTON,          0,   Button1, change_layout,            {.layout=TILE}},             \
    {DESKTOP_BUTTON,       0,   Button1, iconify_all_clients,      {0}},                        \
    {TITLE_AREA,           0,   Button1, pointer_move_client,      {0}},                        \
    {MAIN_BUTTON,          0,   Button1, change_area,              {.area_type=MAIN_AREA}},     \
    {SECOND_BUTTON,        0,   Button1, change_area,              {.area_type=SECOND_AREA}},   \
    {FIXED_BUTTON,         0,   Button1, change_area,              {.area_type=FIXED_AREA}},    \
    {FLOAT_BUTTON,         0,   Button1, change_area,              {.area_type=FLOATING_AREA}}, \
    {ICON_BUTTON,          0,   Button1, change_area,              {.area_type=ICONIFY_AREA}},  \
    {MAX_BUTTON,           0,   Button1, maximize_client,          {0}},                        \
    {CLOSE_BUTTON,         0,   Button1, close_client,             {0}},                        \
    {CLIENT_WIN,           0,   Button1, choose_client,            {0}},                        \
    {CLIENT_WIN,      WM_KEY,   Button1, pointer_move_client,      {0}},                        \
    {CLIENT_WIN,     WM_SKEY,   Button1, pointer_resize_client,    {0}},                        \
    {CLIENT_FRAME,         0,   Button1, pointer_resize_client,    {0}},                        \
    {CLIENT_ICON,          0,   Button1, change_area,              {.area_type=PREV_AREA}},     \
    {ROOT_WIN,             0,   Button1, adjust_layout_ratio,      {0}},                        \
    {DESKTOP_BUTTON,       0,   Button2, close_all_clients,        {0}},                        \
    {TITLE_AREA,           0,   Button2, pointer_change_area,      {0}},                        \
    {CLIENT_WIN,      WM_KEY,   Button2, pointer_change_area,      {0}},                        \
    {CLIENT_ICON,          0,   Button2, close_client,             {0}},                        \
    {DESKTOP_BUTTON,       0,   Button3, deiconify_all_clients,    {0}},                        \
    {TITLE_AREA,           0,   Button3, pointer_swap_clients,     {0}},                        \
    {CLIENT_WIN,           0,   Button3, pointer_focus_client,     {0}},                        \
    {CLIENT_WIN,      WM_KEY,   Button3, pointer_swap_clients,     {0}},                        \
    {CMD_CENTER_BUTTON,    0,   Button1, open_cmd_center,          {0}},                        \
    {HELP_BUTTON,          0,   Button1, exec,                     SH_CMD(HELP)},               \
    {FILE_BUTTON,          0,   Button1, exec,                     SH_CMD(FILE_MANAGER)},       \
    {TERM_BUTTON,          0,   Button1, exec,                     SH_CMD(TERMINAL)},           \
    {BROWSER_BUTTON,       0,   Button1, exec,                     SH_CMD(BROWSER)},            \
    {PLAY_START_BUTTON,    0,   Button1, exec,                     SH_CMD(PLAY_START)},         \
    {PLAY_TOGGLE_BUTTON,   0,   Button1, exec,                     SH_CMD(PLAY_TOGGLE)},        \
    {PLAY_QUIT_BUTTON,     0,   Button1, exec,                     SH_CMD(PLAY_QUIT)},          \
    {VOLUME_DOWN_BUTTON,   0,   Button1, exec,                     SH_CMD(VOLUME_DOWN)},        \
    {VOLUME_UP_BUTTON,     0,   Button1, exec,                     SH_CMD(VOLUME_UP)},          \
    {VOLUME_MAX_BUTTON,    0,   Button1, exec,                     SH_CMD(VOLUME_MAX)},         \
    {VOLUME_TOGGLE_BUTTON, 0,   Button1, exec,                     SH_CMD(VOLUME_TOGGLE)},      \
    {MAIN_NEW_BUTTON,      0,   Button1, change_default_area_type, {.area_type=MAIN_AREA}},     \
    {SEC_NEW_BUTTON,       0,   Button1, change_default_area_type, {.area_type=SECOND_AREA}},   \
    {FIX_NEW_BUTTON,       0,   Button1, change_default_area_type, {.area_type=FIXED_AREA}},    \
    {FLOAT_NEW_BUTTON,     0,   Button1, change_default_area_type, {.area_type=FLOATING_AREA}}, \
    {ICON_NEW_BUTTON,      0,   Button1, change_default_area_type, {.area_type=ICONIFY_AREA}},  \
    {N_MAIN_UP_BUTTON,     0,   Button1, adjust_n_main_max,        {.n=1}},                     \
    {N_MAIN_DOWN_BUTTON,   0,   Button1, adjust_n_main_max,        {.n=-1}},                    \
    {FOCUS_MODE_BUTTON,    0,   Button1, toggle_focus_mode,        {0}},                        \
    {QUIT_WM_BUTTON,       0,   Button1, quit_wm,                  {0}},                        \
    {LOGOUT_BUTTON,        0,   Button1, exec,                     SH_CMD(LOGOUT)},             \
    {REBOOT_BUTTON,        0,   Button1, exec,                     SH_CMD("reboot")},           \
    {POWEROFF_BUTTON,      0,   Button1, exec,                     SH_CMD("poweroff")},         \
    {RUN_BUTTON,           0,   Button1, exec,                     SH_CMD(RUN)},                \
}

#define RULES (Rule []) /* 窗口管理器對窗口的管理規則 */                                                       \
{/* 可通過xprop命令查看客戶程序類型和客戶程序名稱。其結果表示爲：                                              \
        WM_CLASS(STRING) = "客戶程序名稱", "客戶程序類型"                                                      \
    客戶程序類型           客戶程序名稱          圖標文字  窗口放置位置       標題欄高度        邊框寬度 */    \
    {"Qq",                 "qq",                 "QQ",     FIXED_AREA,        0,                0},            \
    {"explorer.exe",       "explorer.exe",       NULL,     FLOATING_AREA,     0,                0},            \
    {"Thunder.exe",        "Thunder.exe",        NULL,     FLOATING_AREA,     TITLE_BAR_HEIGHT, BORDER_WIDTH}, \
    {"Google-chrome",      "google-chrome",      "chrome", DEFAULT_AREA_TYPE, TITLE_BAR_HEIGHT, BORDER_WIDTH}, \
    {"Org.gnome.Nautilus", "org.gnome.Nautilus", "文件",   DEFAULT_AREA_TYPE, TITLE_BAR_HEIGHT, BORDER_WIDTH}, \
}

#endif
