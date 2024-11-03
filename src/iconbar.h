/* *************************************************************************
 *     iconbar.h：與iconbar.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef ICONBAR_H
#define ICONBAR_H

#include "widget.h"

typedef struct iconbar_tag Iconbar;

Window iconbar_get_client_win(Iconbar *iconbar, Window button_win);
Iconbar *iconbar_new(Widget *parent, Widget_state state, int x, int y, int w, int h);
void iconbar_del(Iconbar *iconbar);
void iconbar_add_cbutton(Iconbar *iconbar, Window cwin);
void iconbar_del_cbutton(Iconbar *iconbar, Window cwin);
void iconbar_update(Iconbar *iconbar);
void iconbar_update_by_state(Iconbar *iconbar, Window cwin);
void iconbar_update_by_icon_name(Iconbar *iconbar, Window cwin, const char *icon_name);
void iconbar_update_by_icon(Iconbar *iconbar, Window cwin, Imlib_Image image);

#endif