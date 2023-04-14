/* *************************************************************************
 *     config.h：與config.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

// 說明：尺寸單位均爲像素
struct config_tag
{
    bool set_frame_prop; // true表示把窗口特性復制到窗口框架（代價是每個窗口可能要多消耗幾十到幾百KB內存），false表示不復制
    bool use_image_icon; // true表示使用圖像形式的圖標，false表示使用文字形式的圖標
    char default_font_name[FONT_NAME_MAX]; // 默認字體名
    char font_name[FONT_N][FONT_NAME_MAX]; // 本窗口管理器所使用的字庫名稱列表。注意：每增加一種不同的字體，就會增加2M左右的內存佔用
    Focus_mode focus_mode; // 聚焦模式
    Layout default_layout; // 默認的窗口布局模式
    Area_type default_area_type; // 新打開的窗口的默認區域類型
    Color_theme color_theme; // 顏色主題
    /* 屏幕保護程序的行爲取決於X服務器，可能顯示移動的圖像，可能只是黑屏。*/
    unsigned int screen_saver_time_out; // 激活內置屏幕之前的空閒時間，單位爲秒。當值爲0時表示禁用屏保，爲-1時恢復缺省值。
    unsigned int screen_saver_interval; // 內置屏保周期性變化的時間間隔，單位爲秒。當值爲0時表示禁止周期性變化。
    unsigned int hover_time; // 定位器懸停的判定時間界限，單位爲毫秒
    unsigned int default_cur_desktop; // 默認的當前桌面
    unsigned int default_n_main_max; // 默認的主區域最大窗口數量
    unsigned int cmd_center_col; // 操作中心按鈕列數

    /* 以下尺寸的單位均爲像素 */
    unsigned int font_size[FONT_N]; // 字體尺寸列表
    /* 以下尺寸根據相應字體來確定就有不錯的效果 */
    unsigned int border_width; // 窗口框架边框的宽度
    unsigned int title_bar_height; // 窗口標題欄的高度
    unsigned int title_button_width; // 窗口按鈕的寬度
    unsigned int title_button_height;// 窗口按鈕的高度
    unsigned int win_gap; // 窗口間隔
    unsigned int status_area_width_max; // 任務欄狀態區域的最大寬度
    unsigned int taskbar_height; // 狀態欄的高度
    unsigned int taskbar_button_width; // 任務欄按鈕的寬度
    unsigned int taskbar_button_height; // 任務欄按鈕的高度
    unsigned int icon_size; // 圖標的尺寸
    unsigned int icon_win_width_max; // 縮微窗口的最大寬度
    unsigned int icons_space; // 縮微化窗口的間隔
    unsigned int cmd_center_item_width; // 操作中心按鈕的寬度
    unsigned int cmd_center_item_height; // 操作中心按鈕的高度
    unsigned int entry_text_indent; // 文字縮進量
    unsigned int run_cmd_entry_width; // 運行命令的輸入構件的寬度
    unsigned int run_cmd_entry_height; // 運行命令的輸入構件的寬度
    unsigned int hint_win_line_height; // 提示窗口的行高度
    unsigned int resize_inc; // 調整尺寸的步進值。當應用於窗口時，僅當窗口未有效設置尺寸特性時才使用它。

    unsigned int cursor_shape[POINTER_ACT_N]; // 定位器相關的光標字體

    double default_main_area_ratio; // 默認的主區域比例
    double default_fixed_area_ratio; // 默認的固定區域比例
    const char *autostart; // 在gwm剛啓動時執行的腳本
    const char *cur_icon_theme; // 當前圖標主題
    const char *screenshot_path; // 屏幕截圖的文件保存路徑，請自行確保路徑存在
    const char *screenshot_format; // 屏幕截圖的文件保存格式
    const char *wallpaper_paths; // 壁紙目錄列表，如取消此宏定义或目录为空或不能访问，则切换绝壁时使用纯色
    const char *wallpaper_filename; // 壁紙文件名。若刪除本行或文件不能訪問，則使用純色背景
    const char *tooltip[WIDGET_N]; // 构件提示
    const char *widget_color_name[COLOR_THEME_N][WIDGET_COLOR_N]; // 構件顏色名
    const char *text_color_name[COLOR_THEME_N][TEXT_COLOR_N]; // 構件顏色名
    const char *title_button_text[TITLE_BUTTON_N]; // 窗口標題欄按鈕的標籤
    const char *taskbar_button_text[TASKBAR_BUTTON_N]; // 任務欄按鈕的標籤
    const char *cmd_center_item_text[CMD_CENTER_ITEM_N]; // 操作中心按鈕的標籤
    const wchar_t *run_cmd_entry_hint; // 運行輸入框的提示文字
    const Keybind *keybind; // 按鍵功能綁定。有的鍵盤同時按多個鍵會衝突，故組合鍵宜盡量少
    const Buttonbind *buttonbind; // 定位器按鈕功能綁定。
    const Rule *rule; // 窗口管理器對窗口的管理規則
};

void config(WM *wm);

#endif
