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

#ifndef WIDGET_H
#define WIDGET_H

typedef enum // 構件顏色類型
{
    NORMAL_BORDER_COLOR, CURRENT_BORDER_COLOR, NORMAL_TITLEBAR_COLOR,
    CURRENT_TITLEBAR_COLOR, ENTERED_NORMAL_BUTTON_COLOR,
    ENTERED_CLOSE_BUTTON_COLOR, CHOSEN_BUTTON_COLOR, MENU_COLOR, TASKBAR_COLOR,
    ENTRY_COLOR, HINT_WIN_COLOR, URGENCY_WIDGET_COLOR, ATTENTION_WIDGET_COLOR,
    ROOT_WIN_COLOR, WIDGET_COLOR_N 
} Widget_color;

typedef enum // 文本顏色類型
{
    NORMAL_TITLEBAR_TEXT_COLOR, CURRENT_TITLEBAR_TEXT_COLOR,
    TASKBAR_TEXT_COLOR, CLASS_TEXT_COLOR, MENU_TEXT_COLOR,
    ENTRY_TEXT_COLOR, HINT_TEXT_COLOR, TEXT_COLOR_N 
} Text_color;

typedef enum // 構件類型
{
    ROOT_WIN, HINT_WIN, RUN_CMD_ENTRY,

    CLIENT_FRAME, CLIENT_WIN, TITLE_LOGO, TITLE_AREA,

    SECOND_BUTTON, MAIN_BUTTON, FIXED_BUTTON, FLOAT_BUTTON,
    ICON_BUTTON, MAX_BUTTON, CLOSE_BUTTON,

    CLIENT_MENU,
    SHADE_BUTTON, VERT_MAX_BUTTON, HORZ_MAX_BUTTON, TOP_MAX_BUTTON,
    BOTTOM_MAX_BUTTON, LEFT_MAX_BUTTON, RIGHT_MAX_BUTTON, FULL_MAX_BUTTON,

    TASKBAR, ICON_AREA, CLIENT_ICON, STATUS_AREA,
    DESKTOP1_BUTTON, DESKTOP2_BUTTON, DESKTOP3_BUTTON, 
    PREVIEW_BUTTON, STACK_BUTTON, TILE_BUTTON, DESKTOP_BUTTON,
    ACT_CENTER_ITEM,

    ACT_CENTER, 
    HELP_BUTTON, FILE_BUTTON, TERM_BUTTON, BROWSER_BUTTON, 
    GAME_BUTTON, PLAY_START_BUTTON, PLAY_TOGGLE_BUTTON, PLAY_QUIT_BUTTON,
    VOLUME_DOWN_BUTTON, VOLUME_UP_BUTTON, VOLUME_MAX_BUTTON, VOLUME_TOGGLE_BUTTON,
    MAIN_NEW_BUTTON, SEC_NEW_BUTTON, FIX_NEW_BUTTON, FLOAT_NEW_BUTTON,
    N_MAIN_UP_BUTTON, N_MAIN_DOWN_BUTTON, TITLEBAR_TOGGLE_BUTTON, CLI_BORDER_TOGGLE_BUTTON,
    CLOSE_ALL_CLIENTS_BUTTON, PRINT_WIN_BUTTON, PRINT_SCREEN_BUTTON, FOCUS_MODE_BUTTON,
    COMPOSITOR_BUTTON, WALLPAPER_BUTTON, COLOR_THEME_BUTTON, QUIT_WM_BUTTON,
    LOGOUT_BUTTON, REBOOT_BUTTON, POWEROFF_BUTTON, RUN_BUTTON,

    NON_WIDGET,

    WIDGET_N=NON_WIDGET,
    TITLE_BUTTON_BEGIN=SECOND_BUTTON, TITLE_BUTTON_END=CLOSE_BUTTON,
    TASKBAR_BUTTON_BEGIN=DESKTOP1_BUTTON, TASKBAR_BUTTON_END=ACT_CENTER_ITEM,
    LAYOUT_BUTTON_BEGIN=PREVIEW_BUTTON, LAYOUT_BUTTON_END=TILE_BUTTON, 
    DESKTOP_BUTTON_BEGIN=DESKTOP1_BUTTON, DESKTOP_BUTTON_END=DESKTOP3_BUTTON,
    ACT_CENTER_ITEM_BEGIN=HELP_BUTTON, ACT_CENTER_ITEM_END=RUN_BUTTON,
    CLIENT_MENU_ITEM_BEGIN=SHADE_BUTTON, CLIENT_MENU_ITEM_END=FULL_MAX_BUTTON, 
} Widget_type;

typedef enum // 定位器操作類型
{
    NO_OP, CHOOSE, MOVE, SWAP, CHANGE, TOP_RESIZE, BOTTOM_RESIZE, LEFT_RESIZE,
    RIGHT_RESIZE, TOP_LEFT_RESIZE, TOP_RIGHT_RESIZE, BOTTOM_LEFT_RESIZE,
    BOTTOM_RIGHT_RESIZE, ADJUST_LAYOUT_RATIO, POINTER_ACT_N
} Pointer_act;

#define TITLE_BUTTON_N (TITLE_BUTTON_END-TITLE_BUTTON_BEGIN+1)
#define TASKBAR_BUTTON_N (TASKBAR_BUTTON_END-TASKBAR_BUTTON_BEGIN+1)
#define ACT_CENTER_ITEM_N (ACT_CENTER_ITEM_END-ACT_CENTER_ITEM_BEGIN+1)
#define CLIENT_MENU_ITEM_N (CLIENT_MENU_ITEM_END-CLIENT_MENU_ITEM_BEGIN+1)
#define DESKTOP_N (DESKTOP_BUTTON_END-DESKTOP_BUTTON_BEGIN+1)

#define WIDGET_INDEX(type_name, type_class) ((type_name) - type_class ## _BEGIN)
#define DESKTOP_BUTTON_N(n) (DESKTOP_BUTTON_BEGIN+n-1)
#define IS_WIDGET_CLASS(type_name, type_class) \
    (type_class ## _BEGIN <= (type_name) && (type_name) <= type_class ## _END)
#define IS_BUTTON(type) \
    (  type==CLIENT_ICON || type==TITLE_LOGO \
    || IS_WIDGET_CLASS(type, TITLE_BUTTON) \
    || IS_WIDGET_CLASS(type, CLIENT_MENU_ITEM) \
    || IS_WIDGET_CLASS(type, TASKBAR_BUTTON) \
    || IS_WIDGET_CLASS(type, ACT_CENTER_ITEM))
#define IS_MENU_ITEM(type) \
    (  IS_WIDGET_CLASS(type, CLIENT_MENU_ITEM) \
    || IS_WIDGET_CLASS(type, ACT_CENTER_ITEM))


void alloc_color(void);
unsigned long get_widget_color(Widget_color wc);
XftColor get_text_color(Text_color color_id);
Window create_widget_win(Widget_type type, Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel);
Widget_type get_widget_type(Window win);
void update_hint_win_for_info(Window hover, const char *info);
void draw_icon(Drawable d, Imlib_Image image, const char *name, int size);
void set_xic(Window win, XIC *ic);
KeySym look_up_key(XIC xic, XKeyEvent *e, wchar_t *keyname, size_t n);
void create_hint_win(void);
void create_client_menu(void);
void create_cursors(void);
void set_cursor(Window win, Pointer_act act);
void free_cursors(void);
void grab_keys(void);
void grab_buttons(Window win);
bool is_equal_modifier_mask(unsigned int m1, unsigned int m2);
bool grab_pointer(Window win, Pointer_act act);

#endif
