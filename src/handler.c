/* *************************************************************************
 *     handler.c：實現X事件處理功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "clientop.h"
#include "focus.h"
#include "button.h"
#include "bind_cfg.h"
#include "config.h"
#include "entry.h"
#include "func.h"
#include "focus.h"
#include "menu.h"
#include "minimax.h"
#include "mvresize.h"
#include "layout.h"
#include "place.h"
#include "prop.h"
#include "icccm.h"
#include "image.h"
#include "taskbar.h"
#include "widget.h"
#include "desktop.h"
#include "handler.h"

static void handle_event(WM *wm, XEvent *e);
static void handle_button_press(WM *wm, XEvent *e);
static void exec_buttonbind_func(WM *wm, XEvent *e);
static void handle_button_release(WM *wm, XEvent *e);
static bool is_func_click(const Widget_id id, const Buttonbind *b, XEvent *e);
static void handle_client_message(WM *wm, XEvent *e);
static void change_net_wm_state(WM *wm, Client *c, long *full_act);
static void change_net_wm_state_for_modal(WM *wm, Client *c, long act);
static void change_net_wm_state_for_sticky(WM *wm, Client *c, long act);
static void change_net_wm_state_for_shaded(Client *c, long act);
static void change_net_wm_state_for_skip_taskbar(Client *c, long act);
static void change_net_wm_state_for_skip_pager(Client *c, long act);
static void change_net_wm_state_for_above(Client *c, long act);
static void change_net_wm_state_for_below(Client *c, long act);
static void change_net_wm_state_for_attent(WM *wm, Client *c, long act);
static void change_net_wm_state_for_focused(Client *c, long act);
static void activate_win(Window win, unsigned long src);
static void change_desktop(WM *wm, Window win, unsigned int desktop);
static void handle_config_request(WM *wm, XEvent *e);
static void config_managed_client(Client *c);
static void config_unmanaged_win(XConfigureRequestEvent *e);
static void handle_enter_notify(WM *wm, XEvent *e);
static void handle_pointer_hover(WM *wm, const Widget *widget);
static void handle_expose(WM *wm, XEvent *e);
static void handle_focus_in(WM *wm, XEvent *e);
static void handle_focus_out(WM *wm, XEvent *e);
static void handle_key_press(WM *wm, XEvent *e);
static void key_run_cmd(WM *wm, XKeyEvent *e);
static void key_set_color(WM *wm, XKeyEvent *e);
static void handle_leave_notify(WM *wm, XEvent *e);
static void handle_map_request(WM *wm, XEvent *e);
static void handle_unmap_notify(WM *wm, XEvent *e);
static void handle_property_notify(WM *wm, XEvent *e);
static void handle_wm_hints_notify(WM *wm, Window win);
static void handle_wm_icon_name_notify(WM *wm, Window win, Atom atom);
static void update_ui(WM *wm);
static void handle_wm_name_notify(WM *wm, Window win, Atom atom);
static void handle_wm_transient_for_notify(WM *wm, Window win);
static void handle_selection_notify(WM *wm, XEvent *e);

static Event_handler event_handlers[LASTEvent]; // 事件處理器數組

void handle_events(WM *wm)
{
	XEvent e;
    XSync(xinfo.display, False);
    while(run_flag && !XNextEvent(xinfo.display, &e))
        handle_event(wm, &e);
    clear_wm(wm);
}

static void handle_event(WM *wm, XEvent *e)
{
    if(!XFilterEvent(e, None) && event_handlers[e->type])
        event_handlers[e->type](wm, e);
}

void reg_event_handlers(WM *wm)
{
    for(int i=0; i<LASTEvent; i++)
        event_handlers[i]=NULL;
    wm->event_handler=handle_event;

    event_handlers[ButtonPress]      = handle_button_press;
    event_handlers[ButtonRelease]    = handle_button_release;
    event_handlers[ClientMessage]    = handle_client_message;
    event_handlers[ConfigureRequest] = handle_config_request;
    event_handlers[EnterNotify]      = handle_enter_notify;
    event_handlers[Expose]           = handle_expose;
    event_handlers[FocusIn]          = handle_focus_in;
    event_handlers[FocusOut]         = handle_focus_out;
    event_handlers[KeyPress]         = handle_key_press;
    event_handlers[LeaveNotify]      = handle_leave_notify;
    event_handlers[MapRequest]       = handle_map_request;
    event_handlers[UnmapNotify]      = handle_unmap_notify;
    event_handlers[PropertyNotify]   = handle_property_notify;
    event_handlers[SelectionNotify]  = handle_selection_notify;
}

static void handle_button_press(WM *wm, XEvent *e)
{
    Widget *clicked_widget=widget_find(e->xbutton.window),
           *popped_widget=get_popped_widget();

    if(!popped_widget)
        exec_buttonbind_func(wm, e);
    else if(clicked_widget)
    {
        exec_buttonbind_func(wm, e);
        hide_popped_widget(popped_widget, clicked_widget);
    }
    else
    {
        hide_popped_widget(popped_widget, clicked_widget);
        XUngrabPointer(xinfo.display, CurrentTime);
    }
}

static void exec_buttonbind_func(WM *wm, XEvent *e)
{
    Window win=e->xbutton.window;
    Widget *widget=widget_find(win);
    Widget_id id = widget ? widget->id : (win==xinfo.root_win ? ROOT_WIN : UNUSED_WIDGET_ID);
    Client *c=win_to_client(id==CLIENT_ICON ? iconbar_get_client_win(taskbar_get_iconbar(wm->taskbar), win) : win);
    Client *tmc = c ? get_top_transient_client(c->subgroup_leader, true) : NULL;

    if(widget && widget->id!=TITLEBAR && widget->id!=CLIENT_FRAME)
    {
        widget->state.active=1;
        widget->update_bg(widget);
    }
    
    for(const Buttonbind *p=buttonbind; p->func; p++)
    {
        if( is_func_click(id, p, e)
            && (is_drag_func(p->func) || get_valid_click(wm, CHOOSE, e, NULL)))
        {
            if(id == CLIENT_WIN)
                XAllowEvents(xinfo.display, ReplayPointer, CurrentTime);
            if(c && c!=get_cur_focus_client())
                focus_client(c);
            if((!c || !tmc || c==tmc || id==CLIENT_ICON))
                p->func(wm, e, p->arg);
        }
    }
}

static bool is_func_click(const Widget_id id, const Buttonbind *b, XEvent *e)
{
    return (b->widget_id == id
        && b->button == e->xbutton.button
        && is_equal_modifier_mask(b->modifier, e->xbutton.state));
}

static void handle_button_release(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Widget *widget=widget_find(e->xbutton.window);
    if(widget)
    {
        widget->state.active=0;
        widget->update_bg(widget);
    }
}

static void handle_client_message(WM *wm, XEvent *e)
{
    Window win=e->xclient.window;
    Atom type=e->xclient.message_type;
    Client *c=win_to_client(win);

    if(is_spec_ewmh_atom(type, NET_CURRENT_DESKTOP))
        focus_desktop_n(wm, e->xclient.data.l[0]);
    else if(is_spec_ewmh_atom(type, NET_SHOWING_DESKTOP))
        toggle_showing_desktop_mode(e->xclient.data.l[0]);
    else if(c)
    {
        if(is_spec_icccm_atom(type, WM_CHANGE_STATE))
        {
            if(e->xclient.data.l[0] == IconicState)
                iconify_client(get_cur_focus_client()); 
        }
        else if(is_spec_ewmh_atom(type, NET_ACTIVE_WINDOW))
            activate_win(win, e->xclient.data.l[0]);
        if(is_spec_ewmh_atom(type, NET_CLOSE_WINDOW))
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
    if(mask.skip_taskbar)   change_net_wm_state_for_skip_taskbar(c, act);
    if(mask.skip_pager)     change_net_wm_state_for_skip_pager(c, act);
    if(mask.hidden)         change_net_wm_state_for_hidden(c, act);
    if(mask.fullscreen)     change_net_wm_state_for_fullscreen(c, act);
    if(mask.above)          change_net_wm_state_for_above(c, act);
    if(mask.below)          change_net_wm_state_for_below(c, act);
    if(mask.attent)         change_net_wm_state_for_attent(wm, c, act);
    if(mask.focused)        change_net_wm_state_for_focused(c, act);

    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

static void change_net_wm_state_for_modal(WM *wm, Client *c, long act)
{
    UNUSED(wm);
    c->win_state.modal=SHOULD_ADD_STATE(c, act, modal);
}

static void change_net_wm_state_for_sticky(WM *wm, Client *c, long act)
{
    UNUSED(wm);
    bool add=SHOULD_ADD_STATE(c, act, sticky);

    if(add)
        c->desktop_mask=~0U;
    else
        c->desktop_mask=get_desktop_mask(get_net_current_desktop());
    request_layout_update();
    c->win_state.sticky=add;
}

static void change_net_wm_state_for_shaded(Client *c, long act)
{
    toggle_shade_client_mode(c, SHOULD_ADD_STATE(c, act, shaded));
}

static void change_net_wm_state_for_skip_taskbar(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, skip_taskbar);

    if(add && is_iconic_client(c))
        deiconify_client(c);
    c->win_state.skip_taskbar=add;
}

/* 暫未實現分頁器 */
static void change_net_wm_state_for_skip_pager(Client *c, long act)
{
    c->win_state.skip_pager=SHOULD_ADD_STATE(c, act, skip_pager);
}

static void change_net_wm_state_for_above(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, above);

    if(add)
    {
        if(c->place != ABOVE_LAYER)
            save_place_info_of_client(c);
        move_client(c, NULL, ABOVE_LAYER);
    }
    else
        restore_client(c);
    c->win_state.above=add;
}

static void change_net_wm_state_for_below(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, below);

    if(add)
    {
        if(c->place != BELOW_LAYER)
            save_place_info_of_client(c);
        move_client(c, NULL, BELOW_LAYER);
    }
    else
        restore_client(c), c->win_state.below=0;
    c->win_state.below=add;
}

static void change_net_wm_state_for_attent(WM *wm, Client *c, long act)
{
    c->win_state.attent=SHOULD_ADD_STATE(c, act, attent);
    taskbar_set_attention(wm->taskbar, c->desktop_mask);
}

static void change_net_wm_state_for_focused(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, focused);

    focus_client(add ? c : NULL);
    c->win_state.focused=add;
}

static void activate_win(Window win, unsigned long src)
{
    Client *c=win_to_client(win);
    if(!c)
        return;

    if(src == 2) // 源自分頁器
    {
        if(is_on_cur_desktop(c->desktop_mask))
            focus_client(c);
        else
            set_urgency_hint(win, c->wm_hint, true);
    }
    else // 源自應用程序
        set_state_attent(c, true);
}

static void change_desktop(WM *wm, Window win, unsigned int desktop)
{ 
    Client *c=win_to_client(win);

    if(!c || c!=get_cur_focus_client())
        return;

    if(desktop == ~0U)
        attach_to_desktop_all(wm);
    else
        move_to_desktop_n(wm, desktop);
}

static void handle_config_request(WM *wm, XEvent *e)
{
    UNUSED(wm);
    XConfigureRequestEvent cr=e->xconfigurerequest;
    Client *c=win_to_client(cr.window);

    if(c)
        config_managed_client(c);
    else
        config_unmanaged_win(&cr);
}

static void config_managed_client(Client *c)
{
    XConfigureEvent ce=
    {
        .type=ConfigureNotify, .display=xinfo.display, .event=WIDGET_WIN(c),
        .window=WIDGET_WIN(c), .x=WIDGET_X(c), .y=WIDGET_Y(c), .width=WIDGET_W(c), .height=WIDGET_H(c),
        .border_width=0, .above=None, .override_redirect=False
    };
    XSendEvent(xinfo.display, WIDGET_WIN(c), False, StructureNotifyMask, (XEvent *)&ce);
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
    Client *c=win_to_client(win);
    Pointer_act act=NO_OP;
    Move_info m={x, y, 0, 0};
    Widget *widget=widget_find(win);

    if(cfg->focus_mode==ENTER_FOCUS && c)
        focus_client(c);
    if( is_layout_adjust_area(wm, win, x)
        && get_clients_n(MAIN_AREA, false, false, false))
        set_cursor(win, ADJUST_LAYOUT_RATIO);
    if(widget == NULL)
        return;
    if(widget->id == CLIENT_FRAME)
        act=get_resize_act(c, &m);
    else if(widget->id == TITLEBAR)
        act=MOVE;
    else
    {
        if(widget->id == CLOSE_BUTTON)
            widget->state.warn=1;
        widget->state.hot=1;
        widget_update_bg(widget);
    }
    if(widget->id != UNUSED_WIDGET_ID)
        set_cursor(win, act);
    if(widget->tooltip)
        handle_pointer_hover(wm, widget);
}

static void handle_pointer_hover(WM *wm, const Widget *widget)
{
    XEvent ev;
    bool show=false;
    struct timeval t={cfg->hover_time/1000, cfg->hover_time%1000*1000}, t0=t;
    int fd=ConnectionNumber(xinfo.display);
    fd_set fds;

    while(1)
    {
        if(XPending(xinfo.display))
        {
            XNextEvent(xinfo.display, &ev);
                handle_event(wm, &ev);
            if(ev.type == MotionNotify && ev.xmotion.window==widget->win)
                t=t0, show=false;
            else if( (ev.type==LeaveNotify && ev.xcrossing.window==widget->win)
                ||   (ev.type==ButtonPress && ev.xbutton.window==widget->win) )
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
                if(!show && widget->tooltip)
                    show=true, widget->tooltip->show(widget->tooltip);
            }
        }
    }
    if(widget->tooltip)
        widget->tooltip->hide(widget->tooltip);
}

static void handle_expose(WM *wm, XEvent *e)
{
    UNUSED(wm);
    if(e->xexpose.count)
        return;

    Window win=e->xexpose.window;
    Widget *widget=widget_find(win);
    if(widget == NULL)
        return;

    if(widget->id != CLIENT_WIN)
        widget->update_fg(widget);
}

static void handle_focus_in(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Window win=e->xfocus.window;
    Client *c=win_to_client(e->xfocus.window);
    if(!c)
        return;

    if(c->win_state.fullscreen && c->place!=FULLSCREEN_LAYER)
    {
        WIDGET_X(c)=WIDGET_Y(c)=0, WIDGET_W(c)=xinfo.screen_width, WIDGET_H(c)=xinfo.screen_height;
        move_client(c, NULL, FULLSCREEN_LAYER);
    }
    set_urgency_hint(win, c->wm_hint, false);
    set_state_attent(c, false);
}

static void handle_focus_out(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Client *c=win_to_client(e->xfocus.window);
    if(c && c->win_state.fullscreen && c->place==FULLSCREEN_LAYER)
        move_client(c, NULL, c->old_place);
}

static void handle_key_press(WM *wm, XEvent *e)
{
    Widget *widget=widget_find(e->xkey.window);
    if(widget && widget->id==RUN_CMD_ENTRY)
        key_run_cmd(wm, &e->xkey);
    else if(widget && widget->id==COLOR_ENTRY)
        key_set_color(wm, &e->xkey);
    else
    {
        int n;
        KeySym *ks=XGetKeyboardMapping(xinfo.display, e->xkey.keycode, 1, &n);

        for(const Keybind *p=keybind; p->func; p++)
            if(*ks == p->keysym && is_equal_modifier_mask(p->modifier, e->xkey.state))
                p->func(wm, e, p->arg);
        XFree(ks);
    }
}

static void key_run_cmd(WM *wm, XKeyEvent *e)
{
    if(!entry_input(cmd_entry, e))
        return;

    char cmd[BUFSIZ]={0};
    wcstombs(cmd, entry_get_text(cmd_entry), BUFSIZ);
    exec(wm, NULL, (Arg)SH_CMD(cmd));
    entry_clear(cmd_entry);
}

static void key_set_color(WM *wm, XKeyEvent *e)
{
    if(!entry_input(color_entry, e))
        return;

    char color[BUFSIZ]={0};
    wcstombs(color, entry_get_text(color_entry), BUFSIZ);
    set_main_color_name(color);
    update_ui(wm);
    entry_clear(color_entry);
}

static void handle_leave_notify(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Window win=e->xcrossing.window;
    Widget *widget=widget_find(win);
    if(widget == NULL)
        return;

    widget->state.hot=widget->state.warn=0;
    widget->update_bg(widget);
    if(widget->id != UNUSED_WIDGET_ID)
        set_cursor(win, NO_OP);
}

static void handle_map_request(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Window win=e->xmaprequest.window;

    if(is_wm_win(win, false))
        add_client(win);
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
    Client *c=win_to_client(ue->window);

    if( c && ue->window==WIDGET_WIN(c)
        && (ue->send_event|| ue->event==WIDGET_WIN(c->frame) || ue->event==WIDGET_WIN(c)))
    {
        if(is_iconic_client(c))
            taskbar_remove_client(wm->taskbar, WIDGET_WIN(c));
        remove_client(c, false);
    }
}

static void handle_property_notify(WM *wm, XEvent *e)
{
    Window win=e->xproperty.window;
    Client *c=win_to_client(win);
    Atom atom=e->xproperty.atom;

    if(c && cfg->set_frame_prop)
        copy_prop(WIDGET_WIN(c->frame), WIDGET_WIN(c));
    if(atom == XA_WM_HINTS)
        handle_wm_hints_notify(wm, win);
    else if(atom==XA_WM_ICON_NAME || is_spec_ewmh_atom(atom, NET_WM_ICON_NAME))
        handle_wm_icon_name_notify(wm, win, atom);
    else if(atom == XA_WM_NAME || is_spec_ewmh_atom(atom, NET_WM_NAME))
        handle_wm_name_notify(wm, win, atom);
    else if(atom == XA_WM_TRANSIENT_FOR)
        handle_wm_transient_for_notify(wm, win);
    else if(c && is_spec_ewmh_atom(atom, NET_WM_ICON))
    {
        free_image(c->image);
        c->image=get_win_icon_image(win);
        if(c->decorative)
            frame_change_logo(c->frame, c->image);
        if(is_iconic_client(c))
            iconbar_update_by_icon(taskbar_get_iconbar(wm->taskbar), win, c->image);
    }
    else if(c && is_spec_ewmh_atom(atom, NET_WM_STATE))
        iconbar_update_by_state(taskbar_get_iconbar(wm->taskbar), win);
    else if(is_spec_ewmh_atom(atom, NET_CURRENT_DESKTOP)
        || is_spec_gwm_atom(atom, GWM_LAYOUT))
        taskbar_buttons_update_bg_by_chosen(wm->taskbar);
    else if(is_spec_gwm_atom(atom, GWM_UPDATE_LAYOUT))
        update_layout(wm);
    else if(is_spec_gwm_atom(atom, GWM_MAIN_COLOR_NAME))
        update_ui(wm);
}

static void handle_wm_hints_notify(WM *wm, Window win)
{
    Client *c=win_to_client(win);
    if(!c)
        return;

    XWMHints *oh=c->wm_hint, *nh=XGetWMHints(xinfo.display, win);
    if(nh && has_focus_hint(nh) && (!oh || !has_focus_hint(oh)))
        set_state_attent(c, true);
    if(nh && nh->flags & XUrgencyHint)
        taskbar_set_urgency(wm->taskbar, c->desktop_mask);
    taskbar_buttons_update_bg(wm->taskbar);
    if(nh)
        XFree(c->wm_hint), c->wm_hint=nh;
}

static void handle_wm_icon_name_notify(WM *wm, Window win, Atom atom)
{
    char *s=NULL;
    Client *c=win_to_client(win);

    if(!c || !is_iconic_client(c) || !(s=get_text_prop(win, atom)))
        return;

    iconbar_update_by_icon_name(taskbar_get_iconbar(wm->taskbar), win, s);
    Free(s);
}

static void update_ui(WM *wm)
{
    // 以下函數會產生Expose事件，而處理Expose事件時會更新窗口的文字
    // 內容及其顏色，故此處不必更新構件文字顏色。
    char *name=get_main_color_name();
    alloc_color(name);
    Free(name);
    taskbar_update_bg(WIDGET(wm->taskbar));
    menu_update_bg(WIDGET(act_center));
    clients_for_each(c)
        if(c->decorative)
            menu_update_bg(WIDGET(frame_get_menu(c->frame)));
    entry_update_bg(WIDGET(cmd_entry));
    entry_update_bg(WIDGET(color_entry));
    update_win_bg(xinfo.root_win, get_root_color(), None);
    update_clients_bg();
}

static void handle_wm_name_notify(WM *wm, Window win, Atom atom)
{
    char *s=get_text_prop(win, atom);
    Client *c=win_to_client(win);

    if((win!=xinfo.root_win && !c) || !s)
        return;

    if(win == xinfo.root_win)
        statusbar_change_label(taskbar_get_statusbar(wm->taskbar), s);
    else
    {
        Free(c->title_text);
        c->title_text=copy_string(s);
        frame_change_title(c->frame, s);
    }
    Free(s);
}

static void handle_wm_transient_for_notify(WM *wm, Window win)
{
    UNUSED(wm);
    Client *c=win_to_client(win);
    if(c)
        c->owner=win_to_client(get_transient_for(win));
}

static void handle_selection_notify(WM *wm, XEvent *e)
{
    UNUSED(wm);
    Widget *widget=widget_find(e->xselection.requestor);
    if(e->xselection.property==get_utf8_string_atom()
        && widget && (widget->id==RUN_CMD_ENTRY || widget->id==COLOR_ENTRY))
        entry_paste(ENTRY(widget));
}
