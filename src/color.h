/* *************************************************************************
 *     color.h：與color.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef COLOR_H
#define COLOR_H

/* 以下带参宏中的type是Widget_color和Text_color枚舉常量中的構件成分。
 * 譬如：CURRENT_TITLEBAR_COLOR，type就是TITLEBAR。 */

// 獲取無狀態（即不區分是否當前的或被選中）構件的顏色
#define WIDGET_COLOR(wm, type) get_widget_color(wm, type ##_COLOR)

// 獲取無狀態（即不區分是否當前的或被選中）構件上的文字顏色
#define TEXT_COLOR(wm, type) \
    wm->text_color[wm->cfg->color_theme][type ## _TEXT_COLOR]

// 根據選中與否的條件而得出按鈕的顏色
#define NCHOSEN_BUTTON_COLOR(wm, type, normal_index) \
    get_widget_color(wm, is_chosen_button(wm, type) \
        ? CHOSEN_BUTTON_COLOR : normal_index)

// 根據是否進入關閉按鈕的條件而得出按鈕的顏色
#define ENTERED_NCLOSE_BUTTON_COLOR(wm, type) \
    get_widget_color(wm, type==CLOSE_BUTTON \
        ? ENTERED_CLOSE_BUTTON_COLOR : ENTERED_NORMAL_BUTTON_COLOR)

// 根據是否當前客戶窗口的條件而得出構件顏色
#define NCUR_WIDGET_COLOR(wm, cur, type) \
    get_widget_color(wm, cur ? \
        CURRENT_ ## type ## _COLOR : NORMAL_ ## type ## _COLOR)

// 根據是否當前桌面的當前客戶窗口而得出構件顏色
#define CLI_WIDGET_COLOR(wm, c, type) \
    get_widget_color(wm, c==CUR_FOC_CLI(wm) ? \
        CURRENT_ ## type ## _COLOR : NORMAL_ ## type ## _COLOR)

// 根據是否當前桌面的當前客戶窗口而得出文字顏色
#define CLI_TEXT_COLOR(wm, c, type) \
    wm->text_color[wm->cfg->color_theme][c==CUR_FOC_CLI(wm) ? \
    CURRENT_ ## type ## _TEXT_COLOR : NORMAL_ ## type ## _TEXT_COLOR]

void alloc_color(WM *wm);
void update_widget_color(WM *wm);
unsigned long get_widget_color(WM *wm, Widget_color index);
void update_taskbar_buttons_bg(WM *wm);
void update_client_bg(WM *wm, unsigned int desktop_n, Client *c);
void update_frame_bg(WM *wm, unsigned int desktop_n, Client *c);

#endif
