/* *************************************************************************
 *     widget.h：與widget.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef WIDGET_H_
#define WIDGET_H_

#include <stdbool.h>
#include "color.h"

typedef enum // 構件標識
{
    ROOT_WIN, HINT_WIN, RUN_CMD_ENTRY,

    CLIENT_FRAME, CLIENT_WIN, TITLE_LOGO, TITLEBAR,

    SECOND_BUTTON, MAIN_BUTTON, FIXED_BUTTON, FLOAT_BUTTON,
    ICON_BUTTON, MAX_BUTTON, CLOSE_BUTTON,

    CLIENT_MENU,
    SHADE_BUTTON, VERT_MAX_BUTTON, HORZ_MAX_BUTTON, TOP_MAX_BUTTON,
    BOTTOM_MAX_BUTTON, LEFT_MAX_BUTTON, RIGHT_MAX_BUTTON, FULL_MAX_BUTTON,

    TASKBAR, ICONBAR, CLIENT_ICON, STATUSBAR,
    DESKTOP0_BUTTON, DESKTOP1_BUTTON, DESKTOP2_BUTTON, 
    PREVIEW_BUTTON, STACK_BUTTON, TILE_BUTTON, DESKTOP_BUTTON,
    ACT_CENTER_ITEM,

    ACT_CENTER, 
    HELP_BUTTON, FILE_BUTTON, TERM_BUTTON, BROWSER_BUTTON, 
    GAME_BUTTON, PLAY_START_BUTTON, PLAY_TOGGLE_BUTTON, PLAY_QUIT_BUTTON,
    VOLUME_DOWN_BUTTON, VOLUME_UP_BUTTON, VOLUME_MAX_BUTTON, VOLUME_TOGGLE_BUTTON,
    MAIN_NEW_BUTTON, SEC_NEW_BUTTON, FIX_NEW_BUTTON, FLOAT_NEW_BUTTON,
    N_MAIN_UP_BUTTON, N_MAIN_DOWN_BUTTON, TITLEBAR_TOGGLE_BUTTON, CLI_BORDER_TOGGLE_BUTTON,
    CLOSE_ALL_CLIENTS_BUTTON, PRINT_WIN_BUTTON, PRINT_SCREEN_BUTTON, FOCUS_MODE_BUTTON,
    COMPOSITOR_BUTTON, WALLPAPER_BUTTON, QUIT_WM_BUTTON, LOGOUT_BUTTON,
    REBOOT_BUTTON, POWEROFF_BUTTON, RUN_BUTTON,

    UNUSED_WIDGET_ID,

    WIDGET_N=UNUSED_WIDGET_ID,
    TITLE_BUTTON_BEGIN=SECOND_BUTTON, TITLE_BUTTON_END=CLOSE_BUTTON,
    TASKBAR_BUTTON_BEGIN=DESKTOP0_BUTTON, TASKBAR_BUTTON_END=ACT_CENTER_ITEM,
    LAYOUT_BUTTON_BEGIN=PREVIEW_BUTTON, LAYOUT_BUTTON_END=TILE_BUTTON, 
    DESKTOP_BUTTON_BEGIN=DESKTOP0_BUTTON, DESKTOP_BUTTON_END=DESKTOP2_BUTTON,
    ACT_CENTER_ITEM_BEGIN=HELP_BUTTON, ACT_CENTER_ITEM_END=RUN_BUTTON,
    CLIENT_MENU_ITEM_BEGIN=SHADE_BUTTON, CLIENT_MENU_ITEM_END=FULL_MAX_BUTTON, 
} Widget_id;

typedef enum // 定位器操作類型
{
    NO_OP, CHOOSE, MOVE, SWAP, CHANGE, TOP_RESIZE, BOTTOM_RESIZE, LEFT_RESIZE,
    RIGHT_RESIZE, TOP_LEFT_RESIZE, TOP_RIGHT_RESIZE, BOTTOM_LEFT_RESIZE,
    BOTTOM_RIGHT_RESIZE, ADJUST_LAYOUT_RATIO, POINTER_ACT_N
} Pointer_act;

typedef struct _widget_tag Widget;

struct _widget_tag
{
    Widget_id id;
    Widget_state state;
    int x, y, w, h, border_w;
    bool poppable;
    Window win;
    Widget *parent, *tooltip;

    /* 以下爲虛函數 */
    void (*del)(Widget *widget);
    void (*show)(Widget *widget);
    void (*hide)(const Widget *widget);
    void (*update_bg)(const Widget *widget);
    void (*update_fg)(const Widget *widget);
};

typedef struct rectangle_tag Rect;

#define WIDGET_EVENT_MASK (ExposureMask)
#define TITLE_BUTTON_N (TITLE_BUTTON_END-TITLE_BUTTON_BEGIN+1)
#define TASKBAR_BUTTON_N (TASKBAR_BUTTON_END-TASKBAR_BUTTON_BEGIN+1)
#define ACT_CENTER_ITEM_N (ACT_CENTER_ITEM_END-ACT_CENTER_ITEM_BEGIN+1)
#define CLIENT_MENU_ITEM_N (CLIENT_MENU_ITEM_END-CLIENT_MENU_ITEM_BEGIN+1)
#define DESKTOP_N (DESKTOP_BUTTON_END-DESKTOP_BUTTON_BEGIN+1)

#define WIDGET(p) ((Widget *)(p))
#define WIDGET_ID(p) (WIDGET(p)->id)
#define WIDGET_TYPE(p) (WIDGET(p)->type)
#define WIDGET_STATE(p) (WIDGET(p)->state)
#define WIDGET_X(p) (WIDGET(p)->x)
#define WIDGET_Y(p) (WIDGET(p)->y)
#define WIDGET_W(p) (WIDGET(p)->w)
#define WIDGET_H(p) (WIDGET(p)->h)
#define WIDGET_BORDER_W(p) (WIDGET(p)->border_w)
#define WIDGET_WIN(p) (WIDGET(p)->win)
#define WIDGET_TOOLTIP(p) (WIDGET(p)->tooltip)

#define WIDGET_INDEX(type_name, type_class) ((type_name) - type_class ## _BEGIN)
#define DESKTOP_BUTTON_N(n) (DESKTOP_BUTTON_BEGIN+n-1)

Widget *widget_find(Window win);
Widget *widget_new(Widget *parent, Widget_id id, Widget_state state, int x, int y, int w, int h);
void widget_ctor(Widget *widget, Widget *parent, Widget_id id, Widget_state state, int x, int y, int w, int h);
void widget_del(Widget *widget);
void widget_set_border_width(Widget *widget, int width);
void widget_set_border_color(const Widget *widget, unsigned long pixel);
void widget_show(Widget *widget);
void widget_hide(const Widget *widget);
void widget_resize(Widget *widget, int w, int h);
void widget_move_resize(Widget *widget, int x, int y, int w, int h);
void widget_update_bg(const Widget *widget);
void widget_update_fg(const Widget *widget);
void widget_set_rect(Widget *widget, int x, int y, int w, int h);
Rect widget_get_outline(const Widget *widget);
void widget_set_poppable(Widget *widget, bool poppable);
bool widget_get_poppable(const Widget *widget);
bool widget_is_viewable(const Widget *widget);
bool hide_popped_widgets(const Widget *clicked_widget);
Window create_widget_win(Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel);
void update_hint_win_for_info(const Widget *widget, const char *info);
void set_xic(Window win, XIC *ic);
KeySym look_up_key(XIC xic, XKeyEvent *e, wchar_t *keyname, size_t n);
void create_hint_win(void);
void create_cursors(void);
void set_cursor(Window win, Pointer_act act);
void free_cursors(void);
void grab_keys(void);
void grab_buttons(Window win);
bool is_equal_modifier_mask(unsigned int m1, unsigned int m2);
bool grab_pointer(Window win, Pointer_act act);

#endif
