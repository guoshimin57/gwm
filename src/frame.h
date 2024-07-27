/* *************************************************************************
 *     frame.h：與frame.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef FRAME_H 
#define FRAME_H

#include <Imlib2.h>
#include "menu.h"
#include "widget.h"

typedef struct _frame_tag Frame;
typedef struct _titlebar_tag Titlebar;
typedef struct rectangle_tag Rect;

Frame *create_frame(Widget *parent, Widget_state state, int x, int y, int w, int h, int titlebar_h, int border_w, const char *title, Imlib_Image image);
void destroy_frame(Frame *frame);
void move_resize_frame(Frame *frame, int x, int y, int w, int h);
Rect get_frame_rect_by_win(const Frame *frame, int x, int y, int w, int h);
Rect get_win_rect_by_frame(const Frame *frame);
bool is_frame_part(const Frame *frame, Window win);
void set_frame_state_current(Frame *frame, int value);
void update_frame_bg(const Frame *frame);
Menu *get_frame_menu(const Frame *frame);
Titlebar *get_frame_titlebar(const Frame *frame);
void toggle_titlebar(Frame *frame, const char *title, Imlib_Image image);
void update_titlebar_fg(const Widget *widget);
void update_titlebar_layout(const Frame *frame);
void change_title(const Frame *frame, const char *title);
void change_frame_logo(const Frame *frame, Imlib_Image image);

#endif
