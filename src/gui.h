/* *************************************************************************
 *     gui.h：與gui.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef GUI_H
#define GUI_H

#include <X11/Xlib.h>
#include "widget.h"

void init_gui(void);
void deinit_gui(void);
void update_gui(void);
void open_color_settings(void);
void open_run_cmd(void);
void key_set_color(XKeyEvent *e);
void key_run_cmd(XKeyEvent *e);

#endif
