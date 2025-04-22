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

#include <Imlib2.h>
#include "widget.h"
#include "client.h"

typedef struct _taskbar_tag Taskbar; // 任務欄

Taskbar *get_taskbar(void);
void taskbar_new(Widget *parent, int x, int y, int w, int h);
void taskbar_del(void);
void taskbar_update_bg(void);
void taskbar_set_urgency(const Client *c);
void taskbar_set_attention(const Client *c);
void taskbar_add_client(Window cwin);
void taskbar_remove_client(Window cwin);
Window taskbar_get_client_win(const Window button_win);
void taskbar_update_by_client_state(Window cwin);
void taskbar_update_by_icon_name(const Window cwin, const char *icon_name);
void taskbar_update_by_icon_image(const Window cwin, Imlib_Image image);
void taskbar_change_statusbar_label(const char *label);
void taskbar_show_act_center(void);

#endif
