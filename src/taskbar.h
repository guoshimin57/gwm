/* *************************************************************************
 *     taskbar.h：與taskbar.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

struct taskbar_tag // 窗口管理器的任務欄
{
    /* 分別爲任務欄的窗口、按鈕、縮微區域、狀態區域 */
    Window win, buttons[TASKBAR_BUTTON_N], icon_area, status_area;
    int x, y; // win的坐標
    int w, h, status_area_w; // win的尺寸、按鈕的尺寸和狀態區域的寬度
    char *status_text; // 狀態區域要顯示的文字
};

void create_taskbar(WM *wm);
void update_taskbar_button(WM *wm, Widget_type type, bool change_bg);
void hint_leave_taskbar_button(WM *wm, Widget_type type);
void update_icon_text(WM *wm, Window win);
void update_act_center_button_text(WM *wm, size_t index);
void update_status_area_text(WM *wm);
void update_status_area(WM *wm);

#endif
