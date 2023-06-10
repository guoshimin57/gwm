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

/* 以下带参宏中的type只允许是gwm.h中的枚举常量的构成部分 */
#define WIDGET_COLOR(wm, type) wm->widget_color[wm->cfg->color_theme][type ## _COLOR].pixel
#define TEXT_COLOR(wm, type) wm->text_color[wm->cfg->color_theme][type ## _TEXT_COLOR]
#define TASKBAR_BUTTON_COLOR(wm, type) /* 根據是否選中而得出任務欄按鈕顏色 */ \
    wm->widget_color[wm->cfg->color_theme][is_chosen_button(wm, type) ? \
    CHOSEN_TASKBAR_BUTTON_COLOR : NORMAL_TASKBAR_BUTTON_COLOR].pixel
#define NC_WIDGET_COLOR(wm, cur, type) /* 根據是否當前客戶窗口的條件而得出構件顏色 */ \
    wm->widget_color[wm->cfg->color_theme][cur ? \
    CURRENT_ ## type ## _COLOR : NORMAL_ ## type ## _COLOR].pixel
#define CWIDGET_COLOR(wm, c, type) /* 根據是否當前桌面的當前客戶窗口而得出構件顏色 */ \
    wm->widget_color[wm->cfg->color_theme][c==CUR_FOC_CLI(wm) ? \
    CURRENT_ ## type ## _COLOR : NORMAL_ ## type ## _COLOR].pixel
#define CTEXT_COLOR(wm, c, type) /* 根據是否當前桌面的當前客戶窗口而得出文字顏色 */ \
    wm->text_color[wm->cfg->color_theme][c==CUR_FOC_CLI(wm) ? \
    CURRENT_ ## type ## _TEXT_COLOR : NORMAL_ ## type ## _TEXT_COLOR]

void alloc_color(WM *wm);
void update_widget_color(WM *wm);

#endif
