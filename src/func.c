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
#include "mv_resize.h"

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

void move_resize(WM *wm, XEvent *e, Func_arg arg)
{
    if(e->type == KeyPress)
        key_move_resize_client(wm, e, arg.direction);
    else
        pointer_move_resize_client(wm, e, arg.resize);
}

void choose_client(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=CUR_FOC_CLI(wm);

    if(c->icon)
        deiconify(wm, c);

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
    close_font(wm);
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

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm, TILE_LAYER_SECOND, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double mr=d->main_area_ratio+arg.change_ratio, fr=d->fixed_area_ratio;
        long mw=mr*wm->workarea.w, sw=wm->workarea.w*(1-fr)-mw;
        if(sw>=cfg->resize_inc && mw>=cfg->resize_inc)
        {
            d->main_area_ratio=mr;
            request_layout_update();
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{ 
    UNUSED(e), UNUSED(arg);
    if( DESKTOP(wm)->cur_layout==TILE
        && get_clients_n(wm, TILE_LAYER_FIXED, false, false, false))
    {
        Desktop *d=DESKTOP(wm);
        double fr=d->fixed_area_ratio+arg.change_ratio, mr=d->main_area_ratio;
        long mw=wm->workarea.w*(mr-arg.change_ratio), fw=wm->workarea.w*fr;
        if(mw>=cfg->resize_inc && fw>=cfg->resize_inc)
        {
            d->main_area_ratio-=arg.change_ratio, d->fixed_area_ratio=fr;
            request_layout_update();
        }
    }
}

void change_place(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Client *c=CUR_FOC_CLI(wm);

    move_client(wm, c, NULL, arg.place_type);
    update_win_state_for_move_resize(wm, c);
}

void pointer_swap_clients(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
    XEvent ev;
    Layout layout=DESKTOP(wm)->cur_layout;
    Client *from=CUR_FOC_CLI(wm), *to=NULL, *head=wm->clients;
    if(layout!=TILE || from==head || !get_valid_click(wm, SWAP, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    if((to=win_to_client(wm, ev.xbutton.subwindow)))
        swap_clients(wm, from, to);
}

void minimize_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    iconify(wm, CUR_FOC_CLI(wm)); 
}

void deiconify_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    deiconify(wm, CUR_FOC_CLI(wm)); 
}

void max_restore_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    Client *c=CUR_FOC_CLI(wm);

    if(c == wm->clients)
        return;

    if(is_win_state_max(c))
    {
        if(c->win_state.vmax)
            c->win_state.vmax=0;
        if(c->win_state.hmax)
            c->win_state.hmax=0;
        if(c->win_state.tmax)
            c->win_state.tmax=0;
        if(c->win_state.bmax)
            c->win_state.bmax=0;
        if(c->win_state.lmax)
            c->win_state.lmax=0;
        if(c->win_state.rmax)
            c->win_state.rmax=0;
        restore_client(wm, c);
    }
    else
    {
        max_client(wm, c, FULL_MAX);
        c->win_state.vmax=c->win_state.hmax=1;
    }
    update_net_wm_state(c->win, c->win_state);
}

void maximize_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Client *c=CUR_FOC_CLI(wm);

    if(c == wm->clients)
        return;

    max_client(wm, c, arg.max_way);
    switch(arg.max_way)
    {
        case VERT_MAX:   c->win_state.vmax=1; break;
        case HORZ_MAX:   c->win_state.hmax=1; break;
        case TOP_MAX:    c->win_state.tmax=1; break;
        case BOTTOM_MAX: c->win_state.bmax=1; break;
        case LEFT_MAX:   c->win_state.lmax=1; break;
        case RIGHT_MAX:  c->win_state.rmax=1; break;
        case FULL_MAX:   c->win_state.vmax=c->win_state.hmax=1; break;
    }
    update_net_wm_state(c->win, c->win_state);
}

void toggle_shade_client(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool shade=false;

    toggle_shade_client_mode(CUR_FOC_CLI(wm), shade=!shade);
}

void toggle_shade_client_mode(Client *c, bool shade)
{
    if(shade && c->titlebar_h)
        XResizeWindow(xinfo.display, c->frame, c->w, c->titlebar_h);
    else if(!shade)
        XResizeWindow(xinfo.display, c->frame, c->w, c->titlebar_h+c->h);
    c->win_state.shaded=shade;
}

void pointer_change_place(WM *wm, XEvent *e, Func_arg arg)
{
    XEvent ev;
    Client *from=CUR_FOC_CLI(wm), *to;

    UNUSED(arg);
    if( DESKTOP(wm)->cur_layout!=TILE || from==wm->clients
        || !get_valid_click(wm, CHANGE, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    Window win=ev.xbutton.window, subw=ev.xbutton.subwindow;
    to=win_to_client(wm, subw);
    if(ev.xbutton.x == 0)
        move_client(wm, from, NULL, TILE_LAYER_SECOND);
    else if(ev.xbutton.x == (long)xinfo.screen_width-1)
        move_client(wm, from, NULL, TILE_LAYER_FIXED);
    else if(win==xinfo.root_win && subw==None)
        move_client(wm, from, NULL, TILE_LAYER_MAIN);
    else if(to)
        move_client(wm, from, to, ANY_PLACE);
    update_win_state_for_move_resize(wm, from);
}

void change_layout(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    Layout *cl=&DESKTOP(wm)->cur_layout, *pl=&DESKTOP(wm)->prev_layout;

    if(*cl == arg.layout)
        return;

    if(arg.layout == PREVIEW)
        save_place_info_of_clients(wm);
    if(*cl == PREVIEW)
        restore_place_info_of_clients(wm);

    Display *d=xinfo.display;
    *pl=*cl, *cl=arg.layout;
    if(*pl == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->icon)
                XMapWindow(d, c->icon->win), XUnmapWindow(d, c->frame);
    if(*cl == PREVIEW)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->icon)
                XMapWindow(d, c->frame), XUnmapWindow(d, c->icon->win);

    if(*pl==TILE && *cl==STACK)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && is_normal_layer(c->place_type))
                c->place_type=FLOAT_LAYER;

    if(*pl==STACK && *cl==TILE)
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_on_cur_desktop(wm, c) && c->place_type==FLOAT_LAYER)
                c->place_type=TILE_LAYER_MAIN;

    request_layout_update();
    update_titlebar_layout(wm);
    update_taskbar_buttons_bg();
    set_gwm_current_layout(*cl);
}

void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(arg);
    if( DESKTOP(wm)->cur_layout!=TILE
        || !is_layout_adjust_area(wm, e->xbutton.window, e->xbutton.x_root)
        || !grab_pointer(xinfo.root_win, ADJUST_LAYOUT_RATIO))
        return;

    int ox=e->xbutton.x_root, nx, dx;
    XEvent ev;
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, dx=nx-ox;
            if(abs(dx)>=cfg->resize_inc && change_layout_ratio(wm, ox, nx))
                request_layout_update(), ox=nx;
        }
        else
            wm->event_handlers[ev.type](wm, &ev);
    }while(!is_match_button_release(e, &ev));
    XUngrabPointer(xinfo.display, CurrentTime);
}

void show_desktop(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e), UNUSED(arg);
    static bool show=false;

    toggle_showing_desktop_mode(wm, show=!show);
}

void toggle_showing_desktop_mode(WM *wm, bool show)
{
    if(show)
        iconify_all_clients(wm);
    else
        deiconify_all_clients(wm);
    set_net_showing_desktop(show);
}

void change_default_place_type(WM *wm, XEvent *e, Func_arg arg)
{
    UNUSED(e);
    DESKTOP(wm)->default_place_type=arg.place_type;
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
