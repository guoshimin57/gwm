/* *************************************************************************
 *     entry.h：與entry.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef ENTRY_H
#define ENTRY_H

#include <stdbool.h>
#include "widget.h"
#include "listview.h"
#include "misc.h"

#define ENTRY(widget) ((Entry *)(widget))

typedef struct _entry_tag Entry;

extern Entry *cmd_entry, *color_entry;

Entry *entry_new(Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *));
void entry_clear(Entry *entry);
void entry_del(Entry *entry);
void entry_show(Widget *widget);
void entry_hide(const Widget *widget);
void entry_update_bg(const Widget *widget);
void entry_update_fg(const Widget *widget);
wchar_t *entry_get_text(Entry *entry);
bool entry_input(Entry *entry, XKeyEvent *ke);
void entry_paste(Entry *entry);
Listview *entry_get_listview(Entry *entry);
Entry *cmd_entry_new(Widget_id id);
Entry *color_entry_new(Widget_id id);

#endif
