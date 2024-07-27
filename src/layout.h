/* *************************************************************************
 *     layout.h：與layout.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef LAYOUT_H
#define LAYOUT_H

#include "gwm.h"

void update_layout(WM *wm);
bool is_main_sec_gap(WM *wm, int x);
bool is_main_fix_gap(WM *wm, int x);
bool is_layout_adjust_area(WM *wm, Window win, int x);

#endif
