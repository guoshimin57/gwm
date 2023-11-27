/* *************************************************************************
 *     handler.c：實現X事件處理功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

#define SHOULD_ADD_STATE(c, act, flag) \
    (act==NET_WM_STATE_ADD || (act==NET_WM_STATE_TOGGLE && !c->win_state.flag))

static void ignore_event(WM *wm, XEvent *e);
static void handle_button_press(WM *wm, XEvent *e);
static void unmap_for_click(Widget_type type);
static bool is_func_click(Widget_type type, const Buttonbind *b, XEvent *e);
static void focus_clicked_client(WM *wm, Window win);
static void handle_client_message(WM *wm, XEvent *e);
static void change_net_wm_state(WM *wm, Client *c, long *full_act);
static void change_net_wm_state_for_modal(WM *wm, Client *c, long act);
static void change_net_wm_state_for_sticky(WM *wm, Client *c, long act);
static void change_net_wm_state_for_vmax(WM *wm, Client *c, long act);
static void change_net_wm_state_for_hmax(WM *wm, Client *c, long act);
static void change_net_wm_state_for_tmax(WM *wm, Client *c, long act);
static void change_net_wm_state_for_bmax(WM *wm, Client *c, long act);
static void change_net_wm_state_for_lmax(WM *wm, Client *c, long act);
static void change_net_wm_state_for_rmax(WM *wm, Client *c, long act);
static void change_net_wm_state_for_shaded(Client *c, long act);
static void change_net_wm_state_for_skip_taskbar(WM *wm, Client *c, long act);
static void change_net_wm_state_for_skip_pager(WM *wm, Client *c, long act);
static void change_net_wm_state_for_hidden(WM *wm, Client *c, long act);
static void change_net_wm_state_for_fullscreen(WM *wm, Client *c, long act);
static void change_net_wm_state_for_above(WM *wm, Client *c, long act);
static void change_net_wm_state_for_below(WM *wm, Client *c, long act);
static void change_net_wm_state_for_attent(WM *wm, Client *c, long act);
static void change_net_wm_state_for_focused(WM *wm, Client *c, long act);
static void activate_win(WM *wm, Window win, unsigned long src);
static void change_desktop(WM *wm, Window win, unsigned int desktop);
static void handle_config_request(WM *wm, XEvent *e);
static void config_managed_client(Client *c);
static void config_unmanaged_win(XConfigureRequestEvent *e);
static void handle_enter_notify(WM *wm, XEvent *e);
static void handle_pointer_hover(WM *wm, Window hover, Widget_type type);
static const char *get_tooltip(WM *wm, Window win, Widget_type type);
static void handle_expose(WM *wm, XEvent *e);
static void update_title_area_fg(WM *wm, Client *c);
static void update_title_button_fg(WM *wm, Client *c, size_t index);
static void handle_focus_in(WM *wm, XEvent *e);
static void handle_focus_out(WM *wm, XEvent *e);
static void handle_key_press(WM *wm, XEvent *e);
static void key_run_cmd(WM *wm, XKeyEvent *e);
static void handle_leave_notify(WM *wm, XEvent *e);
static void handle_map_request(WM *wm, XEvent *e);
static void handle_unmap_notify(WM *wm, XEvent *e);
static void handle_property_notify(WM *wm, XEvent *e);
static void handle_wm_hints_notify(WM *wm, Window win);
static void handle_wm_icon_name_notify(WM *wm, Window win, Atom atom);
static void handle_wm_name_notify(WM *wm, Window win, Atom atom);
static void handle_wm_normal_hints_notify(WM *wm, Window win);
static void handle_wm_transient_for_notify(WM *wm, Window win);
static void handle_selection_notify(WM *wm, XEvent *e);

void handle_events(WM *wm)
{
	XEvent e;
    XSync(xinfo.display, False);
    while(run_flag && !XNextEvent(xinfo.display, &e))
        if(!XFilterEvent(&e, None))
            wm->event_handlers[e.type](wm, &e);
    clear_wm(wm);
}

void reg_event_handlers(WM *wm)
{
    for(int i=0; i<LASTEvent; i++)
        wm->event_handlers[i]=ignore_event;

    wm->event_handlers[ButtonPress]      = handle_button_press;
    wm->event_handlers[ClientMessage]    = handle_client_message;
    wm->event_handlers[ConfigureRequest] = handle_config_request;
    wm->event_handlers[EnterNotify]      = handle_enter_notify;
    wm->event_handlers[Expose]           = handle_expose;
    wm->event_handlers[FocusIn]          = handle_focus_in;
    wm->event_handlers[FocusOut]         = handle_focus_out;
    wm->event_handlers[KeyPress]         = handle_key_press;
    wm->event_handlers[LeaveNotify]      = handle_leave_notify;
    wm->event_handlers[MapRequest]       = handle_map_request;
    wm->event_handlers[UnmapNotify]      = handle_unmap_notify;
    wm->event_handlers[PropertyNotify]   = handle_property_notify;
    wm->event_handlers[SelectionNotify]  = handle_selection_notify;
}

static void ignore_event(WM *wm, XEvent *e)
{
    UNUSED(wm), UNUSED(e);
}

static void handle_button_press(WM *wm, XEvent *e)
{
    Window win=e->xbutton.window;
    Client *c=win_to_client(wm, win),
           *tmc = c ? get_top_transient_client(c->subgroup_leader, true) : NULL;
    Widget_type type=get_widget_type(wm, win);

    for(const Buttonbind *b=cfg->buttonbind; b->func; b++)
    {
        if( is_func_click(type, b, e)
            && (is_drag_func(b->func) || get_valid_click(wm, CHOOSE, e, NULL)))
        {
            if(type == CLIENT_WIN)
                XAllowEvents(xinfo.display, ReplayPointer, CurrentTime);
            focus_clicked_client(wm, win);
            if((DESKTOP(wm)->cur_layout==PREVIEW || !c || !tmc || c==tmc) && b->func)
                b->func(wm, e, b->arg);
        }
    }
    unmap_for_click(type);
}

static void unmap_for_click(Widget_type type)
{
    if(type != ACT_CENTER_ITEM)
        XUnmapWindow(xinfo.display, act_center->win);
    if(type != TITLE_LOGO)
        XUnmapWindow(xinfo.display, client_menu->win);
    if(type!=RUN_CMD_ENTRY && type!=RUN_BUTTON)
    {
        XUnmapWindow(xinfo.display, cmd_entry->win);
        XUnmapWindow(xinfo.display, xinfo.hint_win);
    }
}

static bool is_func_click(Widget_type type, const Buttonbind *b, XEvent *e)
{
    return (b->widget_type == type 
        && b->button == e->xbutton.button
        && is_equal_modifier_mask( b->modifier, e->xbutton.state));
}

static void focus_clicked_client(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(c == NULL)
        c=win_to_iconic_state_client(wm, win);
    if(c && c!=CUR_FOC_CLI(wm))
        focus_client(wm, wm->cur_desktop, c);
}

static void handle_client_message(WM *wm, XEvent *e)
{
    Window win=e->xclient.window;
    Atom type=e->xclient.message_type;
    Client *c=win_to_client(wm, win);

    if(is_spec_ewmh_atom(type, NET_CURRENT_DESKTOP))
        focus_desktop_n(wm, e->xclient.data.l[0]+1);
    else if(is_spec_ewmh_atom(type, NET_SHOWING_DESKTOP))
        toggle_showing_desktop_mode(wm, e->xclient.data.l[0]);
    else if(c)
    {
        if(is_spec_ewmh_atom(type, NET_ACTIVE_WINDOW))
            activate_win(wm, win, e->xclient.data.l[0]);
        else if(is_spec_ewmh_atom(type, NET_CLOSE_WINDOW))
            close_win(win);
        else if(is_spec_ewmh_atom(type, NET_WM_DESKTOP))
            change_desktop(wm, win, e->xclient.data.l[0]);
        else if(is_spec_ewmh_atom(type, NET_WM_STATE))
            change_net_wm_state(wm, c, e->xclient.data.l);
    }
}

static void change_net_wm_state(WM *wm, Client *c, long *full_act)
{
    long act=full_act[0];
    if((act!=NET_WM_STATE_REMOVE && act!=NET_WM_STATE_ADD && act!=NET_WM_STATE_TOGGLE))
        return;

    Net_wm_state mask=get_net_wm_state_mask(full_act);

    if(mask.modal)          change_net_wm_state_for_modal(wm, c, act);
    if(mask.sticky)         change_net_wm_state_for_sticky(wm, c, act);
    if(mask.vmax)           change_net_wm_state_for_vmax(wm, c, act);
    if(mask.hmax)           change_net_wm_state_for_hmax(wm, c, act);
    if(mask.tmax)           change_net_wm_state_for_tmax(wm, c, act);
    if(mask.bmax)           change_net_wm_state_for_bmax(wm, c, act);
    if(mask.lmax)           change_net_wm_state_for_lmax(wm, c, act);
    if(mask.rmax)           change_net_wm_state_for_rmax(wm, c, act);
    if(mask.shaded)         change_net_wm_state_for_shaded(c, act);
    if(mask.skip_taskbar)   change_net_wm_state_for_skip_taskbar(wm, c, act);
    if(mask.skip_pager)     change_net_wm_state_for_skip_pager(wm, c, act);
    if(mask.hidden)         change_net_wm_state_for_hidden(wm, c, act);
    if(mask.fullscreen)     change_net_wm_state_for_fullscreen(wm, c, act);
    if(mask.above)          change_net_wm_state_for_above(wm, c, act);
    if(mask.below)          change_net_wm_state_for_below(wm, c, act);
    if(mask.attent)         change_net_wm_state_for_attent(wm, c, act);
    if(mask.focused)        change_net_wm_state_for_focused(wm, c, act);

    update_net_wm_state(c->win, c->win_state);
}

static void change_net_wm_state_for_modal(WM *wm, Client *c, long act)
{
    UNUSED(wm);
    c->win_state.modal=SHOULD_ADD_STATE(c, act, modal);
}

static void change_net_wm_state_for_sticky(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, sticky);

    if(add)
        c->desktop_mask=~0U;
    else
        c->desktop_mask=get_desktop_mask(wm->cur_desktop);
    update_layout(wm);
    c->win_state.sticky=add;
}

static void change_net_wm_state_for_vmax(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, vmax);

    if(add)
        max_client(wm, c, VERT_MAX);
    else
        restore_client(wm, c);
    c->win_state.vmax=add;
}

static void change_net_wm_state_for_hmax(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, hmax);

    if(add)
        max_client(wm, c, HORZ_MAX);
    else
        restore_client(wm, c);
    c->win_state.hmax=add;
}

static void change_net_wm_state_for_tmax(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, tmax);

    if(add)
        max_client(wm, c, TOP_MAX);
    else
        restore_client(wm, c);
    c->win_state.tmax=add;
}

static void change_net_wm_state_for_bmax(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, bmax);

    if(add)
        max_client(wm, c, BOTTOM_MAX);
    else
        restore_client(wm, c);
    c->win_state.bmax=add;
}

static void change_net_wm_state_for_lmax(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, lmax);

    if(add)
        max_client(wm, c, LEFT_MAX);
    else
        restore_client(wm, c);
    c->win_state.lmax=add;
}

static void change_net_wm_state_for_rmax(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, rmax);

    if(add)
        max_client(wm, c, RIGHT_MAX);
    else
        restore_client(wm, c);
    c->win_state.rmax=add;
}

static void change_net_wm_state_for_shaded(Client *c, long act)
{
    toggle_shade_client_mode(c, SHOULD_ADD_STATE(c, act, shaded));
}

static void change_net_wm_state_for_skip_taskbar(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, skip_taskbar);

    if(add && c->icon)
        deiconify(wm, c);
    c->win_state.skip_taskbar=add;
}

/* 暫未實現分頁器 */
static void change_net_wm_state_for_skip_pager(WM *wm, Client *c, long act)
{
    UNUSED(wm);
    c->win_state.skip_pager=SHOULD_ADD_STATE(c, act, skip_pager);
}

static void change_net_wm_state_for_hidden(WM *wm, Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hidden))
        iconify(wm, c);
    else
        deiconify(wm, c->icon ? c : NULL);
}

static void change_net_wm_state_for_fullscreen(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, fullscreen);

    if(add)
    {
        save_place_info_of_client(c);
        c->x=c->y=0, c->w=xinfo.screen_width, c->h=xinfo.screen_height;
        move_client(wm, c, NULL, FULLSCREEN_LAYER);
    }
    else
        restore_client(wm, c);
    c->win_state.fullscreen=add;
}

static void change_net_wm_state_for_above(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, above);

    if(add)
    {
        if(c->place_type != ABOVE_LAYER)
            save_place_info_of_client(c);
        move_client(wm, c, NULL, ABOVE_LAYER);
    }
    else
        restore_client(wm, c);
    c->win_state.above=add;
}

static void change_net_wm_state_for_below(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, below);

    if(add)
    {
        if(c->place_type != BELOW_LAYER)
            save_place_info_of_client(c);
        move_client(wm, c, NULL, BELOW_LAYER);
    }
    else
        restore_client(wm, c), c->win_state.below=0;
    c->win_state.below=add;
}

static void change_net_wm_state_for_attent(WM *wm, Client *c, long act)
{
    c->win_state.attent=SHOULD_ADD_STATE(c, act, attent);
    update_taskbar_buttons_bg(wm);
}

static void change_net_wm_state_for_focused(WM *wm, Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, focused);

    if(add)
        focus_client(wm, wm->cur_desktop, c);
    else
        focus_client(wm, wm->cur_desktop, NULL);
    c->win_state.focused=add;
}

static void activate_win(WM *wm, Window win, unsigned long src)
{
    Client *c=win_to_client(wm, win);
    if(!c)
        return;

    if(src == 2) // 源自分頁器
    {
        if(is_on_cur_desktop(wm, c))
            focus_client(wm, wm->cur_desktop, c);
        else
            set_urgency(c->win, c->wm_hint, true);
    }
    else // 源自應用程序
        set_attention(wm, c, true);
}

static void change_desktop(WM *wm, Window win, unsigned int desktop)
{ 
    Client *c=win_to_client(wm, win);

    if(!c || c==wm->clients)
        return;

    if(desktop == 0xFFFFFFFF)
        attach_to_desktop_all(wm, c);
    else
        move_to_desktop_n(wm, c, desktop+1);
}

static void handle_config_request(WM *wm, XEvent *e)
{
    XConfigureRequestEvent cr=e->xconfigurerequest;
    Client *c=win_to_client(wm, cr.window);

    if(c)
        config_managed_client(c);
    else
        config_unmanaged_win(&cr);
}

static void config_managed_client(Client *c)
{
    XConfigureEvent ce=
    {
        .type=ConfigureNotify, .display=xinfo.display, .event=c->win,
        .window=c->win, .x=c->x, .y=c->y, .width=c->w, .height=c->h,
        .border_width=0, .above=None, .override_redirect=False
    };
    XSendEvent(xinfo.display, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

static void config_unmanaged_win(XConfigureRequestEvent *e)
{
    XWindowChanges wc=
    {
        .x=e->x, .y=e->y, .width=e->width, .height=e->height,
        .border_width=e->border_width, .sibling=e->above, .stack_mode=e->detail
    };
    XConfigureWindow(xinfo.display, e->window, e->value_mask, &wc);
}

static void handle_enter_notify(WM *wm, XEvent *e)
{
    int x=e->xcrossing.x_root, y=e->xcrossing.y_root;
    Window win=e->xcrossing.window;
    Widget_type type=get_widget_type(wm, win);
    Client *c=win_to_client(wm, win);
    Pointer_act act=NO_OP;
    Move_info m={x, y, 0, 0};

    if(cfg->focus_mode==ENTER_FOCUS && c)
        focus_client(wm, wm->cur_desktop, c);
    if( is_layout_adjust_area(wm, win, x)
        && get_clients_n(wm, TILE_LAYER_MAIN, false, false, false))
        act=ADJUST_LAYOUT_RATIO;
    else if(IS_BUTTON(type))
        update_win_bg(win, get_widget_color(type==CLOSE_BUTTON ?
            ENTERED_CLOSE_BUTTON_COLOR : ENTERED_NORMAL_BUTTON_COLOR), None);
    else if(type == CLIENT_FRAME)
        act=get_resize_act(c, &m);
    else if(type == TITLE_AREA)
        act=MOVE;
    if(type != UNDEFINED)
        XDefineCursor(xinfo.display, win, wm->cursors[act]);
    handle_pointer_hover(wm, win, type);
}

static void handle_pointer_hover(WM *wm, Window hover, Widget_type type)
{
    const char *tooltip=get_tooltip(wm, hover, type);

    if(!tooltip)
        return;

    XEvent ev;
    bool done=false;
    struct timeval t={cfg->hover_time/1000, cfg->hover_time%1000*1000}, t0=t;
    int fd=ConnectionNumber(xinfo.display);
    fd_set fds;

    while(1)
    {
        if(XPending(xinfo.display))
        {
            XNextEvent(xinfo.display, &ev);
                wm->event_handlers[ev.type](wm, &ev);
            if(ev.type == MotionNotify && ev.xmotion.window==hover)
                XUnmapWindow(xinfo.display, xinfo.hint_win), t=t0, done=false;
            else if(ev.type==LeaveNotify && ev.xcrossing.window==hover)
                break;
        }
        else
        {
            FD_ZERO(&fds); FD_SET(fd, &fds);
            // 需要注意的是，只有linux和部分unix會爲select設置最後一個參數爲剩餘時間
            if(select(fd+1, &fds, 0, 0, &t) == -1)
                break;
            if(!t.tv_sec && !t.tv_usec)
            {
                t=t0;
                if(!done)
                    update_hint_win_for_info(hover, tooltip), done=true;
            }
        }
    }
    XUnmapWindow(xinfo.display, xinfo.hint_win);
}

static const char *get_tooltip(WM *wm, Window win, Widget_type type)
{
    switch(type)
    {
        case CLIENT_ICON: return win_to_iconic_state_client(wm, win)->icon->title_text;
        case TITLE_AREA: return win_to_client(wm, win)->title_text;
        default: return cfg->tooltip[type];
    }
}

static void handle_expose(WM *wm, XEvent *e)
{
    if(e->xexpose.count)
        return;

    Window win=e->xexpose.window;
    Client *c=win_to_client(wm, win);
    Widget_type type=get_widget_type(wm, win);

    if(type == CLIENT_ICON)
        update_client_icon_fg(wm, win);
    else if(IS_WIDGET_CLASS(type, TASKBAR_BUTTON))
        update_taskbar_button_fg(type);
    else if(IS_WIDGET_CLASS(type, ACT_CENTER_ITEM))
        update_menu_item_fg(act_center, WIDGET_INDEX(type, ACT_CENTER_ITEM));
    else if(IS_WIDGET_CLASS(type, CLIENT_MENU_ITEM))
        update_menu_item_fg(client_menu, WIDGET_INDEX(type, CLIENT_MENU_ITEM));
    else if(type == STATUS_AREA)
        update_status_area_fg();
    else if(type == TITLE_LOGO)
        draw_icon(c->logo, c->image, c->class_name, c->titlebar_h);
    else if(type == TITLE_AREA)
        update_title_area_fg(wm, c);
    else if(IS_WIDGET_CLASS(type, TITLE_BUTTON))
        update_title_button_fg(wm, c, WIDGET_INDEX(type, TITLE_BUTTON));
    else if(type == RUN_CMD_ENTRY)
        update_entry_text(cmd_entry);
}

static void update_title_area_fg(WM *wm, Client *c)
{
    if(c->titlebar_h <= 0)
        return;

    Rect r=get_title_area_rect(wm, c);
    Text_color id = (c==CUR_FOC_CLI(wm)) ?
        CURRENT_TITLEBAR_TEXT_COLOR : NORMAL_TITLEBAR_TEXT_COLOR;
    Str_fmt f={0, 0, r.w, r.h, CENTER, true, false, 0,
        get_text_color(id)};
    draw_string(c->title_area, c->title_text, &f);
}

static void update_title_button_fg(WM *wm, Client *c, size_t index)
{
    if(c->titlebar_h <= 0)
        return;

    int w=cfg->title_button_width, h=get_font_height_by_pad();
    Text_color id = (c==CUR_FOC_CLI(wm)) ?
        CURRENT_TITLEBAR_TEXT_COLOR : NORMAL_TITLEBAR_TEXT_COLOR;
    Str_fmt f={0, 0, w, h, CENTER, false, false, 0,
        get_text_color(id)};
    draw_string(c->buttons[index], cfg->title_button_text[index], &f);
}

static void handle_focus_in(WM *wm, XEvent *e)
{
    Client *c=win_to_client(wm, e->xfocus.window);
    if(c && c->win_state.fullscreen && c->place_type!=FULLSCREEN_LAYER)
    {
        c->x=c->y=0, c->w=xinfo.screen_width, c->h=xinfo.screen_height;
        move_client(wm, c, NULL, FULLSCREEN_LAYER);
    }
}

static void handle_focus_out(WM *wm, XEvent *e)
{
    Client *c=win_to_client(wm, e->xfocus.window);
    if(c && c->win_state.fullscreen && c->place_type==FULLSCREEN_LAYER)
        move_client(wm, c, NULL, c->old_place_type);
}

static void handle_key_press(WM *wm, XEvent *e)
{
    if(e->xkey.window == cmd_entry->win)
        key_run_cmd(wm, &e->xkey);
    else
    {
        int n;
        KeySym *ks=XGetKeyboardMapping(xinfo.display, e->xkey.keycode, 1, &n);

        for(const Keybind *kb=cfg->keybind; kb->func; kb++)
            if( *ks == kb->keysym
                && is_equal_modifier_mask(kb->modifier, e->xkey.state)
                && kb->func)
                kb->func(wm, e, kb->arg);
        XFree(ks);
    }
}

static void key_run_cmd(WM *wm, XKeyEvent *e)
{
    if(!input_for_entry(cmd_entry, e))
        return;

    char cmd[BUFSIZ]={0};
    wcstombs(cmd, cmd_entry->text, BUFSIZ);
    exec(wm, NULL, (Func_arg)SH_CMD(cmd));
}

static void handle_leave_notify(WM *wm, XEvent *e)
{
    Window win=e->xcrossing.window;
    Widget_type type=get_widget_type(wm, win);
    Client *c=win_to_client(wm, win);

    if(IS_WIDGET_CLASS(type, TASKBAR_BUTTON))
        update_taskbar_button_bg(wm, type);
    else if(type == CLIENT_ICON)
        update_win_bg(win, get_widget_color(TASKBAR_COLOR), None);
    else if(IS_MENU_ITEM(type))
        update_win_bg(win, get_widget_color(MENU_COLOR), None);
    else if(type == TITLE_LOGO)
        update_win_bg(win, get_widget_color(c==CUR_FOC_CLI(wm) ?
            CURRENT_TITLEBAR_COLOR : NORMAL_TITLEBAR_COLOR), None);
    else if(IS_WIDGET_CLASS(type, TITLE_BUTTON))
        update_win_bg(win, get_widget_color(c==CUR_FOC_CLI(wm) ?
            CURRENT_TITLEBAR_COLOR : NORMAL_TITLEBAR_COLOR), None);
    if(type != UNDEFINED)
        XDefineCursor(xinfo.display, win, wm->cursors[NO_OP]);
}

static void handle_map_request(WM *wm, XEvent *e)
{
    Window win=e->xmaprequest.window;

    if(is_wm_win(wm, win, false))
    {
        add_client(wm, win);
        DESKTOP(wm)->default_place_type=TILE_LAYER_MAIN;
    }
    else
        restack_win(wm, win);
    XMapWindow(xinfo.display, win);
}

/* 對已經映射的窗口重設父窗口會依次執行以下操作：
 *     1、自動解除映射該窗口，原父窗口可以收到该UnmapNotify事件；
 *     2、把該窗口從窗口層次結構中移走；
 *     3、把該窗口插入到新父窗口的子窗口堆疊頂部；
 *     4、產生ReparentNotify事件；
 *     5、自動重新映射該窗口，新父窗口可以收到该MapNotify事件。
 * 因爲只在接收MapRequest事件時才考慮添加client，而在接受MapNotify事件時沒
 * 考慮，從而保證不會重復添加，所以相應地，重設父窗口產生UnmapNotify事件時
 * ，也不重復刪除client。重設父窗口產生UnmapNotify事件時，xunmap.event等於
 * 原父窗口。銷毀窗口產生UnmapNotify事件時，xunmap.event等於新父窗口。若通
 * 過SendEvent請求來產生UnmapNotify事件（此時xunmap.send_event的值爲true）
 * ，xunmap.event就有可能是原父窗口、新父窗口、根窗口，等等。*/
static void handle_unmap_notify(WM *wm, XEvent *e)
{
    XUnmapEvent *ue=&e->xunmap;
    Client *c=win_to_client(wm, ue->window);

    if( c && ue->window==c->win
        && (ue->send_event|| ue->event==c->frame || ue->event==c->win))
        del_client(wm, c, false);
}

static void handle_property_notify(WM *wm, XEvent *e)
{
    Window win=e->xproperty.window;
    Client *c=win_to_client(wm, win);
    Atom atom=e->xproperty.atom;

    if(c && cfg->set_frame_prop)
        copy_prop(c->frame, c->win);
    if(atom == XA_WM_HINTS)
        handle_wm_hints_notify(wm, win);
    else if(atom==XA_WM_ICON_NAME || is_spec_ewmh_atom(atom, NET_WM_ICON_NAME))
        handle_wm_icon_name_notify(wm, win, atom);
    else if(atom == XA_WM_NAME || is_spec_ewmh_atom(atom, NET_WM_NAME))
        handle_wm_name_notify(wm, win, atom);
    else if(atom == XA_WM_NORMAL_HINTS)
        handle_wm_normal_hints_notify(wm, win);
    else if(atom == XA_WM_TRANSIENT_FOR)
        handle_wm_transient_for_notify(wm, win);
    else if(c && is_spec_ewmh_atom(atom, NET_WM_ICON))
    {
        c->image=get_icon_image(c->win, c->wm_hint, c->class_hint.res_name,
            cfg->icon_image_size, cfg->cur_icon_theme);
        draw_image(c->image, c->logo, 0, 0, c->titlebar_h, c->titlebar_h);
        if(c->icon)
            draw_image(c->image, c->icon->win, 0, 0, c->titlebar_h, c->titlebar_h);
    }
}

static void handle_wm_hints_notify(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(!c)
        return;

    XWMHints *oh=c->wm_hint, *nh=XGetWMHints(xinfo.display, win);
    if(nh && has_focus_hint(nh) && (!oh || !has_focus_hint(oh)))
        set_attention(wm, c, true);
    update_taskbar_buttons_bg(wm);
    if(nh)
        XFree(c->wm_hint), c->wm_hint=nh;
}

static void handle_wm_icon_name_notify(WM *wm, Window win, Atom atom)
{
    char *s=NULL;
    Client *c=win_to_client(wm, win);

    if(!c || !c->icon || !(s=get_text_prop(c->win, atom)))
        return;

    free(c->icon->title_text);
    c->icon->title_text=s;
    update_icon_area(wm);
}

static void handle_wm_name_notify(WM *wm, Window win, Atom atom)
{
    char *s=get_text_prop(win, atom);
    Client *c=win_to_client(wm, win);

    if((win!=xinfo.root_win && !c) || !s)
        return;

    if(win == xinfo.root_win)
    {
        free(taskbar->status_text);
        taskbar->status_text=s;
        update_icon_status_area();
    }
    else
    {
        free(c->title_text);
        c->title_text=s;
        update_title_area_fg(wm, c);
    }
}

static void handle_wm_normal_hints_notify(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(c)
    {
        update_size_hint(c->win, cfg->resize_inc, &c->size_hint);
        if( DESKTOP(wm)->cur_layout!=TILE
            || (c->place_type!=FULLSCREEN_LAYER && !is_tile_client(wm, c)))
            fix_win_rect(wm, c), update_layout(wm);
    }
}

static void handle_wm_transient_for_notify(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(c)
        c->owner=win_to_client(wm, get_transient_for(win));
}

static void handle_selection_notify(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Window win=e->xselection.requestor;
    if( is_spec_icccm_atom(e->xselection.property, UTF8_STRING)
        && win==cmd_entry->win)
        paste_for_entry(cmd_entry);
}
