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
#include "entry.h"
#include "file.h"
#include "font.h"
#include "layout.h"
#include "mvresize.h"
#include "minimax.h"
#include "menu.h"
#include "icccm.h"
#include "image.h"
#include "place.h"
#include "prop.h"
#include "focus.h"
#include "taskbar.h"
#include "desktop.h"
#include "func.h"

static bool is_valid_click(XEvent *oe, XEvent *ne);

bool is_drag_func(void (*func)(WM *, XEvent *, Arg))
{
    return func == pointer_swap_clients
        || func == pointer_move
        || func == pointer_resize
        || func == pointer_change_place
        || func == adjust_layout_ratio;
}

static bool is_grab_root_act(Pointer_act act)
{
    return act==SWAP || act==CHANGE;
}

bool get_valid_click(WM *wm, Pointer_act act, XEvent *oe, XEvent *ne)
{
    if(act==CHOOSE && widget_find(oe->xbutton.window)->id==CLIENT_WIN)
        return true;

    Window win = is_grab_root_act(act) ? xinfo.root_win : oe->xbutton.window;
    if(act!=NO_OP && !grab_pointer(win, act))
        return false;

    XEvent e, *p=(ne ? ne : &e);
    do
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, p);
        wm->event_handler(wm, p);
    }while(!is_match_button_release(oe, p));
    if(act != NO_OP)
        XUngrabPointer(xinfo.display, CurrentTime);
    return is_valid_click(oe, p);
}

static bool is_valid_click(XEvent *oe, XEvent *ne)
{
    return is_equal_modifier_mask(oe->xbutton.state, ne->xbutton.state)
        && is_pointer_on_win(ne->xbutton.window);
}

void choose_client(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();

    if(is_iconic_client(c))
        deiconify_client(c);

    if(is_spec_layout(PREVIEW))
        restore_prev_layout(wm);
}

void exec(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e);
    exec_cmd(arg.cmd);
}

void quit_wm(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    clear_wm(wm);
    exit(EXIT_SUCCESS);
}

void clear_wm(WM *wm)
{
    clients_for_each_safe(c)
    {
        XReparentWindow(xinfo.display, WIDGET_WIN(c), xinfo.root_win, WIDGET_X(c), WIDGET_Y(c));
        client_del(c, true);
    }
    free_all_images();
    XDestroyWindow(xinfo.display, xinfo.hint_win);
    XDestroyWindow(xinfo.display, wm->wm_check_win);
    taskbar_del(wm->taskbar);
    entry_del(cmd_entry);
    entry_del(color_entry);
    menu_del(act_center);
    del_refer_top_wins();
    XFreeModifiermap(xinfo.mod_map);
    free_cursors();
    XSetInputFocus(xinfo.display, xinfo.root_win, RevertToPointerRoot, CurrentTime);
    if(xinfo.xim)
        XCloseIM(xinfo.xim);
    close_fonts();
    XClearWindow(xinfo.display, xinfo.root_win);
    XFlush(xinfo.display);
    XCloseDisplay(xinfo.display);
    clear_zombies(0);
    vfree_strings(wm->wallpapers);
    Free(cfg);
}

void close_client(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    /* 刪除窗口會產生UnmapNotify事件，處理該事件時再刪除框架 */
    Client *c=get_cur_focus_client();
    if(c)
        close_win(WIDGET_WIN(c));
}

void close_all_clients(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask))
            close_win(WIDGET_WIN(c));
}

void next_client(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    focus_client(get_next_client(get_cur_focus_client()));
}

void prev_client(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    focus_client(get_prev_client(get_cur_focus_client()));
}

void increase_main_n(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    adjust_main_area_n(1);
}

void decrease_main_n(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    adjust_main_area_n(-1);
}

void toggle_focus_mode(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    cfg->focus_mode = cfg->focus_mode==ENTER_FOCUS ? CLICK_FOCUS : ENTER_FOCUS;
}

void open_act_center(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    menu_show(WIDGET(act_center));
}

void open_client_menu(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();
    if(c->decorative)
        menu_show(WIDGET(frame_get_menu(c->frame)));
}

void focus_desktop(WM *wm, XEvent *e, Arg arg)
{
    focus_desktop_n(wm, get_desktop_n(e, arg));
}

void next_desktop(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    unsigned int cur_desktop=get_net_current_desktop();
    focus_desktop_n(wm, cur_desktop+1<DESKTOP_N ? cur_desktop+1 : 1);
}

void prev_desktop(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    unsigned int cur_desktop=get_net_current_desktop();
    focus_desktop_n(wm, cur_desktop>0 ? cur_desktop-1 : DESKTOP_N-1);
}

void move_to_desktop(WM *wm, XEvent *e, Arg arg)
{
    move_to_desktop_n(wm, get_desktop_n(e, arg));
}

void all_move_to_desktop(WM *wm, XEvent *e, Arg arg)
{
    all_move_to_desktop_n(wm, get_desktop_n(e, arg));
}

void change_to_desktop(WM *wm, XEvent *e, Arg arg)
{
    change_to_desktop_n(wm, get_desktop_n(e, arg));
}

void all_change_to_desktop(WM *wm, XEvent *e, Arg arg)
{
    all_change_to_desktop_n(wm, get_desktop_n(e, arg));
}

void attach_to_desktop(WM *wm, XEvent *e, Arg arg)
{
    attach_to_desktop_n(wm, get_desktop_n(e, arg));
}

void attach_to_all_desktops(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    attach_to_desktop_all(wm);
}

void all_attach_to_desktop(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm);
    all_attach_to_desktop_n(get_desktop_n(e, arg));
}

void run_cmd(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    entry_clear(cmd_entry);
    entry_show(WIDGET(cmd_entry));
}

void set_color(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    entry_clear(color_entry);
    entry_show(WIDGET(color_entry));
}

void switch_wallpaper(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    srand((unsigned int)time(NULL));
    unsigned long r1=rand(), r2=rand(), color=(r1<<16)|r2|0xff000000UL;
    Pixmap pixmap=None;
    if(cfg->wallpaper_paths)
    {
        pixmap=create_pixmap_from_file(xinfo.root_win, wm->cur_wallpaper->str);
        wm->cur_wallpaper=list_next_entry(wm->cur_wallpaper, Strings, list);
        if(list_entry_is_head(wm->cur_wallpaper, &wm->wallpapers->list, list))
            wm->cur_wallpaper=list_next_entry(wm->cur_wallpaper, Strings, list);
    }
    update_win_bg(xinfo.root_win, color, pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}

void print_screen(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    print_area(xinfo.root_win, 0, 0, xinfo.screen_width, xinfo.screen_height);
}

void print_win(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    Client *c=get_cur_focus_client();
    if(c)
        print_area(WIDGET_WIN(c->frame), 0, 0, WIDGET_W(c), WIDGET_H(c));
}

void toggle_compositor(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    Window win=get_compositor();

    if(win)
        XKillClient(xinfo.display, win);
    else
        exec(wm, e, (Arg)SH_CMD((char *)cfg->compositor));
}
