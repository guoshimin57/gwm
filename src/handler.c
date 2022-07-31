/* *************************************************************************
 *     handler.c：實現X事件處理功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "handler.h"
#include "client.h"
#include "entry.h"
#include "font.h"
#include "func.h"
#include "grab.h"
#include "layout.h"
#include "misc.h"

static bool is_func_click(WM *wm, Widget_type type, Buttonbind *b, XEvent *e);
static void focus_clicked_client(WM *wm, Window win);
static void config_managed_client(WM *wm, Client *c);
static void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e);
static void hint_enter_taskbar_button(WM *wm, Widget_type type);
static void hint_enter_cmd_center_button(WM *wm, Widget_type type);
static void hint_enter_title_button(WM *wm, Client *c, Widget_type type);
static void hint_resize_client(WM *wm, Client *c, int x, int y);
static void update_icon_text(WM *wm, Window win);
static void update_taskbar_button_text(WM *wm, size_t index);
static void update_cmd_center_button_text(WM *wm, size_t index);
static void update_title_area_text(WM *wm, Client *c);
static void update_title_button_text(WM *wm, Client *c, size_t index);
static void update_status_area_text(WM *wm);
static void update_client_look(WM *wm, unsigned int desktop_n, Client *c);
static void key_run_cmd(WM *wm, XKeyEvent *e);
static void hint_leave_taskbar_button(WM *wm, Widget_type type);
static void hint_leave_cmd_center_button(WM *wm, Widget_type type);
static void hint_leave_title_button(WM *wm, Client *c, Widget_type type);

void handle_events(WM *wm)
{
	XEvent e;
    XSync(wm->display, False);
	while(!XNextEvent(wm->display, &e))
        if(!XFilterEvent(&e, None))
            handle_event(wm, &e);
}

void handle_event(WM *wm, XEvent *e)
{

    static void (*event_handlers[])(WM *, XEvent *)=
    {
        [ButtonPress]       = handle_button_press,
        [ConfigureRequest]  = handle_config_request,
        [EnterNotify]       = handle_enter_notify,
        [Expose]            = handle_expose,
        [FocusIn]           = handle_focus_change,
        [FocusOut]          = handle_focus_change,
        [KeyPress]          = handle_key_press,
        [LeaveNotify]       = handle_leave_notify,
        [MapRequest]        = handle_map_request,
        [MotionNotify]      = handle_motion_notify,
        [UnmapNotify]       = handle_unmap_notify,
        [PropertyNotify]    = handle_property_notify,
        [SelectionNotify]   = handle_selection_notify,
    };
    if(event_handlers[e->type])
        event_handlers[e->type](wm, e);
}

void handle_button_press(WM *wm, XEvent *e)
{
    Buttonbind *b=BUTTONBIND;
    Widget_type type=get_widget_type(wm, e->xbutton.window);
    XUnmapWindow(wm->display, wm->cmd_center.win);
    for(size_t i=0; i<ARRAY_NUM(BUTTONBIND); i++, b++)
    {
        if(is_func_click(wm, type, b, e))
        {
            focus_clicked_client(wm, e->xbutton.window);
            if(b->func)
                b->func(wm, e, b->arg);
            if(type == CLIENT_WIN)
                XAllowEvents(wm->display, ReplayPointer, CurrentTime);
        }
    }
}
 
static bool is_func_click(WM *wm, Widget_type type, Buttonbind *b, XEvent *e)
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

void handle_config_request(WM *wm, XEvent *e)
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

void handle_enter_notify(WM *wm, XEvent *e)
{
    int x=e->xcrossing.x_root, y=e->xcrossing.y_root;
    Window win=e->xcrossing.window;
    Client *c=win_to_client(wm, win);
    Widget_type type=get_widget_type(wm, win);
    if(wm->focus_mode==ENTER_FOCUS && c)
        focus_client(wm, wm->cur_desktop, c);
    if(is_layout_adjust_area(wm, win, x))
        XDefineCursor(wm->display, win, wm->cursors[ADJUST_LAYOUT_RATIO]);
    else if(type == ROOT_WIN)
        XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
    else if(type == STATUS_AREA)
        XDefineCursor(wm->display, wm->taskbar.status_area, wm->cursors[NO_OP]);
    else if(IS_TASKBAR_BUTTON(type))
        hint_enter_taskbar_button(wm, type);
    else if(IS_CMD_CENTER_ITEM(type))
        hint_enter_cmd_center_button(wm, type);
    else if(type == CLIENT_FRAME)
        hint_resize_client(wm, c, x, y);
    else if(type == TITLE_AREA)
        XDefineCursor(wm->display, c->title_area, wm->cursors[MOVE]);
    else if(IS_TITLE_BUTTON(type))
        hint_enter_title_button(wm, c, type);
}

static void hint_enter_taskbar_button(WM *wm, Widget_type type)
{
    Window win=wm->taskbar.buttons[TASKBAR_BUTTON_INDEX(type)];
    XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
    update_win_background(wm, win, wm->widget_color[ENTERED_NORMAL_BUTTON_COLOR].pixel);
}

static void hint_enter_cmd_center_button(WM *wm, Widget_type type)
{
    Window win=wm->cmd_center.items[CMD_CENTER_ITEM_INDEX(type)];
    XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
    update_win_background(wm, win, wm->widget_color[ENTERED_NORMAL_BUTTON_COLOR].pixel);
}

static void hint_enter_title_button(WM *wm, Client *c, Widget_type type)
{
    Window win=c->buttons[TITLE_BUTTON_INDEX(type)];
    XDefineCursor(wm->display, win, wm->cursors[NO_OP]);
    update_win_background(wm, win, type==CLOSE_BUTTON ?
        wm->widget_color[ENTERED_CLOSE_BUTTON_COLOR].pixel :
        wm->widget_color[ENTERED_NORMAL_BUTTON_COLOR].pixel);
}

static void hint_resize_client(WM *wm, Client *c, int x, int y)
{
    Move_info m={x, y, 0, 0};
    XDefineCursor(wm->display, c->frame, wm->cursors[get_resize_act(c, &m)]);
}

void handle_expose(WM *wm, XEvent *e)
{
    if(e->xexpose.count)
        return;
    Window win=e->xexpose.window;
    Widget_type type=get_widget_type(wm, win);
    if(type == CLIENT_ICON)
        update_icon_text(wm, win);
    else if(IS_TASKBAR_BUTTON(type))
        update_taskbar_button_text(wm, TASKBAR_BUTTON_INDEX(type));
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
        update_entry_text(wm, &wm->run_cmd);
}

static void update_icon_text(WM *wm, Window win)
{
    Client *c=win_to_iconic_state_client(wm, win);
    if(c)
    {
        unsigned int w;
        get_string_size(wm, wm->font[ICON_CLASS_FONT], c->class_name, &w, NULL);
        String_format f={{0, 0, w, c->icon->h}, CENTER, false, 0,
            wm->text_color[ICON_CLASS_TEXT_COLOR], ICON_CLASS_FONT};
        draw_string(wm, c->icon->win, c->class_name, &f);
        if(!c->icon->is_short_text)
        {
            String_format f={{w, 0, c->icon->w-w, c->icon->h}, CENTER, false,
                0, wm->text_color[ICON_TITLE_TEXT_COLOR], ICON_TITLE_FONT};
            draw_string(wm, c->icon->win, c->title_text, &f);
        }
    }
}

static void update_taskbar_button_text(WM *wm, size_t index)
{
    String_format f={{0, 0, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT},
        CENTER, false, 0, wm->text_color[TASKBAR_BUTTON_TEXT_COLOR],
        TASKBAR_BUTTON_FONT};
    draw_string(wm, wm->taskbar.buttons[index], TASKBAR_BUTTON_TEXT[index], &f);
}

static void update_cmd_center_button_text(WM *wm, size_t index)
{
    String_format f={{0, 0, CMD_CENTER_ITEM_WIDTH, CMD_CENTER_ITEM_HEIGHT},
        CENTER_LEFT, false, 0, wm->text_color[CMD_CENTER_ITEM_TEXT_COLOR],
        CMD_CENTER_FONT};
    draw_string(wm, wm->cmd_center.items[index], CMD_CENTER_ITEM_TEXT[index], &f);
}

static void update_title_area_text(WM *wm, Client *c)
{
    if(c->title_bar_h)
    {
        String_format f={get_title_area_rect(wm, c), CENTER_LEFT, false, 0,
            wm->text_color[TITLE_AREA_TEXT_COLOR], TITLE_AREA_FONT};
        draw_string(wm, c->title_area, c->title_text, &f);
    }
}

static void update_title_button_text(WM *wm, Client *c, size_t index)
{
    if(c->title_bar_h)
    {
        String_format f={{0, 0, TITLE_BUTTON_WIDTH, TITLE_BUTTON_HEIGHT},
            CENTER, false, 0, wm->text_color[TITLE_BUTTON_TEXT_COLOR],
            TITLE_BUTTON_FONT};
        draw_string(wm, c->buttons[index], TITLE_BUTTON_TEXT[index], &f);
    }
}

static void update_status_area_text(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    String_format f={{0, 0, b->status_area_w, b->h}, CENTER_RIGHT, false, 0,
        wm->text_color[STATUS_AREA_TEXT_COLOR], STATUS_AREA_FONT};
    draw_string(wm, b->status_area, b->status_text, &f);
}

void handle_focus_change(WM *wm, XEvent *e)
{
    Client *c=win_to_client(wm, e->xfocus.window);
    if(!c)
        c=win_to_iconic_state_client(wm, e->xfocus.window);
    if(c)
        update_client_look(wm, wm->cur_desktop, c);
}

static void update_client_look(WM *wm, unsigned int desktop_n, Client *c)
{
    if(c && c!=wm->clients)
    {
        Desktop *d=wm->desktop+desktop_n-1;
        if(c->area_type==ICONIFY_AREA && d->cur_layout!=PREVIEW)
            XSetWindowBorder(wm->display, c->icon->win, c==d->cur_focus_client ?
                wm->widget_color[CURRENT_BORDER_COLOR].pixel :
                wm->widget_color[NORMAL_BORDER_COLOR].pixel);
        else
            update_frame(wm, desktop_n,  c);
    }
}

void handle_key_press(WM *wm, XEvent *e)
{
    Keybind *p=KEYBIND;
    wchar_t keyname[BUFSIZ]={0};
    KeySym ks=look_up_key(0, &e->xkey, keyname, BUFSIZ);

    for(size_t i=0; i<ARRAY_NUM(KEYBIND); i++, p++)
        if( ks == p->keysym
            && is_equal_modifier_mask(wm, p->modifier, e->xkey.state)
            && p->func)
            p->func(wm, e, p->arg);
    if(e->xkey.window == wm->run_cmd.win)
        key_run_cmd(wm, &e->xkey);
}

static void key_run_cmd(WM *wm, XKeyEvent *e)
{
    if(input_for_entry(wm, &wm->run_cmd, e))
    {
        char cmd[BUFSIZ]={0};
        wcstombs(cmd, wm->run_cmd.text, BUFSIZ);
        exec(wm, NULL, (Func_arg)SH_CMD(cmd));
    }
}

void handle_leave_notify(WM *wm, XEvent *e)
{
    Window win=e->xcrossing.window;
    Widget_type type=get_widget_type(wm, win);
    if(IS_TASKBAR_BUTTON(type))
        hint_leave_taskbar_button(wm, type);
    else if(IS_CMD_CENTER_ITEM(type))
        hint_leave_cmd_center_button(wm, type);
    else if(IS_TITLE_BUTTON(type))
        hint_leave_title_button(wm, win_to_client(wm, win), type);
    XUndefineCursor(wm->display, win);
}

static void hint_leave_taskbar_button(WM *wm, Widget_type type)
{
    unsigned long color = is_chosen_button(wm, type) ?
        wm->widget_color[CHOSEN_TASKBAR_BUTTON_COLOR].pixel :
        wm->widget_color[NORMAL_TASKBAR_BUTTON_COLOR].pixel ;
    Window win=wm->taskbar.buttons[TASKBAR_BUTTON_INDEX(type)];
    update_win_background(wm, win, color);
}

static void hint_leave_cmd_center_button(WM *wm, Widget_type type)
{
    Window win=wm->cmd_center.items[CMD_CENTER_ITEM_INDEX(type)];
    update_win_background(wm, win, wm->widget_color[CMD_CENTER_COLOR].pixel);
}

static void hint_leave_title_button(WM *wm, Client *c, Widget_type type)
{
    Window win=c->buttons[TITLE_BUTTON_INDEX(type)];
    update_win_background(wm, win, c==DESKTOP(wm).cur_focus_client ?
        wm->widget_color[CURRENT_TITLE_BUTTON_COLOR].pixel :
        wm->widget_color[NORMAL_TITLE_BUTTON_COLOR].pixel);
}

void handle_map_request(WM *wm, XEvent *e)
{
    Window win=e->xmaprequest.window;
    XMapWindow(wm->display, win);
    if(is_wm_win(wm, win) && !win_to_client(wm, win))
    {
        add_client(wm, win);
        update_layout(wm);
        DESKTOP(wm).default_area_type=DEFAULT_AREA_TYPE;
    }
}

void handle_motion_notify(WM *wm, XEvent *e)
{
    Window win=e->xmotion.window;
    if(win==wm->root_win)
    {
        if(is_layout_adjust_area(wm, win, e->xmotion.x))
            XDefineCursor(wm->display, win, wm->cursors[ADJUST_LAYOUT_RATIO]);
        else
            XUndefineCursor(wm->display, win);
    }
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
 * 原父窗口。銷毀窗口產生UnmapNotify事件時，xunmap.event等於新父窗口。*/
void handle_unmap_notify(WM *wm, XEvent *e)
{
    Client *c=win_to_client(wm, e->xunmap.window);
    if(c && (e->xunmap.event==c->frame || e->xunmap.event==c->win) && e->xunmap.window==c->win)
    {
        XUnmapWindow(wm->display, c->frame);
        del_client(wm, c);
        update_layout(wm);
    }
}

void handle_property_notify(WM *wm, XEvent *e)
{
    Window win=e->xproperty.window;
    char *s;
    if(e->xproperty.atom==XA_WM_NAME && (s=get_text_prop(wm, win, XA_WM_NAME)))
    {
        Client *c;
        if((c=win_to_client(wm, win)) && win==c->win)
        {
            free(c->title_text);
            c->title_text=s;
            update_title_area_text(wm, c);
        }
        else if(win == wm->root_win)
        {
            unsigned int w, bw=TASKBAR_BUTTON_WIDTH*TASKBAR_BUTTON_N;
            Taskbar *b=&wm->taskbar;
            free(b->status_text);
            get_string_size(wm, wm->font[STATUS_AREA_FONT], s, &w, NULL);
            if(w > STATUS_AREA_WIDTH_MAX)
                w=STATUS_AREA_WIDTH_MAX;
            if(w != b->status_area_w)
            {
                XMoveResizeWindow(wm->display, b->status_area, b->w-w, 0, w, b->h);
                XMoveResizeWindow(wm->display, b->icon_area, bw, 0, b->w-bw-w, b->h);
            }
            b->status_text=s;
            b->status_area_w=w;
            update_status_area_text(wm);
        }
    }
}

void handle_selection_notify(WM *wm, XEvent *e)
{
    Window win=e->xselection.requestor;
    if(e->xselection.property==wm->utf8 && win==wm->run_cmd.win)
        paste_for_entry(wm, &wm->run_cmd);
}
