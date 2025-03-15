/* *************************************************************************
 *     config.h：與config.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include "gwm.h"
#include <stdbool.h>

typedef enum // 窗口聚焦模式
{
    ENTER_FOCUS, // 進入窗口即聚焦的模式
    CLICK_FOCUS, // 點擊窗口才聚焦的模式
} Focus_mode;

// 說明：尺寸單位均爲像素
typedef struct
{
    bool set_frame_prop; // true表示把窗口特性復制到窗口框架（代價是每個窗口可能要多消耗幾十到幾百KB內存），false表示不復制
    bool show_taskbar, taskbar_on_top; // 是否顯示任務欄、是否在屏幕頂部顯示
    Focus_mode focus_mode; // 聚焦模式
    Layout default_layout; // 默認的窗口布局模式

    /* 屏幕保護程序的行爲取決於X服務器，可能顯示移動的圖像，可能只是黑屏。*/
    int screen_saver_time_out; // 激活內置屏幕之前的空閒時間，單位爲秒。當值爲0時表示禁用屏保，爲-1時恢復缺省值。
    int screen_saver_interval; // 內置屏保周期性變化的時間間隔，單位爲秒。當值爲0時表示禁止周期性變化。

    int default_n_main_max; // 默認的主區域最大窗口數量
    int act_center_col; // 操作中心按鈕列數

    int font_size; // 字體尺寸
    /* 以下尺寸根據相應字體來確定就有不錯的效果 */
    int border_width; // 窗口框架边框的宽度
    int title_button_width; // 窗口按鈕的寬度
    int win_gap; // 窗口間隔
    int statusbar_width_max; // 任務欄狀態區域的最大寬度
    int taskbar_button_width; // 任務欄按鈕的寬度
    int icon_win_width_max; // 縮微窗口的最大寬度
    int icon_image_size; // 圖標映像的邊長
    int icon_gap; // 縮微化窗口的間隔
    int resize_inc; // 調整尺寸的步進值。當應用於窗口時，僅當窗口未有效設置尺寸特性時才使用它。

    unsigned int default_cur_desktop; // 默認的當前桌面
    unsigned int cursor_shape[POINTER_ACT_N]; // 定位器相關的光標字體
    time_t hover_time; // 定位器懸停的判定時間界限，單位爲毫秒

    double font_pad_ratio; // 文字與構件邊緣的間距與字體高度的比值
    double default_main_area_ratio; // 默認的主區域比例
    double default_fixed_area_ratio; // 默認的固定區域比例
    float widget_opacity; // 構件背景色不透明度
    const char **font_names; // 本窗口管理器所偏好的字體名稱列表。
    const char *autostart; // 在gwm剛啓動時執行的腳本
    const char *cur_icon_theme; // 當前圖標主題
    const char *screenshot_path; // 屏幕截圖的文件保存路徑，請自行確保路徑存在
    const char *screenshot_format; // 屏幕截圖的文件保存格式
    const char *wallpaper_paths; // 壁紙目錄列表，如取消此宏定义或目录为空或不能访问，则切换绝壁时使用纯色
    const char *wallpaper_filename; // 壁紙文件名。若刪除本行或文件不能訪問，則使用純色背景
    const char *main_color_name; // 界面主色調
    const char *tooltip[WIDGET_N]; // 构件提示
    const char *title_button_text[TITLE_BUTTON_N]; // 窗口標題欄按鈕的標籤
    const char *taskbar_button_text[TASKBAR_BUTTON_N]; // 任務欄按鈕的標籤
    const char *act_center_item_icon[ACT_CENTER_ITEM_N]; // 操作中心菜單項的圖標名
    const char *act_center_item_symbol[ACT_CENTER_ITEM_N]; // 操作中心菜單項的符號
    const char *act_center_item_label[ACT_CENTER_ITEM_N]; // 操作中心菜單項的標籤
    const char *client_menu_item_icon[CLIENT_MENU_ITEM_N]; // 客戶窗口菜單項的圖標名
    const char *client_menu_item_symbol[CLIENT_MENU_ITEM_N]; // 客戶窗口菜單項的符號
    const char *client_menu_item_label[CLIENT_MENU_ITEM_N]; // 客戶窗口菜單項的標籤
    const char *cmd_entry_hint; // 運行輸入框的提示文字
    const char *color_entry_hint; // 颜色輸入框的提示文字
    const char *compositor; // 合成管理器命令
    const Rule *rule; // 窗口管理器對窗口的管理規則
} Config;

extern Config *cfg; // 窗口管理器配置

void config(void);

#endif
