/* *************************************************************************
 *     listview.h：與listview.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef LISTVIEW_H
#define LISTVIEW_H

#include "misc.h"
#include "widget.h"

typedef struct _listview_tag Listview;

#define LIST_VIEW(widget) ((Listview *)(widget))

Listview *listview_new(Widget *parent, Widget_id id, int x, int y, int w, int h, const Strings *texts);
void listview_show(Widget *widget);
void listview_update_fg(const Widget *widget);
void listview_update(Listview *listview, Strings *texts);
void listview_set_nmax(Listview *listview, int nmax);

#endif
