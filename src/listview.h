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

#include "gwm.h"

typedef struct _list_view_tag List_view;

#define LIST_VIEW(widget) ((List_view *)(widget))

List_view *create_list_view(Widget *parent, Widget_id id, int x, int y, int w, int h, const Strings *texts);
void show_list_view(Widget *widget);
void update_list_view_fg(const Widget *widget);
void update_list_view(List_view *list_view, Strings *texts);
void set_list_view_nmax(List_view *list_view, int nmax);

#endif
