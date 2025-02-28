/* *************************************************************************
 *     taskbar.h：與taskbar.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef TASKBAR_H
#define TASKBAR_H

#include "widget.h"
#include "iconbar.h"
#include "statusbar.h"

typedef struct _taskbar_tag Taskbar; // 任務欄

Taskbar *taskbar_new(Widget *parent, int x, int y, int w, int h);
void taskbar_del(Taskbar *taskbar);
void taskbar_iconbar_update(Taskbar *taskbar);
void taskbar_iconbar_update_by_state(Taskbar *taskbar, Window cwin);
void taskbar_buttons_update_bg(Taskbar *taskbar);
void taskbar_update_bg(const Widget *widget);
void taskbar_set_urgency(Taskbar *taskbar, unsigned int desktop_mask);
void taskbar_set_attention(Taskbar *taskbar, unsigned int desktop_mask);
void taskbar_buttons_update_bg_by_chosen(Taskbar *taskbar);
Iconbar *taskbar_get_iconbar(const Taskbar *taskbar);
Statusbar *taskbar_get_statusbar(const Taskbar *taskbar);
void taskbar_add_client(Taskbar *taskbar, Window cwin);
void taskbar_del_client(Taskbar *taskbar, Window cwin);

#endif
