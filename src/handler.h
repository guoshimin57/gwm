/* *************************************************************************
 *     handler.h：與handler.c相應的頭文件。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef HANDLER_H
#define HANDLER_H

void handle_events(WM *wm);
void handle_event(WM *wm, XEvent *e);
void handle_button_press(WM *wm, XEvent *e);
void handle_config_request(WM *wm, XEvent *e);
void handle_enter_notify(WM *wm, XEvent *e);
void handle_expose(WM *wm, XEvent *e);
void handle_focus_change(WM *wm, XEvent *e);
void handle_key_press(WM *wm, XEvent *e);
void handle_leave_notify(WM *wm, XEvent *e);
void handle_map_request(WM *wm, XEvent *e);
void handle_motion_notify(WM *wm, XEvent *e);
void handle_unmap_notify(WM *wm, XEvent *e);
void handle_property_notify(WM *wm, XEvent *e);

#endif
