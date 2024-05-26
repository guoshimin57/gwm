/* *************************************************************************
 *     tooltip.h：與tooltip.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef TOOLTIP_H
#define MTOOLTIP_H

#include "gwm.h"

typedef struct _tooltip_tag Tooltip;

#define TOOLTIP(widget) ((Tooltip *)(widget))

Tooltip *create_tooltip(Widget *owner, const char *tip);
void change_tooltip_tip(Tooltip *tooltip, const char *tip);
void destroy_tooltip(Tooltip *tooltip);
void show_tooltip(Widget *widget);
void update_tooltip_fg(const Widget *widget);

#endif
