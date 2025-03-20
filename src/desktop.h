/* *************************************************************************
 *     desktop.h：與desktop.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef DESKTOP_H
#define DESKTOP_H

#include "gwm.h"
#include "client.h"

typedef struct // 虛擬桌面相關信息
{
    int n_main_max; // 主區域可容納的客戶窗口數量
    Client *cur_focus_client, *prev_focus_client; // 分別爲當前聚焦結點、前一個聚焦結點
    Layout cur_layout, prev_layout; // 分別爲當前布局模式和前一個布局模式
    double main_area_ratio, fixed_area_ratio; // 分別爲主要和固定區域與工作區寬度的比值
} Desktop;

void init_desktop(void);
void free_desktop(void);
Desktop *get_cur_desktop(void);
void set_cur_focus_client(Client *c);
Client *get_cur_focus_client(void);
void set_prev_focus_client(Client *c);
Client *get_prev_focus_client(void);
Layout get_cur_layout(void);
void set_cur_layout(Layout layout);
Layout get_prev_layout(void);
void set_prev_layout(Layout layout);
int get_n_main_max(void);
void set_n_main_max(int n);
double get_main_area_ratio(void);
void set_main_area_ratio(double ratio);
double get_fixed_area_ratio(void);
void set_fixed_area_ratio(double ratio);
unsigned int get_desktop_n(XEvent *e, Arg arg);
void focus_desktop_n(WM *wm, unsigned int n);
void move_to_desktop_n(WM *wm, unsigned int n);
void all_move_to_desktop_n(WM *wm, unsigned int n);
void change_to_desktop_n(WM *wm, unsigned int n);
void all_change_to_desktop_n(WM *wm, unsigned int n);
void attach_to_desktop_n(WM *wm, unsigned int n);
void attach_to_desktop_all(WM *wm);
void all_attach_to_desktop_n(unsigned int n);

#endif
