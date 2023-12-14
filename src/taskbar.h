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

typedef struct // 窗口管理器的任務欄
{
    /* 分別爲任務欄的窗口、按鈕、縮微區域、狀態區域 */
    Window win, buttons[TASKBAR_BUTTON_N], icon_area, status_area;
    int urgency_n[DESKTOP_N], attent_n[DESKTOP_N]; // 分別爲存儲各桌面的緊急、注意提示數量的數組
    int x, y; // win的坐標
    int w, h, status_area_w; // win的尺寸、按鈕的尺寸和狀態區域的寬度
    char *status_text; // 狀態區域要顯示的文字
} Taskbar;

extern Taskbar *taskbar;

Taskbar *create_taskbar(void);
void update_taskbar_buttons_bg(WM *wm);
void update_taskbar_button_bg(WM *wm, Widget_type type);
void update_taskbar_button_fg(Widget_type type);
void update_status_area_fg(void);
void update_icon_status_area(void);
void update_client_icon_fg(WM *wm, Window win);
void destroy_taskbar(Taskbar *taskbar);

#endif
