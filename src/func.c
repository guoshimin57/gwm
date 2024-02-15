/* *************************************************************************
 *     func.c：實現按鍵和按鍵所要綁定的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "mvresize.h"

static bool is_valid_click(XEvent *oe, XEvent *ne);

bool is_drag_func(void (*func)(WM *, XEvent *, Func_arg))
{
    return func == pointer_swap_clients
        || func == move_resize
        || func == pointer_change_place
        || func == adjust_layout_ratio;
}

static bool is_grab_root_act(Pointer_act act)
{
    return act==SWAP || act==CHANGE;
}

bool get_valid_click(WM *wm, Pointer_act act, XEvent *oe, XEvent *ne)
{
    if(act==CHOOSE && get_widget_type(oe->xbutton.window)==CLIENT_WIN)
        return true;

    Window win = is_grab_root_act(act) ? xinfo.root_win : oe->xbutton.window;
    if(act!=NO_OP && !grab_pointer(win, act))
        return false;

    XEvent e, *p=(ne ? ne : &e);
    do
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, p);
        wm->event_handlers[p->type](wm, p);
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

void choose_client(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);

    if(c->icon)
        deiconify_client(wm, c);

    if(DESKTOP(wm)->cur_layout == PREVIEW)
        arg.layout=DESKTOP(wm)->prev_layout, change_layout(wm, e, arg);
}

void exec(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e);
    exec_cmd(arg.cmd);
}

void quit_wm(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    clear_wm(wm);
    exit(EXIT_SUCCESS);
}

void clear_wm(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        XReparentWindow(xinfo.display, c->win, xinfo.root_win, c->x, c->y);
        del_client(wm, c, true);
    }
    XDestroyWindow(xinfo.display, xinfo.hint_win);
    XDestroyWindow(xinfo.display, wm->wm_check_win);
    destroy_taskbar(taskbar);
    destroy_entry(cmd_entry);
    destroy_menu(act_center);
    destroy_menu(client_menu);
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        XDestroyWindow(xinfo.display, wm->top_wins[i]);
    XFreeGC(xinfo.display, wm->gc);
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
    free_strings(wm->wallpapers);
    for(size_t i=0; i<DESKTOP_N; i++)
        free(wm->desktop[i]);
    free(cfg);
}

void close_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    /* 刪除窗口會產生UnmapNotify事件，處理該事件時再刪除框架 */
    Client *c=CUR_FOC_CLI(wm);
    if(c != wm->clients)
        close_win(c->win);
}

void close_all_clients(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(is_on_cur_desktop(wm, c))
            close_win(c->win);
}

/* 取得存儲次序上在當前客戶之前的客戶（或其亞組長）。因使用頭插法存儲客戶，
 * 故若兩者同放置類型的話，該客戶在時間上比當前客戶出現得遲(next) */
void next_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_client(wm, wm->cur_desktop, get_prev_client(wm, CUR_FOC_CLI(wm)));
}

/* 取得存儲次序上在當前客戶（或其亞組長）之後的客戶。因使用頭插法存儲客戶，
 * 故若兩者同放置類型的話，該客戶在時間上比當前客戶出現得早(prev) */
void prev_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_client(wm, wm->cur_desktop, get_next_client(wm, CUR_FOC_CLI(wm)));
}

void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    if(DESKTOP(wm)->cur_layout == TILE)
    {
        int *m=&DESKTOP(wm)->n_main_max;
        *m = *m+arg.desktop_n>=1 ? *m+arg.desktop_n : 1;
        request_layout_update();
    }
}

void toggle_focus_mode(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    cfg->focus_mode = cfg->focus_mode==ENTER_FOCUS ? CLICK_FOCUS : ENTER_FOCUS;
}

void open_act_center(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(arg);
    show_menu(e, act_center, e->xbutton.window);
}

void open_client_menu(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(arg);
    show_menu(e, client_menu, e->xbutton.window);
}

void toggle_border_visibility(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);
    c->border_w = c->border_w ? 0 : cfg->border_width;
    XSetWindowBorderWidth(xinfo.display, c->frame, c->border_w);
    request_layout_update();
}

void toggle_titlebar_visibility(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);
    c->titlebar_h = c->titlebar_h ? 0 : get_font_height_by_pad();
    if(c->titlebar_h)
    {
        create_titlebar(wm, c);
        XMapSubwindows(xinfo.display, c->frame);
    }
    else
    {
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            XDestroyWindow(xinfo.display, c->buttons[i]);
        XDestroyWindow(xinfo.display, c->title_area);
        XDestroyWindow(xinfo.display, c->logo);
    }
    request_layout_update();
}

void focus_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    focus_desktop_n(wm, get_desktop_n(e, arg));
}

void next_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_desktop_n(wm, wm->cur_desktop<DESKTOP_N ? wm->cur_desktop+1 : 1);
}

void prev_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    focus_desktop_n(wm, wm->cur_desktop>1 ? wm->cur_desktop-1 : DESKTOP_N);
}

void move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    move_to_desktop_n(wm, CUR_FOC_CLI(wm), get_desktop_n(e, arg));
}

void all_move_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_move_to_desktop_n(wm, get_desktop_n(e, arg));
}

void change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    change_to_desktop_n(wm, CUR_FOC_CLI(wm), get_desktop_n(e, arg));
}

void all_change_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_change_to_desktop_n(wm, get_desktop_n(e, arg));
}

void attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    attach_to_desktop_n(wm, CUR_FOC_CLI(wm), get_desktop_n(e, arg));
}

void attach_to_all_desktops(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    attach_to_desktop_all(wm, CUR_FOC_CLI(wm));
}

void all_attach_to_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    all_attach_to_desktop_n(wm, get_desktop_n(e, arg));
}

void enter_and_run_cmd(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    show_entry(cmd_entry);
}

void switch_wallpaper(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    srand((unsigned int)time(NULL));
    unsigned long r1=rand(), r2=rand(), color=(r1<<16)|r2|0xff000000UL;
    Pixmap pixmap=None;
    if(cfg->wallpaper_paths)
    {
        Strings *f=wm->cur_wallpaper;
        if(f)
        {
            f=wm->cur_wallpaper=(f->next ? f->next : wm->wallpapers->next);
            pixmap=create_pixmap_from_file(xinfo.root_win, f->str);
        }
    }
    update_win_bg(xinfo.root_win, color, pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}

void print_screen(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    print_area(xinfo.root_win, 0, 0, xinfo.screen_width, xinfo.screen_height);
}

void print_win(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);
    if(c != wm->clients)
        print_area(c->frame, 0, 0, c->w, c->h);
}

void switch_color_theme(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    if(cfg->color_theme < COLOR_THEME_N-1)
        cfg->color_theme++;
    else
        cfg->color_theme=0;
    // 以下函數會產生Expose事件，而處理Expose事件時會更新窗口的文字
    // 內容及其顏色，故此處不必更新構件文字顏色。
    update_taskbar_bg();
    update_menu_bg(act_center, ACT_CENTER_ITEM_N);
    update_menu_bg(client_menu, CLIENT_MENU_ITEM_N);
    update_entry_bg(cmd_entry);
    update_win_bg(xinfo.hint_win, get_widget_color(HINT_WIN_COLOR), None);
    update_clients_bg(wm);
}

void toggle_compositor(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Window win=get_compositor();

    if(win)
        XKillClient(xinfo.display, win);
    else
        exec(wm, e, (Func_arg)SH_CMD((char *)cfg->compositor));
}
