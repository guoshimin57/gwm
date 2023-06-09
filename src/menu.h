/* *************************************************************************
 *     menu.h：與menu.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef MENU_H
#define MENU_H

struct menu_tag // 一級多行多列菜單 
{
    Window win, *items; // 菜單窗口和菜單項
    int n, col, row, w, h, pad; // 菜單項數量、列數、行數、寬度、高度、四周空白
    int x, y; // 菜單窗口的坐標
    unsigned long bg; // 菜單的背景色
};

Menu *create_menu(WM *wm, int n, int col, int w, int h, int pad, unsigned long bg);
void show_menu(WM *wm, XEvent *e, Menu *menu, Window bind);
void set_menu_pos_for_click(WM *wm, Window win, int x, int y, Menu *menu);

#endif
