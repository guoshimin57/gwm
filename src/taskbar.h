/* *************************************************************************
 *     taskbar.h：與taskbar.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

#include <stdbool.h>
#include "widget.h"

typedef struct _taskbar_tag Taskbar; // 任務欄

extern Taskbar *taskbar;

void create_taskbar(void);
void taskbar_add_cbutton(Window cwin);
void taskbar_del_cbutton(Window cwin);
Window get_iconic_win(Window button_win);
void update_iconbar(void);
void update_iconbar_by_state(Window cwin);
void update_taskbar_buttons_bg(void);
void update_statusbar_fg(void);
void set_statusbar_label(const char *label);
void update_taskbar_bg(const Widget *widget);
void destroy_taskbar(void);
void set_taskbar_urgency(Window cwin, bool urg);
void set_taskbar_attention(Window cwin, bool attent);
void update_taskbar_buttons_bg_by_chosen(void);

#endif
