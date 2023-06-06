/* *************************************************************************
 *     desktop.h：與desktop.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef DESKTOP_H
#define DESKTOP_H

struct desktop_tag // 虛擬桌面相關信息
{
    int n_main_max; // 主區域可容納的客戶窗口數量
    Client *cur_focus_client, *prev_focus_client; // 分別爲當前聚焦結點、前一個聚焦結點
    Layout cur_layout, prev_layout; // 分別爲當前布局模式和前一個布局模式
    Area_type default_area_type; // 默認的窗口區域類型
    double main_area_ratio, fixed_area_ratio; // 分別爲主要和固定區域與工作區寬度的比值
};

void init_desktop(WM *wm);
unsigned int get_desktop_n(WM *wm, XEvent *e, Func_arg arg);
void focus_desktop_n(WM *wm, unsigned int n);
bool is_on_desktop_n(unsigned int n, Client *c);
bool is_on_cur_desktop(WM *wm, Client *c);
unsigned int get_desktop_mask(unsigned int desktop_n);
void move_to_desktop_n(WM *wm, Client *c, unsigned int n);
void all_move_to_desktop_n(WM *wm, unsigned int n);
void change_to_desktop_n(WM *wm, Client *c, unsigned int n);
void all_change_to_desktop_n(WM *wm, unsigned int n);
void attach_to_desktop_n(WM *wm, Client *c, unsigned int n);
void attach_to_desktop_all(WM *wm, Client *c);
void all_attach_to_desktop_n(WM *wm, unsigned int n);

#endif
