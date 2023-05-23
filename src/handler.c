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
static void toggle_showing_desktop_mode(WM *wm, bool show_mode);
static void change_desktop(WM *wm, Window win, unsigned int desktop);
static void update_hint_win_for_info(WM *wm, const char *info);
static void handle_config_request(WM *wm, XEvent *e);
static void config_managed_client(WM *wm, Client *c);
static void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
static void handle_enter_notify(WM *wm, XEvent *e);
static void handle_pointer_hover(WM *wm, Window hover, void (*handler)(WM *, Window));
static void handle_expose(WM *wm, XEvent *e);
static void update_title_area_text(WM *wm, Client *c);
static void update_title_button_text(WM *wm, Client *c, size_t index);
static void handle_key_press(WM *wm, XEvent *e);
static void key_run_cmd(WM *wm, XKeyEvent *e);
static void handle_leave_notify(WM *wm, XEvent *e);
static void hint_leave_title_button(WM *wm, Client *c, Widget_type type);
static void handle_map_request(WM *wm, XEvent *e);
static void handle_unmap_notify(WM *wm, XEvent *e);
static void handle_property_notify(WM *wm, XEvent *e);
static void handle_wm_hints_notify(WM *wm, Window win);
static void handle_wm_icon_name_notify(WM *wm, Window win);
static void handle_wm_hints_notify(WM *wm, Window win);
static void handle_wm_icon_name_notify(WM *wm, Window win);
static void handle_wm_name_notify(WM *wm, Window win);
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
    wm->event_handlers[ClientMessage]    = handle_client_message;;
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
    XWindowAttributes a;
    Window win=e->xbutton.window;
    Widget_type type=get_widget_type(wm, win);
    for(const Buttonbind *b=wm->cfg->buttonbind; b->func; b++)
    {
        if( is_func_click(wm, type, b, e)
            && (is_drag_func(b->func) || get_valid_click(wm, CHOOSE, e, NULL)))
        {
            if( type == CMD_CENTER_ITEM
                && XGetWindowAttributes(wm->display, wm->cmd_center->win, &a)
                && a.map_state==IsViewable)
                XUnmapWindow(wm->display, wm->cmd_center->win);
            else
            {
                focus_clicked_client(wm, win);
                if(b->func)
                    b->func(wm, e, b->arg);
                if(type == CLIENT_WIN)
                    XAllowEvents(wm->display, ReplayPointer, CurrentTime);
            }
        }
    }

    if(type != CMD_CENTER_ITEM)
        XUnmapWindow(wm->display, wm->cmd_center->win);
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
    if(type == wm->ewmh_atom[_NET_NUMBER_OF_DESKTOPS])
        update_hint_win_for_info(wm, "gwm不支持動態更改虛擬桌面數量！");
    else if(type == wm->ewmh_atom[_NET_DESKTOP_GEOMETRY])
        update_hint_win_for_info(wm, "gwm不支持大型桌面，故不支持動態更改虛擬桌面尺寸！");
    else if(type == wm->ewmh_atom[_NET_DESKTOP_VIEWPORT])
        update_hint_win_for_info(wm, "gwm不支持大型桌面，故不支持動態更改當前虛擬桌面視口！");
    if(type == wm->ewmh_atom[_NET_CURRENT_DESKTOP])
        focus_desktop_n(wm, e->xclient.data.l[0]+1);
    else if(type == wm->ewmh_atom[_NET_DESKTOP_NAMES])
        update_hint_win_for_info(wm, "gwm不支持動態更改虛擬桌面名稱！");
    else if(type == wm->ewmh_atom[_NET_ACTIVE_WINDOW])
        activate_win(wm, win, e->xclient.data.l[0]);
    else if(type == wm->ewmh_atom[_NET_SHOWING_DESKTOP])
        toggle_showing_desktop_mode(wm, e->xclient.data.l[0]);
    else if(type == wm->ewmh_atom[_NET_CLOSE_WINDOW])
    {
        // 尚未知道有哪些分頁器支持此特性，誰要是知道的話，煩請告知我
        update_hint_win_for_info(wm, "此分頁器支持_NET_CLOSE_WINDOW特性");
        close_win(wm, win);
    }
    else if(type == wm->ewmh_atom[_NET_MOVERESIZE_WINDOW])
        // 尚未知道有哪些分頁器支持此特性，誰要是知道的話，煩請告知我
        update_hint_win_for_info(wm, "此分頁器支持_NET_MOVERESIZE_WINDOW特性");
    else if(type == wm->ewmh_atom[_NET_WM_MOVERESIZE])
        update_hint_win_for_info(wm, "稍後支持_NET_WM_MOVERESIZE特性，敬請期待！");
    else if(type == wm->ewmh_atom[_NET_RESTACK_WINDOW])
        // 尚未知道有哪些分頁器支持此特性，誰要是知道的話，煩請告知我
        update_hint_win_for_info(wm, "此分頁器支持_NET_RESTACK_WINDOW特性");
    else if(type == wm->ewmh_atom[_NET_REQUEST_FRAME_EXTENTS])
        // 尚未知道有哪些分頁器支持此特性，誰要是知道的話，煩請告知我
        update_hint_win_for_info(wm, "此分頁器支持_NET_REQUEST_FRAME_EXTENTS特性");
    else if(type == wm->ewmh_atom[_NET_WM_DESKTOP])
        change_desktop(wm, win, e->xclient.data.l[0]);
}

static void activate_win(WM *wm, Window win, unsigned long src)
{
    Client *c=NULL;
    if(src==1 && (c=win_to_client(wm, win)) && c!=CUR_FOC_CLI(wm))
        set_urgency(wm, c, true);
    if(src==2 && (c=win_to_client(wm, win)) && is_on_cur_desktop(wm, c))
        focus_client(wm, wm->cur_desktop, c);
}

static void toggle_showing_desktop_mode(WM *wm, bool show_mode)
{
    if(show_mode)
        iconify_all_clients(wm);
    else
        deiconify_all_clients(wm);
    set_net_showing_desktop(wm, show_mode);
}

static void change_desktop(WM *wm, Window win, unsigned int desktop)
{ 
    Client *c=win_to_client(wm, win);
    if(c && c!=wm->clients)
    {
        if(desktop== 0xFFFFFFFF)
            attach_to_desktop_all(wm, c);
        else
            move_to_desktop_n(wm, c, desktop+1);
    }
}

static void update_hint_win_for_info(WM *wm, const char *info)
{
    unsigned int w=0, h=wm->cfg->hint_win_line_height;
    get_string_size(wm, wm->font[HINT_FONT], info, &w, NULL);
    w+=h, h+=h;
    int x=(wm->screen_width-w)/2, y=(wm->screen_height-h)/2;
    XMoveResizeWindow(wm->display, wm->hint_win, x, y, w, h);
    XMapRaised(wm->display, wm->hint_win);
    String_format f={{0, 0, w, h}, CENTER, false, 0,
        wm->text_color[wm->cfg->color_theme][HINT_TEXT_COLOR], HINT_FONT};
    draw_string(wm, wm->hint_win, info, &f);
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
    else if(IS_TASKBAR_BUTTON(type))
        update_win_background(wm, win,
            wm->widget_color[wm->cfg->color_theme][ENTERED_NORMAL_BUTTON_COLOR].pixel, None);
    else if(type == CLIENT_ICON)
        update_win_background(wm, win,
            wm->widget_color[wm->cfg->color_theme][ENTERED_NORMAL_BUTTON_COLOR].pixel, None);
    else if(IS_CMD_CENTER_ITEM(type))
        update_win_background(wm, win,
            wm->widget_color[wm->cfg->color_theme][ENTERED_NORMAL_BUTTON_COLOR].pixel, None);
    else if(type == CLIENT_FRAME)
        act=get_resize_act(c, &m);
    else if(type == TITLE_AREA)
        act=MOVE;
    else if(IS_TITLE_BUTTON(type))
        update_win_background(wm, win, type==CLOSE_BUTTON ?
            wm->widget_color[wm->cfg->color_theme][ENTERED_CLOSE_BUTTON_COLOR].pixel :
            wm->widget_color[wm->cfg->color_theme][ENTERED_NORMAL_BUTTON_COLOR].pixel, None);
    if(type != UNDEFINED)
        XDefineCursor(wm->display, win, wm->cursors[act]);
    handle_pointer_hover(wm, win, show_tooltip);
}

static void handle_pointer_hover(WM *wm, Window hover, void (*handler)(WM *, Window))
{
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
                    handler(wm, hover), done=true;
            }
        }
    }
    XUnmapWindow(wm->display, wm->hint_win);
}

static void handle_expose(WM *wm, XEvent *e)
{
    if(e->xexpose.count)
        return;
    Window win=e->xexpose.window;
    Widget_type type=get_widget_type(wm, win);
    if(type == CLIENT_ICON)
        update_icon_text(wm, win);
    else if(IS_TASKBAR_BUTTON(type))
        update_taskbar_button(wm, type, !e->xexpose.send_event);
    else if(IS_CMD_CENTER_ITEM(type))
        update_cmd_center_button_text(wm, CMD_CENTER_ITEM_INDEX(type));
    else if(type == STATUS_AREA)
        update_status_area_text(wm);
    else if(type == TITLE_AREA)
        update_title_area_text(wm, win_to_client(wm, win));
    else if(IS_TITLE_BUTTON(type))
        update_title_button_text(wm, win_to_client(wm, win),
            TITLE_BUTTON_INDEX(type));
    else if(type == RUN_CMD_ENTRY)
        update_entry_text(wm, wm->run_cmd);
}

static void update_title_area_text(WM *wm, Client *c)
{
    if(c->title_bar_h)
    {
        String_format f={get_title_area_rect(wm, c), CENTER_LEFT, false, 0,
            wm->text_color[wm->cfg->color_theme][c==CUR_FOC_CLI(wm) ?  CURRENT_TITLE_TEXT_COLOR
                : NORMAL_TITLE_TEXT_COLOR], TITLE_FONT};
        draw_string(wm, c->title_area, c->title_text, &f);
    }
}

static void update_title_button_text(WM *wm, Client *c, size_t index)
{
    if(c->title_bar_h)
    {
        String_format f={{0, 0, wm->cfg->title_button_width,
            wm->cfg->title_button_height}, CENTER, false, 0,
            wm->text_color[wm->cfg->color_theme][c==CUR_FOC_CLI(wm) ? CURRENT_TITLE_BUTTON_TEXT_COLOR
                : NORMAL_TITLE_BUTTON_TEXT_COLOR], TITLE_BUTTON_FONT};
        draw_string(wm, c->buttons[index], wm->cfg->title_button_text[index], &f);
    }
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
    if(input_for_entry(wm, wm->run_cmd, e))
    {
        char cmd[BUFSIZ]={0};
        wcstombs(cmd, wm->run_cmd->text, BUFSIZ);
        exec(wm, NULL, (Func_arg)SH_CMD(cmd));
    }
}

static void handle_leave_notify(WM *wm, XEvent *e)
{
    Window win=e->xcrossing.window;
    Widget_type type=get_widget_type(wm, win);
    if(IS_TASKBAR_BUTTON(type))
        hint_leave_taskbar_button(wm, type);
    else if(type == CLIENT_ICON)
        update_win_background(wm, win, wm->widget_color[wm->cfg->color_theme][ICON_AREA_COLOR].pixel, None);
    else if(IS_CMD_CENTER_ITEM(type))
        update_win_background(wm, win, wm->widget_color[wm->cfg->color_theme][CMD_CENTER_COLOR].pixel, None);
    else if(IS_TITLE_BUTTON(type))
        hint_leave_title_button(wm, win_to_client(wm, win), type);
    if(type != UNDEFINED)
        XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
}

static void hint_leave_title_button(WM *wm, Client *c, Widget_type type)
{
    Window win=c->buttons[TITLE_BUTTON_INDEX(type)];
    update_win_background(wm, win, c==CUR_FOC_CLI(wm) ?
        wm->widget_color[wm->cfg->color_theme][CURRENT_TITLE_BUTTON_COLOR].pixel :
        wm->widget_color[wm->cfg->color_theme][NORMAL_TITLE_BUTTON_COLOR].pixel, None);
}

static void handle_map_request(WM *wm, XEvent *e)
{
    Window win=e->xmaprequest.window;
    if(is_wm_win(wm, win, false))
    {
        add_client(wm, win);
        DESKTOP(wm)->default_area_type=wm->cfg->default_area_type;
    }
    else // 不受WM控制的窗口要放在窗口疊頂部才能確保可見，如截圖、通知窗口
        XRaiseWindow(wm->display, win);
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
    if(wm->cfg->set_frame_prop && (c=win_to_client(wm, win)))
        copy_prop(wm, c->frame, c->win);
    switch(e->xproperty.atom)
    {
        case XA_WM_HINTS:
            handle_wm_hints_notify(wm, win); break;
        case XA_WM_ICON_NAME:
            handle_wm_icon_name_notify(wm, win); break;
        case XA_WM_NAME:
            handle_wm_name_notify(wm, win); break;
        case XA_WM_NORMAL_HINTS:
            handle_wm_normal_hints_notify(wm, win); break;
        case XA_WM_TRANSIENT_FOR:
            handle_wm_transient_for_notify(wm, win); break;
        default: break; // 或許其他的情況也應考慮，但暫時還沒遇到必要的情況
    }
}

static void handle_wm_hints_notify(WM *wm, Window win)
{
    Client *c=win_to_client(wm, win);
    if(c)
    {
        XWMHints *oh=c->wm_hint, *nh=XGetWMHints(wm->display, win);
        if( nh && ((nh->flags & InputHint) && nh->input) // 變成需要鍵盤輸入
            && (!oh || !((oh->flags & InputHint) && oh->input)))
            set_input_focus(wm, nh, win);
        if(nh)
            XFree(c->wm_hint), c->wm_hint=nh;
    }
}

static void handle_wm_icon_name_notify(WM *wm, Window win)
{
    char *s=NULL;
    Client *c=win_to_client(wm, win);

    if(c && c->area_type==ICONIFY_AREA)
    {
        if((s=get_text_prop(wm, c->win, XA_WM_ICON_NAME)))
        {
            free(c->icon->title_text);
            c->icon->title_text=s;
            update_icon_area(wm);
        }
    }
}

static void handle_wm_name_notify(WM *wm, Window win)
{
    char *s=NULL;
    Client *c=NULL;

    if( (win==wm->root_win || (c=win_to_client(wm, win)))
        && (s=get_text_prop(wm, win, XA_WM_NAME)))
    {
        if(win == wm->root_win)
        {
            free(wm->taskbar->status_text);
            wm->taskbar->status_text=s;
            update_status_area(wm);
        }
        else
        {
            free(c->title_text);
            c->title_text=s;
            update_title_area_text(wm, c);
        }
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
