/* *************************************************************************
 *     func.c：實現按鍵和按鍵所要綁定的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "clientop.h"
#include "entry.h"
#include "file.h"
#include "font.h"
#include "layout.h"
#include "mvresize.h"
#include "icccm.h"
#include "image.h"
#include "place.h"
#include "focus.h"
#include "desktop.h"
#include "wallpaper.h"
#include "grab.h"
#include "taskbar.h"
#include "gui.h"
#include "func.h"

/* ========================== Func函數命名風格 =============================
 * 爲了方便配置功能綁定，對於所有用於綁定的函數名均應盡量簡潔。如無特殊說明，
 * 缺省操作對象的，均指操作Client對象。
 */

void choose(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();

    if(is_iconic_client(c))
        deiconify_client(c);
}

void exec(XEvent *e, Arg arg)
{
    UNUSED(e);
    exec_cmd(arg.cmd);
}

void quit_wm(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    request_quit();
}

void quit(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    /* 刪除窗口會產生UnmapNotify事件，處理該事件時再刪除框架 */
    close_win(WIDGET_WIN(get_cur_focus_client()));
}

void quit_all(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    clients_for_each(c)
        if(is_on_cur_desktop(c))
            close_win(WIDGET_WIN(c));
}

void next(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_client(get_next(get_cur_focus_client()));
}

void prev(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_client(get_prev(get_cur_focus_client()));
}

void rise_main_n(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    adjust_main_area_n(1);
}

void fall_main_n(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    adjust_main_area_n(-1);
}

void toggle_focus_mode(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    cfg->focus_mode = cfg->focus_mode==ENTER_FOCUS ? CLICK_FOCUS : ENTER_FOCUS;
}

void start(XEvent *e, Arg arg) // 開始，即打開操作中心（然後開始執行操作）
{
    UNUSED(e), UNUSED(arg);
    taskbar_show_act_center();
}

void open_client_menu(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();
    if(c->decorative)
        menu_show(WIDGET(frame_get_menu(c->frame)));
}

void focus_desktop(XEvent *e, Arg arg)
{
    focus_desktop_n(get_desktop_n(e, arg));
}

void next_desktop(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    unsigned int cur_desktop=get_net_current_desktop();
    focus_desktop_n(cur_desktop+1<DESKTOP_N ? cur_desktop+1 : 1);
}

void prev_desktop(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    unsigned int cur_desktop=get_net_current_desktop();
    focus_desktop_n(cur_desktop>0 ? cur_desktop-1 : DESKTOP_N-1);
}

void move_to_desktop(XEvent *e, Arg arg)
{
    move_to_desktop_n(get_desktop_n(e, arg));
}

void all_move_to_desktop(XEvent *e, Arg arg)
{
    all_move_to_desktop_n(get_desktop_n(e, arg));
}

void change_to_desktop(XEvent *e, Arg arg)
{
    change_to_desktop_n(get_desktop_n(e, arg));
}

void all_change_to_desktop(XEvent *e, Arg arg)
{
    all_change_to_desktop_n(get_desktop_n(e, arg));
}

void attach_to_desktop(XEvent *e, Arg arg)
{
    attach_to_desktop_n(get_desktop_n(e, arg));
}

void attach_to_all_desktops(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    attach_to_desktop_all();
}

void all_attach_to_desktop(XEvent *e, Arg arg)
{
    all_attach_to_desktop_n(get_desktop_n(e, arg));
}

void run_cmd(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    open_run_cmd();
}

void set_color(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    open_color_settings();
}

void switch_wallpaper(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    switch_to_next_wallpaper();
}

void print_screen(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    print_area(xinfo.root_win, 0, 0, xinfo.screen_width, xinfo.screen_height);
}

void print_win(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();
    if(c)
        print_area(WIDGET_WIN(c->frame), 0, 0, WIDGET_W(c), WIDGET_H(c));
}

void toggle_compositor(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Window win=get_compositor();

    if(win)
        XKillClient(xinfo.display, win);
    else
        exec_cmd(SH_CMD(cfg->compositor));
}

void mini(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    iconify_client(get_cur_focus_client()); 
}

void deiconify(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    deiconify_client(get_cur_focus_client()); 
}

void toggle_max_restore(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();

    if(is_win_state_max(c->win_state))
        restore_client(c);
    else
        maximize_client(c, FULL_MAX);
}

void vmax(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), VERT_MAX);
}

void hmax(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), HORZ_MAX);
}

void tmax(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), TOP_MAX);
}

void bmax(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), BOTTOM_MAX);
}

void lmax(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), LEFT_MAX);
}

void rmax(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), RIGHT_MAX);
}

void max(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    maximize_client(get_cur_focus_client(), FULL_MAX);
}

void show_desktop(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool show=false;

    toggle_showing_desktop_mode(show=!show);
}

void move_up(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, UP);
}

void move_down(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, DOWN);
}

void move_left(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, LEFT);
}

void move_right(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, RIGHT);
}

void fall_width(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, FALL_WIDTH);
}

void rise_width(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, RISE_WIDTH);
}

void fall_height(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, FALL_HEIGHT);
}

void rise_height(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_move_resize_client(e, RISE_HEIGHT);
}

void move(XEvent *e, Arg arg)
{
    UNUSED(arg);
    pointer_move_resize_client(e, false);
}

void resize(XEvent *e, Arg arg)
{
    UNUSED(arg);
    pointer_move_resize_client(e, true);
}

void toggle_shade(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool shade=false;

    toggle_shade_mode(get_cur_focus_client(), shade=!shade);
}

void change_place(XEvent *e, Arg arg)
{
    UNUSED(arg);
    pointer_change_place(e);
}

void to_main_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(TILE_LAYER, MAIN_AREA);
}

void to_second_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(TILE_LAYER, SECOND_AREA);
}

void to_fixed_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(TILE_LAYER, FIXED_AREA);
}

void to_stack_layer(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(STACK_LAYER, ANY_AREA);
}

void fullscreen(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(FULLSCREEN_LAYER, ANY_AREA);
}

void to_above_layer(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(ABOVE_LAYER, ANY_AREA);
}

void to_below_layer(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    client_change_place(BELOW_LAYER, ANY_AREA);
}

void swap(XEvent *e, Arg arg)
{
    UNUSED(arg);
    pointer_swap_clients(e);
}

void stack(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    change_layout(STACK);
}

void tile(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    change_layout(TILE);
}

void adjust_layout_ratio(XEvent *e, Arg arg)
{
    UNUSED(arg);
    pointer_adjust_layout_ratio(e);
}

void rise_main_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Rect wr=get_net_workarea();
    adjust_main_area((double)cfg->resize_inc/wr.w);
}

void fall_main_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Rect wr=get_net_workarea();
    adjust_main_area(-(double)cfg->resize_inc/wr.w);
}

void rise_fixed_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Rect wr=get_net_workarea();
    adjust_fixed_area((double)cfg->resize_inc/wr.w);
}

void fall_fixed_area(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Rect wr=get_net_workarea();
    adjust_fixed_area(-(double)cfg->resize_inc/wr.w);
}
