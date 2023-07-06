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

static void ignore_event(WM *wm, XEvent *e);
static void handle_button_press(WM *wm, XEvent *e);
static bool is_func_click(WM *wm, Widget_type type, const Buttonbind *b, XEvent *e);
static void focus_clicked_client(WM *wm, Window win);
static void handle_client_message(WM *wm, XEvent *e);
static void activate_win(WM *wm, Window win, unsigned long src);
static void change_desktop(WM *wm, Window win, unsigned int desktop);
static void handle_config_request(WM *wm, XEvent *e);
static void config_managed_client(WM *wm, Client *c);
static void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
static void handle_enter_notify(WM *wm, XEvent *e);
static void handle_pointer_hover(WM *wm, Window hover, Widget_type type);
static const char *get_tooltip(WM *wm, Window win, Widget_type type);
static void handle_expose(WM *wm, XEvent *e);
static void update_title_logo_fg(WM *wm, Client *c);
static void update_title_area_fg(WM *wm, Client *c);
static void update_title_button_fg(WM *wm, Client *c, size_t index);
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
    XSync(wm->display, False);
    while(run_flag && !XNextEvent(wm->display, &e))
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
    Widget_type type=get_widget_type(wm, win);
    for(const Buttonbind *b=wm->cfg->buttonbind; b->func; b++)
    {
        if( is_func_click(wm, type, b, e)
            && (is_drag_func(b->func) || get_valid_click(wm, CHOOSE, e, NULL)))
        {
            focus_clicked_client(wm, win);
            if(b->func)
                b->func(wm, e, b->arg);
            if(type == CLIENT_WIN)
                XAllowEvents(wm->display, ReplayPointer, CurrentTime);
        }
    }

    if(type != ACT_CENTER_ITEM)
        XUnmapWindow(wm->display, wm->act_center->win);
    if(type != TITLE_LOGO)
        XUnmapWindow(wm->display, wm->client_menu->win);
    if(type!=RUN_CMD_ENTRY && type!=RUN_BUTTON)
    {
        XUnmapWindow(wm->display, wm->run_cmd->win);
        XUnmapWindow(wm->display, wm->hint_win);
    }
}
 
static bool is_func_click(WM *wm, Widget_type type, const Buttonbind *b, XEvent *e)
{
    return (b->widget_type == type 
        && b->button == e->xbutton.button
        && is_equal_modifier_mask(wm, b->modifier, e->xbutton.state));
}

static void focus_clicked_client(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(c == NULL)
        c=win_to_iconic_state_client(wm, win);
    if(c)
        focus_client(wm, wm->cur_desktop, c);
}

static void handle_client_message(WM *wm, XEvent *e)
{
    Window win=e->xclient.window;
    Atom type=e->xclient.message_type;
    if(type == wm->ewmh_atom[NET_CURRENT_DESKTOP])
        focus_desktop_n(wm, e->xclient.data.l[0]+1);
    else if(type == wm->ewmh_atom[NET_ACTIVE_WINDOW])
        activate_win(wm, win, e->xclient.data.l[0]);
    else if(type == wm->ewmh_atom[NET_SHOWING_DESKTOP])
        toggle_showing_desktop_mode(wm, e->xclient.data.l[0]);
    else if(type == wm->ewmh_atom[NET_WM_DESKTOP])
        change_desktop(wm, win, e->xclient.data.l[0]);
    else if(type == wm->ewmh_atom[NET_CLOSE_WINDOW])
        close_win(wm, win);
}

static void activate_win(WM *wm, Window win, unsigned long src)
{
    Client *c=NULL;
    if(src==1 && (c=win_to_client(wm, win)) && c!=CUR_FOC_CLI(wm))
        set_urgency(wm, c, true);
    if(src==2 && (c=win_to_client(wm, win)) && is_on_cur_desktop(wm, c))
        focus_client(wm, wm->cur_desktop, c);
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
        config_managed_client(wm, c);
    else
        config_unmanaged_win(wm, &cr);
}

static void config_managed_client(WM *wm, Client *c)
{
    XConfigureEvent ce=
    {
        .type=ConfigureNotify, .display=wm->display, .event=c->win,
        .window=c->win, .x=c->x, .y=c->y, .width=c->w, .height=c->h,
        .border_width=0, .above=None, .override_redirect=False
    };
    XSendEvent(wm->display, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

static void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e)
{
    XWindowChanges wc=
    {
        .x=e->x, .y=e->y, .width=e->width, .height=e->height,
        .border_width=e->border_width, .sibling=e->above, .stack_mode=e->detail
    };
    XConfigureWindow(wm->display, e->window, e->value_mask, &wc);
}

static void handle_enter_notify(WM *wm, XEvent *e)
{
    int x=e->xcrossing.x_root, y=e->xcrossing.y_root;
    Window win=e->xcrossing.window;
    Widget_type type=get_widget_type(wm, win);
    Client *c=win_to_client(wm, win);
    Pointer_act act=NO_OP;
    Move_info m={x, y, 0, 0};

    if(wm->cfg->focus_mode==ENTER_FOCUS && c)
        focus_client(wm, wm->cur_desktop, c);
    if(is_layout_adjust_area(wm, win, x) && get_typed_clients_n(wm, MAIN_AREA))
        act=ADJUST_LAYOUT_RATIO;
    else if(IS_BUTTON(type))
        update_win_bg(wm, win, ENTERED_NCLOSE_BUTTON_COLOR(wm, type), None);
    else if(type == CLIENT_FRAME)
        act=get_resize_act(c, &m);
    else if(type == TITLE_AREA)
        act=MOVE;
    if(type != UNDEFINED)
        XDefineCursor(wm->display, win, wm->cursors[act]);
    handle_pointer_hover(wm, win, type);
}

static void handle_pointer_hover(WM *wm, Window hover, Widget_type type)
{
    const char *tooltip=get_tooltip(wm, hover, type);

    if(!tooltip)
        return;

    XEvent ev;
    bool done=false;
    struct timeval t={wm->cfg->hover_time/1000, wm->cfg->hover_time%1000*1000}, t0=t;
    int fd=ConnectionNumber(wm->display);
    fd_set fds;

    while(1)
    {
        if(XPending(wm->display))
        {
            XNextEvent(wm->display, &ev);
                wm->event_handlers[ev.type](wm, &ev);
            if(ev.type == MotionNotify && ev.xmotion.window==hover)
                XUnmapWindow(wm->display, wm->hint_win), t=t0, done=false;
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
                    update_hint_win_for_info(wm, hover, tooltip), done=true;
            }
        }
    }
    XUnmapWindow(wm->display, wm->hint_win);
}

static const char *get_tooltip(WM *wm, Window win, Widget_type type)
{
    switch(type)
    {
        case CLIENT_ICON: return win_to_iconic_state_client(wm, win)->icon->title_text;
        case TITLE_AREA: return win_to_client(wm, win)->title_text;
        default: return wm->cfg->tooltip[type];
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
        update_taskbar_button_fg(wm, type);
    else if(IS_MENU_ITEM(type))
        update_menu_item_fg(wm, win);
    else if(type == STATUS_AREA)
        update_status_area_fg(wm);
    else if(type == TITLE_LOGO)
        update_title_logo_fg(wm, c);
    else if(type == TITLE_AREA)
        update_title_area_fg(wm, c);
    else if(IS_WIDGET_CLASS(type, TITLE_BUTTON))
        update_title_button_fg(wm, c, WIDGET_INDEX(type, TITLE_BUTTON));
    else if(type == RUN_CMD_ENTRY)
        update_entry_text(wm, wm->run_cmd);
}

static void update_title_logo_fg(WM *wm, Client *c)
{
    Icon *i=c->icon;
    if(c->icon->image)
        draw_image(wm, i->image, c->logo, 0, 0, c->titlebar_h, c->titlebar_h);
    else
    {
        String_format f={{0, 0, c->titlebar_h, c->titlebar_h}, CENTER, true,
            false, false, 0, TEXT_COLOR(wm, CLASS), CLASS_FONT};
        draw_string(wm, c->logo, c->class_name, &f);
    }
}

static void update_title_area_fg(WM *wm, Client *c)
{
    if(c->titlebar_h <= 0)
        return;

    Rect r=get_title_area_rect(wm, c);
    String_format f={{0, 0, r.w, r.h}, CENTER, true, true, false, 0,
        CLI_TEXT_COLOR(wm, c, TITLEBAR), TITLEBAR_FONT};
    draw_string(wm, c->title_area, c->title_text, &f);
}

static void update_title_button_fg(WM *wm, Client *c, size_t index)
{
    if(c->titlebar_h <= 0)
        return;

    int w=wm->cfg->title_button_width, h=TITLEBAR_HEIGHT(wm);
    String_format f={{0, 0, w, h}, CENTER, true, false, false, 0,
        CLI_TEXT_COLOR(wm, c, TITLEBAR), TITLEBAR_FONT};
    draw_string(wm, c->buttons[index], wm->cfg->title_button_text[index], &f);
}

static void handle_key_press(WM *wm, XEvent *e)
{
    if(e->xkey.window == wm->run_cmd->win)
        key_run_cmd(wm, &e->xkey);
    else
    {
        int n;
        KeySym *ks=XGetKeyboardMapping(wm->display, e->xkey.keycode, 1, &n);

        for(const Keybind *kb=wm->cfg->keybind; kb->func; kb++)
            if( *ks == kb->keysym
                && is_equal_modifier_mask(wm, kb->modifier, e->xkey.state)
                && kb->func)
                kb->func(wm, e, kb->arg);
        XFree(ks);
    }
}

static void key_run_cmd(WM *wm, XKeyEvent *e)
{
    if(!input_for_entry(wm, wm->run_cmd, e))
        return;

    char cmd[BUFSIZ]={0};
    wcstombs(cmd, wm->run_cmd->text, BUFSIZ);
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
        update_win_bg(wm, win, WIDGET_COLOR(wm, TASKBAR), None);
    else if(IS_MENU_ITEM(type))
        update_win_bg(wm, win, WIDGET_COLOR(wm, MENU), None);
    else if(type == TITLE_LOGO)
        update_win_bg(wm, win, CLI_WIDGET_COLOR(wm, c, TITLEBAR), None);
    else if(IS_WIDGET_CLASS(type, TITLE_BUTTON))
        update_win_bg(wm, win, CLI_WIDGET_COLOR(wm, c, TITLEBAR), None);
    if(type != UNDEFINED)
        XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
}

static void handle_map_request(WM *wm, XEvent *e)
{
    Window win=e->xmaprequest.window;

    if(is_wm_win(wm, win, false))
    {
        add_client(wm, win);
        DESKTOP(wm)->default_area_type=wm->cfg->default_area_type;
    }
    restack_win(wm, win);
    XMapWindow(wm->display, win);
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
        && (ue->send_event || ue->event==c->frame || ue->event==c->win))
        del_client(wm, c, false);
}

static void handle_property_notify(WM *wm, XEvent *e)
{
    Window win=e->xproperty.window;
    Client *c=NULL;
    Atom atom=e->xproperty.atom;

    if(wm->cfg->set_frame_prop && (c=win_to_client(wm, win)))
        copy_prop(wm, c->frame, c->win);
    if(atom == XA_WM_HINTS)
        handle_wm_hints_notify(wm, win);
    else if(atom==XA_WM_ICON_NAME || atom==wm->ewmh_atom[NET_WM_ICON_NAME])
        handle_wm_icon_name_notify(wm, win, atom);
    else if(atom == XA_WM_NAME || atom==wm->ewmh_atom[NET_WM_NAME])
        handle_wm_name_notify(wm, win, atom);
    else if(atom == XA_WM_NORMAL_HINTS)
        handle_wm_normal_hints_notify(wm, win);
    else if(atom == XA_WM_TRANSIENT_FOR)
        handle_wm_transient_for_notify(wm, win);
}

static void handle_wm_hints_notify(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(!c)
        return;

    XWMHints *oh=c->wm_hint, *nh=XGetWMHints(wm->display, win);
    if( nh && ((nh->flags & InputHint) && nh->input) // 變成需要鍵盤輸入
        && (!oh || !((oh->flags & InputHint) && oh->input)))
        set_input_focus(wm, nh, win);
    if(nh)
        XFree(c->wm_hint), c->wm_hint=nh;
}

static void handle_wm_icon_name_notify(WM *wm, Window win, Atom atom)
{
    char *s=NULL;
    Client *c=win_to_client(wm, win);

    if(!c || c->area_type!=ICONIFY_AREA || !(s=get_text_prop(wm, c->win, atom)))
        return;

    free(c->icon->title_text);
    c->icon->title_text=s;
    update_icon_area(wm);
}

static void handle_wm_name_notify(WM *wm, Window win, Atom atom)
{
    char *s=get_text_prop(wm, win, atom);
    Client *c=win_to_client(wm, win);

    if((win!=wm->root_win && !c) || !s)
        return;

    if(win == wm->root_win)
    {
        free(wm->taskbar->status_text);
        wm->taskbar->status_text=s;
        update_icon_status_area(wm);
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
        update_size_hint(wm, c);
}

static void handle_wm_transient_for_notify(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(c)
        c->owner=get_transient_for(wm, win);
}

static void handle_selection_notify(WM *wm, XEvent *e)
{
    Window win=e->xselection.requestor;
    if(e->xselection.property==wm->utf8 && win==wm->run_cmd->win)
        paste_for_entry(wm, wm->run_cmd);
}
