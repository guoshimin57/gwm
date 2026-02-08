/* *************************************************************************
 *     config.c：gwm通用配置。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <X11/cursorfont.h>
#include "misc.h"
#include "config.h"

#define SET_WIDGET_MARKERS(id, icon_name, symbol, label) \
    cfg->widget_icon_names[id]=icon_name, \
    cfg->widget_symbols[id]=symbol, \
    cfg->widget_labels[id]=label

Config *cfg=NULL;

/* 本窗口管理器所偏好的字體名稱列表。
 * 每增加一個字體，會增加0.1M內存，但也會提高效果。 */
static const char *font_names[]=
{
    "monospace",
//    "Noto Color Emoji",
    "Noto Sans Symbols 2",
    "Symbola",
    NULL // 哨兵值，表示結束，切勿刪改之
};

/* 功能：設置字體。
 * 說明：當字體大小設置爲0或負數時，表示由程序自行選擇合適的尺寸。
 * 可通過fc-list命令查看可用字體，如：
 *     fc-list :lang=zh family
 */
static void config_font(void)
{
    cfg->font_size=0;
    cfg->font_names=font_names;
}

/* 功能：設置構件尺寸。
 * 說明：當以下尺寸爲0或負數時，表示程序自行選擇合適的尺寸。
 * 建議以字號爲基準來設置構件大小。標識符定義詳見gwm.h。
 */
static void config_widget_size(void)
{
    cfg->border_width=0;
    cfg->title_button_width=0;
    cfg->win_gap=0;
    cfg->statusbar_width_max=0;
    cfg->taskbar_button_width=0;
    cfg->iconbar_width_max=0;
    cfg->icon_image_size=0;
    cfg->icon_gap=0;
    cfg->resize_inc=0;
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
    cursor_shape[LAYOUT_RESIZE]       = XC_sb_h_double_arrow;
}

/* 功能：設置標題按鈕的文字。
 * 說明：標題欄按鈕類型的定義詳見gwm.h:Widget_id。
 */
static void config_title_button_text(void)
{

    //                 標題欄按鈕類型 圖標名 符號 標籤
    SET_WIDGET_MARKERS(SECOND_BUTTON, NULL, NULL, "◁");
    SET_WIDGET_MARKERS(MAIN_BUTTON,   NULL, NULL, "▼");
    SET_WIDGET_MARKERS(FIXED_BUTTON,  NULL, NULL, "▷");
    SET_WIDGET_MARKERS(ICON_BUTTON,   NULL, NULL, "—");
    SET_WIDGET_MARKERS(MAX_BUTTON,    NULL, NULL, "◲" );
    SET_WIDGET_MARKERS(CLOSE_BUTTON,  NULL, NULL, "🗙" );
}

/* 功能：設置任務欄按鈕的文字。
 * 說明：任務欄按鈕類型的定義詳見gwm.h:Widget_id。
 */
static void config_taskbar_button_text(void)
{
    //                 任務欄按鈕類型   圖標名 符號 標籤
    SET_WIDGET_MARKERS(DESKTOP0_BUTTON, NULL, NULL, "1" );
    SET_WIDGET_MARKERS(DESKTOP1_BUTTON, NULL, NULL, "2" );
    SET_WIDGET_MARKERS(DESKTOP2_BUTTON, NULL, NULL, "3" );
    SET_WIDGET_MARKERS(STACK_BUTTON,    NULL, NULL, "▣");
    SET_WIDGET_MARKERS(TILE_BUTTON,     NULL, NULL, "▥");
    SET_WIDGET_MARKERS(DESKTOP_BUTTON,  NULL, NULL, "■");
    SET_WIDGET_MARKERS(ACT_CENTER_ITEM, NULL, NULL, "^" );
}

/* 功能：設置操作中心菜單項。
 * 說明：操作中心按鈕類型的定義詳見gwm.h:Widget_id。
 */
static void config_act_center_item(void)
{
    //                 操作中心按鈕類型         圖標名 符號     標籤
    SET_WIDGET_MARKERS(HELP_BUTTON,              NULL, "🛟", _("幫助"));
    SET_WIDGET_MARKERS(FILE_BUTTON,              NULL, "📁", _("文件"));
    SET_WIDGET_MARKERS(TERM_BUTTON,              NULL, "🖥️", _("終端模擬器"));
    SET_WIDGET_MARKERS(BROWSER_BUTTON,           NULL, "🌐", _("網絡瀏覽器"));

    SET_WIDGET_MARKERS(GAME_BUTTON,              NULL, "🎮️", _("遊戲"));
    SET_WIDGET_MARKERS(PLAY_START_BUTTON,        NULL, "🎬", _("播放影音"));
    SET_WIDGET_MARKERS(PLAY_TOGGLE_BUTTON,       NULL, "⏯️", _("切換播放狀態"));
    SET_WIDGET_MARKERS(PLAY_QUIT_BUTTON,         NULL, "⏹️", _("關閉影音"));

    SET_WIDGET_MARKERS(VOLUME_DOWN_BUTTON,       NULL, "🔈️", _("减小音量"));
    SET_WIDGET_MARKERS(VOLUME_UP_BUTTON,         NULL, "🔉", _("增大音量"));
    SET_WIDGET_MARKERS(VOLUME_MAX_BUTTON,        NULL, "🔊", _("最大音量"));
    SET_WIDGET_MARKERS(VOLUME_TOGGLE_BUTTON,     NULL, "🔇", _("靜音切換"));

    SET_WIDGET_MARKERS(N_MAIN_UP_BUTTON,         NULL, "⬆️", _("增大主區容量"));
    SET_WIDGET_MARKERS(N_MAIN_DOWN_BUTTON,       NULL, "⬇️", _("减小主區容量"));
    
    SET_WIDGET_MARKERS(CLOSE_ALL_CLIENTS_BUTTON, NULL, "❎", _("關閉桌面所有窗口"));
    SET_WIDGET_MARKERS(PRINT_WIN_BUTTON,         NULL, "✀",  _("當前窗口截圖"));
    SET_WIDGET_MARKERS(PRINT_SCREEN_BUTTON,      NULL, "🖵",  _("全屏截圖"));
    SET_WIDGET_MARKERS(FOCUS_MODE_BUTTON,        NULL, "👁️", _("切換聚焦模式"));

    SET_WIDGET_MARKERS(COMPOSITOR_BUTTON,        NULL, "🪡", _("開關合成器"));
    SET_WIDGET_MARKERS(WALLPAPER_BUTTON,         NULL, "🌌", _("切換壁紙"));
    SET_WIDGET_MARKERS(COLOR_BUTTON,             NULL, "🎨", _("设置顏色"));
    SET_WIDGET_MARKERS(QUIT_WM_BUTTON,           NULL, "🗘", _("退出gwm"));

    SET_WIDGET_MARKERS(LOGOUT_BUTTON,            NULL, "🚶", _("注銷"));
    SET_WIDGET_MARKERS(REBOOT_BUTTON,            NULL, "↻",  _("重啓"));
    SET_WIDGET_MARKERS(POWEROFF_BUTTON,          NULL, "⏻",  _("關機"));
    SET_WIDGET_MARKERS(RUN_BUTTON,               NULL, "🔍️", _("運行"));
}

/* 功能：設置客戶窗口菜單項。
 * 說明：客戶窗口菜單項類型的定義詳見gwm.h:Widget_id。
 */
static void config_client_menu_item(void)
{
    //                 客戶窗口菜單項類型   圖標名 符號    標籤
    SET_WIDGET_MARKERS(SHADE_BUTTON,         NULL, "⊽", _("卷起/放下"));
    SET_WIDGET_MARKERS(VERT_MAX_BUTTON,      NULL, "⇅", _("縱向最大化"));
    SET_WIDGET_MARKERS(HORZ_MAX_BUTTON,      NULL, "⇆", _("橫向最大化"));
    SET_WIDGET_MARKERS(TOP_MAX_BUTTON,       NULL, "⬓", _("最大化至上半屏"));
    SET_WIDGET_MARKERS(BOTTOM_MAX_BUTTON,    NULL, "⬒", _("最大化至下半屏"));
    SET_WIDGET_MARKERS(LEFT_MAX_BUTTON,      NULL, "◨", _("最大化至左半屏"));
    SET_WIDGET_MARKERS(RIGHT_MAX_BUTTON,     NULL, "◧", _("最大化至右半屏"));
    SET_WIDGET_MARKERS(FULL_MAX_BUTTON,      NULL, "🗖", _("完全最大化"));
    SET_WIDGET_MARKERS(FULLSCREEN_BUTTON,    NULL, "□",_("全屏"));
    SET_WIDGET_MARKERS(ABOVE_BUTTON,         NULL, "❏", _("置頂"));
    SET_WIDGET_MARKERS(BELOW_BUTTON,         NULL, "_", _("置底"));
}

/* 功能：設置構件功能提示。
 * 說明：構件標識的定義詳見gwm.h:Widget_id。以下未列出的構件要麼不必顯示提示，
 * 要麼動態變化而不可在此設置。
 */
static void config_tooltip(void)
{
    const char **tooltip=cfg->tooltip;

    /*      構件標識             構件功能提示文字 */
    tooltip[SECOND_BUTTON]   = _("切換到次要區域");
    tooltip[MAIN_BUTTON]     = _("切換到主要區域");
    tooltip[FIXED_BUTTON]    = _("切換到固定區域");
    tooltip[ICON_BUTTON]     = _("切換到圖符區域");
    tooltip[MAX_BUTTON]      = _("最大化/還原窗口");
    tooltip[CLOSE_BUTTON]    = _("關閉窗口");
    tooltip[DESKTOP0_BUTTON] = _("切換到虛擬桌面1");
    tooltip[DESKTOP1_BUTTON] = _("切換到虛擬桌面2");
    tooltip[DESKTOP2_BUTTON] = _("切換到虛擬桌面3");
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
    cfg->screen_saver_time_out=1800;
    cfg->screen_saver_interval=1800;
    cfg->hover_time=300;
    cfg->default_cur_desktop=0;
    cfg->default_main_area_n=1;
    cfg->act_center_col=4;
    cfg->font_pad_ratio=0.25;
    cfg->default_main_area_ratio=0.533;
    cfg->default_fixed_area_ratio=0.15;
    cfg->autostart="~/.config/gwm/autostart.sh";
    cfg->cur_icon_theme="default";
    cfg->screenshot_path="~";
    cfg->screenshot_format="png";
    cfg->wallpaper_paths="/usr/share/backgrounds/fedora-workstation:/usr/share/wallpapers";
    cfg->wallpaper_filename="/usr/share/backgrounds/gwm.png";
    cfg->cmd_entry_hint=_("請輸入命令，然後按回車執行");
    cfg->color_entry_hint=_("請輸入系統界面主色調的顏色名（支持英文顏色名和十六进制顏色名），然後按回車執行");
    cfg->compositor="picom";
}

/* =========================== 用戶配置項結束 =========================== */ 

void config(void)
{
    cfg=Malloc(sizeof(Config));
    for(size_t i=0; i<WIDGET_N; i++)
        cfg->tooltip[i]=NULL;

    config_misc();
    config_font();
    config_widget_size();
    config_cursor_shape();
    cfg->main_color_name="black";
    cfg->widget_opacity=0.8; // 全局不透明度
    config_title_button_text();
    config_taskbar_button_text();
    config_act_center_item();
    config_client_menu_item();
    config_tooltip();
}
