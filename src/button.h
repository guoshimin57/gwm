/* *************************************************************************
 *     button.h：與button.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef BUTTON_H
#define BUTTON_H

#include "gwm.h"
#include "font.h"

typedef struct _button_tag Button;

#define BUTTON(widget) ((Button *)(widget))

Button *create_button(Widget_id id, Widget_state state, Window parent, int x, int y, int w, int h, const char *label);
void set_button_icon(Button *button, Imlib_Image image, const char *icon_name, const char *symbol);
char *get_button_label(Button *button);
void set_button_label(Button *button, const char *label);
void set_button_align(Button *button, Align_type align);
void destroy_button(Button *button);
void show_button(const Button *button);
void update_button_fg(const Button *button);

#endif
