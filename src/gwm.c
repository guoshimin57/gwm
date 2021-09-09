/* *************************************************************************
 *     gwm.c：實現窗口管理器的主要部分。
 *     版權 (C) 2021 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "gwm.h"

void (*event_handlers[])(WM *, XEvent *)=
{
    [ButtonPress]       = handle_button_press,
    [ConfigureRequest]  = handle_config_request,
    [EnterNotify]       = handle_enter_notify,
    [Expose]            = handle_expose,
    [KeyPress]          = handle_key_press,
    [LeaveNotify]       = handle_leave_notify,
    [MapRequest]        = handle_map_request,
    [UnmapNotify]       = handle_unmap_notify,
    [PropertyNotify]    = handle_property_notify,
};

int main(int argc, char *argv[])
{
    WM wm;
    init_wm(&wm);
    set_wm(&wm);
    handle_events(&wm);
    return EXIT_SUCCESS;
}

void exit_with_msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void init_wm(WM *wm)
{
    memset(wm, 0, sizeof(WM));
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fprintf(stderr, "warning: no locale support\n");
	if(!(wm->display=XOpenDisplay(NULL)))
        exit_with_msg("error: cannot open display");
    wm->screen=DefaultScreen(wm->display);
    wm->screen_width=DisplayWidth(wm->display, wm->screen);
    wm->screen_height=DisplayHeight(wm->display, wm->screen);
	wm->mod_map=XGetModifierMapping(wm->display);
    wm->root_win=RootWindow(wm->display, wm->screen);
    wm->gc=XCreateGC(wm->display, wm->root_win, 0, NULL);
    wm->cur_layout=wm->prev_layout=DEFAULT_LAYOUT;
    wm->main_area_ratio=DEFAULT_MAIN_AREA_RATIO;
    wm->fixed_area_ratio=DEFAULT_FIXED_AREA_RATIO;
    wm->n_main_max=DEFAULT_N_MAIN_MAX;
}

void set_wm(WM *wm)
{
    XSetErrorHandler(my_x_error_handler);
    XSelectInput(wm->display, wm->root_win, ROOT_EVENT_MASK);
    create_font_set(wm);
    create_cursors(wm);
    create_taskbar(wm);
    create_clients(wm);
    update_layout(wm);
    grab_keys(wm);
    exec(wm, NULL, (Func_arg)SH_CMD("[ -x "AUTOSTART" ] && "AUTOSTART));
}

int my_x_error_handler(Display *display, XErrorEvent *e)
{
    if( e->request_code == X_ChangeWindowAttributes
        && e->error_code == BadAccess)
        exit_with_msg("錯誤：已經有其他窗口管理器在運行！");
    print_fatal_msg(display, e);
	if( e->error_code == BadWindow
        || (e->request_code==X_ConfigureWindow && e->error_code==BadMatch))
		return -1;
    return 0;
}

void create_font_set(WM *wm)
{
    char **list, *str;
    int n;
    wm->font_set=XCreateFontSet(wm->display, FONT_SET, &list, &n, &str);
}

void create_cursors(WM *wm)
{
    wm->cursors[NO_OP]=None;
    create_cursor(wm, MOVE, XC_fleur);
    create_cursor(wm, HORIZ_RESIZE, XC_sb_h_double_arrow);
    create_cursor(wm, VERT_RESIZE, XC_sb_v_double_arrow);
    create_cursor(wm, TOP_LEFT_RESIZE, XC_top_left_corner);
    create_cursor(wm, TOP_RIGHT_RESIZE, XC_top_right_corner);
    create_cursor(wm, BOTTOM_LEFT_RESIZE, XC_bottom_left_corner);
    create_cursor(wm, BOTTOM_RIGHT_RESIZE, XC_bottom_right_corner);
    create_cursor(wm, ADJUST_LAYOUT_RATIO, XC_cross);
}

void create_cursor(WM *wm, Pointer_act act, unsigned int shape)
{
    wm->cursors[act]=XCreateFontCursor(wm->display, shape);
}

void create_taskbar(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    b->x=0, b->y=wm->screen_height-TASKBAR_HEIGHT;
    b->w=wm->screen_width, b->h=TASKBAR_HEIGHT;
    b->win=XCreateSimpleWindow(wm->display, wm->root_win, b->x, b->y,
        b->w, b->h, 0, 0, TASKBAR_COLOR);
    XSelectInput(wm->display, b->win, TASKBAR_EVENT_MASK);
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        b->buttons[i]=XCreateSimpleWindow(wm->display, b->win,
            TASKBAR_BUTTON_WIDTH*i, 0,
            TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT,
            0, 0, TASKBAR_BUTTON_COLOR);
        XSelectInput(wm->display, b->buttons[i], BUTTON_EVENT_MASK);
    }
    create_status_area(wm);
    XMapRaised(wm->display, b->win);
    XMapSubwindows(wm->display, b->win);
}

void create_status_area(WM *wm)
{
    int x=TASKBAR_BUTTON_WIDTH*TASKBAR_BUTTON_N;
    unsigned int w=wm->taskbar.w-x;
    char *s=get_text_prop(wm, wm->root_win, XA_WM_NAME);
    wm->taskbar.status_win=XCreateSimpleWindow(wm->display, wm->taskbar.win,
        x, 0, w, wm->taskbar.h, 0, 0, STATUS_AREA_COLOR);
    wm->taskbar.status_text=s ? s : "gwm";
    XSelectInput(wm->display, wm->taskbar.status_win, ExposureMask);
}

void print_fatal_msg(Display *display, XErrorEvent *e)
{
    fprintf(stderr, "X錯誤：資源號=%#lx, 請求量=%lu, 錯誤碼=%d, "
        "主請求碼=%d, 次請求碼=%d\n", e->resourceid, e->serial,
        e->error_code, e->request_code, e->minor_code);
}

/* 生成帶表頭結點的雙向循環鏈表 */
void create_clients(WM *wm)
{
    Window root, parent, *child=NULL;
    unsigned int n;

    wm->cur_focus_client=wm->prev_focus_client=wm->clients=malloc_s(sizeof(Client));
    memset(wm->clients, 0, sizeof(Client));
    wm->clients->win=wm->root_win;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(!XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &n))
        exit_with_msg("錯誤：查詢窗口清單失敗！");
    for(size_t i=0; i<n; i++)
        if(is_wm_win(wm, child[i]))
            add_client(wm, child[i]);
    XFree(child);
}

void *malloc_s(size_t size)
{
    void *p=malloc(size);
    if(p == NULL)
        exit_with_msg("錯誤：申請內存失敗");
    return p;
}

bool is_wm_win(WM *wm, Window win)
{
    XWindowAttributes attr;
    return ( win != wm->taskbar.win
        && XGetWindowAttributes(wm->display, win, &attr)
        && attr.map_state != IsUnmapped
        && !attr.override_redirect);
}

void add_client(WM *wm, Window win)
{
    Client *c;
    char *s=get_text_prop(wm, win, XA_WM_NAME);
    c=malloc_s(sizeof(Client));
    c->win=win;
    memset(&c->icon, 0, sizeof(c->icon));
    c->title_text=s ? s : "";
    apply_rules(wm, c);
    add_client_node(get_area_head(wm, c->place_type), c);
    wm->clients_n[c->place_type]++;
    set_default_rect(wm, c);
    frame_client(wm, c);
    focus_client(wm, c);
    grab_buttons(wm, c);
    XSelectInput(wm->display, win, PropertyChangeMask);
    if(c->place_type == ICONIFY)
        iconify(wm, c);
}

Client *get_area_head(WM *wm, Place_type type)
{
    Client *head=wm->clients;
    for(int i=0; i<type; i++)
        for(int j=0; j<wm->clients_n[i]; j++)
            head=head->next;
    return head;
}

void update_layout(WM *wm)
{
    if(wm->clients == wm->clients->next)
        return;

    switch(wm->cur_layout)
    {
        case FULL: set_full_layout(wm); break;
        case PREVIEW: set_preview_layout(wm); break;
        case STACK: return;
        case TILE: set_tile_layout(wm); break;
    }
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        move_resize_client(wm, c, 0, 0, 0, 0);
}

void iconify_all_for_vision(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(c->place_type == ICONIFY)
        {
            XMapWindow(wm->display, c->icon.win);
            XUnmapWindow(wm->display, c->frame);
            if(c == wm->cur_focus_client)
            {
                focus_client(wm, NULL);
                update_frame(wm, c);
            }
        }
    }
}

void deiconify_all_for_vision(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(c->place_type == ICONIFY)
        {
            XMapWindow(wm->display, c->frame);
            XUnmapWindow(wm->display, c->icon.win);
            if(c == wm->cur_focus_client)
                focus_client(wm, c);
        }
    }
}

void set_full_layout(WM *wm)
{
    Client *c=wm->cur_focus_client;
    c->x=c->y=0, c->w=wm->screen_width, c->h=wm->screen_height;
}

void set_preview_layout(WM *wm)
{
    int n=get_clients_n(wm), i=n-1, rows, cols, w, h,
        ch=(wm->screen_height-wm->taskbar.h);
    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=n && cols*cols<n; cols++)
        ;
    rows=(cols-1)*cols>=n ? cols-1 : cols;
    w=wm->screen_width/cols, h=ch/rows;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev, i--)
    {
        c->x=(i%cols)*w, c->y=(i/cols)*h;
        /* 下邊和右邊的窗口佔用剩餘空間 */
        c->w=(i+1)%cols ? w : w+(wm->screen_width-w*cols);
        c->h=i<cols*(rows-1) ? h : h+(ch-h*rows);
        fix_win_rect(c);
    }
}

unsigned int get_clients_n(WM *wm)
{
    unsigned int n=0;
    for(int i=0; i<PLACE_TYPE_N; i++)
        n+=wm->clients_n[i];
    return n;
}

void set_tile_layout(WM *wm)
{
    Client *c;
    unsigned int *n=wm->clients_n, i, j, mw, sw, fw, mh, sh, fh, h;

    mw=wm->main_area_ratio*wm->screen_width;
    fw=wm->screen_width*wm->fixed_area_ratio;
    sw=wm->screen_width-fw-mw;
    h=wm->screen_height-wm->taskbar.h;
    mh=n[NORMAL]>=wm->n_main_max ? h/wm->n_main_max :
        (n[NORMAL] ? h/n[NORMAL] : h);
    fh=n[FIXED] ? h/n[FIXED] : h;
    sh=n[NORMAL]>wm->n_main_max ? h/(n[NORMAL]-wm->n_main_max) : h;
    if(n[FIXED] == 0) mw+=fw, fw=0;
    if(n[NORMAL] <= wm->n_main_max) mw+=sw, sw=0;

    for(i=0, j=0, c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(c->place_type == FIXED)
            c->x=mw+sw, c->y=i++*fh, c->w=fw, c->h=fh;
        else if(c->place_type == NORMAL)
        {
            if(j < wm->n_main_max)
                c->x=sw, c->y=j*mh, c->w=mw, c->h=mh;
            else
                c->x=0, c->y=(j-wm->n_main_max)*sh, c->w=sw, c->h=sh;
            j++;
        }
        if(c->place_type==NORMAL || c->place_type==FIXED)
            fix_win_rect(c);
    }
}

void grab_keys(WM *wm)
{
    unsigned int num_lock_mask=get_num_lock_mask(wm);
    unsigned int masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    KeyCode code;
    XUngrabKey(wm->display, AnyKey, AnyModifier, wm->root_win);
    for(size_t i=0; i<ARRAY_NUM(KEYBINDS); i++)
        if((code=XKeysymToKeycode(wm->display, KEYBINDS[i].keysym)))
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabKey(wm->display, code, KEYBINDS[i].modifier|masks[j],
                    wm->root_win, True, GrabModeAsync, GrabModeAsync);
}

unsigned int get_num_lock_mask(WM *wm)
{
    size_t i, j;
	XModifierKeymap *m=XGetModifierMapping(wm->display);
    KeyCode code=XKeysymToKeycode(wm->display, XK_Num_Lock);
    if(code)
        for(i=0; i<8; i++)
            for(j=0; j<m->max_keypermod; j++)
                if(m->modifiermap[i*m->max_keypermod+j] == code)
                    break;
    if(j == m->max_keypermod)
        return 0;
    XFreeModifiermap(m);
    return (1<<i);
}
    
void grab_buttons(WM *wm, Client *c)
{
    unsigned int num_lock_mask=get_num_lock_mask(wm),
                 masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    Buttonbind *p=BUTTONBINDS;
    XUngrabButton(wm->display, AnyButton, AnyModifier, c->win);
    for(size_t i=0; i<ARRAY_NUM(BUTTONBINDS); i++, p++)
    {
        if(p->click_type == CLICK_WIN)
        {
            int m=is_equal_modifier_mask(wm, 0, p->modifier) ?
                GrabModeSync : GrabModeAsync;
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabButton(wm->display, p->button, p->modifier|masks[j],
                    c->win, False, BUTTON_MASK, m, m, None, None);
        }
    }
}

void handle_events(WM *wm)
{
	XEvent e;
    XSync(wm->display, False);
	while(!XNextEvent(wm->display, &e))
        if(event_handlers[e.type])
            event_handlers[e.type](wm, &e);
}

void handle_button_press(WM *wm, XEvent *e)
{
    Buttonbind *b=BUTTONBINDS;
    Click_type type=get_click_type(wm, e->xbutton.window);
    Client *c=win_to_client(wm, e->xbutton.window);
    if(c)
        focus_client(wm, c);
    for(size_t i=0; i<ARRAY_NUM(BUTTONBINDS); i++, b++)
    {
        if(is_func_click(wm, type, b, e))
        {
            if(b->func)
                b->func(wm, e, b->arg);
            if(type == CLICK_WIN)
                XAllowEvents(wm->display, ReplayPointer, CurrentTime);
            break;
        }
    }
    if(is_click_client_in_preview(wm, type))
        choose_client_in_preview(wm, c);
}
 
bool is_func_click(WM *wm, Click_type type, Buttonbind *b, XEvent *e)
{
    return (b->click_type == type 
        && b->button == e->xbutton.button
        && is_equal_modifier_mask(wm, b->modifier, e->xbutton.state));
}

bool is_equal_modifier_mask(WM *wm, unsigned int m1, unsigned int m2)
{
    return (get_valid_mask(wm, m1) == get_valid_mask(wm, m2));
}

bool is_click_client_in_preview(WM *wm, Click_type type)
{
    return ((type==CLICK_WIN || type==CLICK_FRAME || type==CLICK_TITLE)
        && wm->cur_layout==PREVIEW);
}

void choose_client_in_preview(WM *wm, Client *c)
{
    for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
    {
        if(p->place_type==ICONIFY && p==c)
        {
            update_client_n_and_place_type(wm, p, NORMAL);
            memset(&p->icon, 0, sizeof(p->icon));
        }
    }
    focus_client(wm, c);
    change_layout(wm, NULL, (Func_arg){.layout=wm->prev_layout});
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

void config_managed_client(WM *wm, Client *c)
{
    XConfigureEvent ce;
    ce.type=ConfigureNotify;
    ce.display=wm->display;
    ce.event=c->win;
    ce.window=c->win;
    ce.x=c->x;
    ce.y=c->y;
    ce.width=c->w;
    ce.height=c->h;
    ce.border_width=0;
    ce.above=None;
    ce.override_redirect=False;
    XSendEvent(wm->display, c->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void config_unmanaged_win(WM *wm, XConfigureRequestEvent *e)
{
    XWindowChanges wc;
    wc.x=e->x;
    wc.y=e->y;
    wc.width=e->width;
    wc.height=e->height;
    wc.border_width=e->border_width;
    wc.sibling=e->above;
    wc.stack_mode=e->detail;
    XConfigureWindow(wm->display, e->window, e->value_mask, &wc);
}

Client *win_to_client(WM *wm, Window win)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(win==c->win || win==c->frame || win==c->title_area)
            return c;
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            if(win == c->buttons[i])
                return c;
    }
        
    return NULL;
}

void del_client(WM *wm, Client *c)
{
    if(c)
    {
        del_client_node(c);
        wm->clients_n[c->place_type]--;
        free(c);
        focus_client(wm, NULL);
    }
}

void handle_expose(WM *wm, XEvent *e)
{
    Window win=e->xexpose.window;
    if(update_taskbar_button_text(wm, win))
        return;
    if(win == wm->taskbar.status_win)
        update_status_area_text(wm);
    else
        for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(!update_title_button_text(wm, c, win))
                update_title_area_text(wm, c);
}

bool update_taskbar_button_text(WM *wm, Window win)
{
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        if(win == wm->taskbar.buttons[i])
        {
            draw_string(wm, win, TASKBAR_BUTTON_TEXT_COLOR, CENTER, 0, 0,
                TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT, TASKBAR_BUTTON_TEXT[i]);
            return true;
        }
    }
    return false;
}

void handle_key_press(WM *wm, XEvent *e)
{
    int n;
    KeyCode kc=e->xkey.keycode;
	KeySym *keysym=XGetKeyboardMapping(wm->display, kc, 1, &n);
    Keybind *p=KEYBINDS;
	for(size_t i=0; i<ARRAY_NUM(KEYBINDS); i++, p++)
    {
		if( *keysym == p->keysym
            && is_equal_modifier_mask(wm, p->modifier, e->xkey.state)
            && p->func)
        {
            p->func(wm, e, p->arg);
            break;
        }
    }
    XFree(keysym);
}
 
unsigned int get_valid_mask(WM *wm, unsigned int mask)
{
    return (mask & ~(LockMask|get_modifier_mask(wm, XK_Num_Lock))
        & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask));
}

unsigned int get_modifier_mask(WM *wm, KeySym key_sym)
{
    KeyCode kc;
    if((kc=XKeysymToKeycode(wm->display, key_sym)) != 0)
    {
        for(size_t i=0; i<8*wm->mod_map->max_keypermod; i++)
            if(wm->mod_map->modifiermap[i] == kc)
                return 1 << (i/wm->mod_map->max_keypermod);
        fprintf(stderr, "錯誤：找不到指定的鍵符號相應的功能轉換鍵！\n");
    }
    else
        fprintf(stderr, "錯誤：指定的鍵符號不存在對應的鍵代碼！\n");
    return 0;
}

void handle_map_request(WM *wm, XEvent *e)
{
    Window win=e->xmaprequest.window;
    XMapWindow(wm->display, win);
    if(is_wm_win(wm, win) && !win_to_client(wm, win))
    {
        add_client(wm, win);
        update_layout(wm);
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
    if(c && e->xunmap.event==c->frame && e->xunmap.window==c->win)
    {
        XUnmapWindow(wm->display, c->frame);
        if(c->icon.win != None)
            XDestroyWindow(wm->display, c->icon.win);
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
        if(win == wm->root_win)
        {
            wm->taskbar.status_text=s;
            update_status_area_text(wm);
        }
        else if((c=win_to_client(wm, win)) && win==c->win)
        {
            c->title_text=s;
            update_title_area_text(wm, c);
        }
    }
}

char *get_text_prop(WM *wm, Window win, Atom atom)
{
    int n;
    char **list=NULL;
    XTextProperty name;
    if(XGetTextProperty(wm->display, win, &name, atom))
    {
        if(name.encoding == XA_STRING)
            return (char *)name.value;
        if(XmbTextPropertyToTextList(wm->display, &name, &list, &n) == Success)
            return *list;
    }
    return NULL;
}

void draw_string(WM *wm, Drawable drawable, unsigned long color, Direction d, int x, int y, unsigned int w, unsigned h, const char *str)
{
    if(str && str[0]!='\0')
    {
        int x_start;
        XRectangle ink, logical;
        XSetForeground(wm->display, wm->gc, color);
        XmbTextExtents(wm->font_set, str, strlen(str), &ink, &logical);
        if(d == RIGHT)
            x_start=x+w-logical.width;
        else if(d == CENTER)
            x_start=x+w/2-logical.width/2;
        else x_start=0;
        XClearArea(wm->display, drawable, 0, 0, w, h, False); 
        XmbDrawString(wm->display, drawable, wm->font_set, wm->gc,
            x_start, y+h/2+logical.height/2, str, strlen(str));
    }
}

void exec(WM *wm, XEvent *e, Func_arg arg)
{
	if(fork() == 0)
    {
		if(wm->display && close(ConnectionNumber(wm->display)))
            perror("命令錯誤地繼承了其父進程打開的文件的描述符：");
		if(!setsid())
            perror("未能成功地爲命令創建新會話：");
		execvp(arg.cmd[0], arg.cmd);
		perror("命令執行錯誤：");
		exit(EXIT_FAILURE);
    }
}

void key_move_resize_client(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout==TILE || wm->cur_layout==STACK)
    {
        int s=MOVE_RESIZE_INC;
        int d[][4]={      /* dx, dy, dw, dh */
            [UP]          = { 0, -s,  0,  0},
            [DOWN]        = { 0,  s,  0,  0},
            [LEFT]        = {-s,  0,  0,  0},
            [RIGHT]       = { s,  0,  0,  0},
            [LEFT2LEFT]   = {-s,  0,  s,  0},
            [LEFT2RIGHT]  = { s,  0, -s,  0},
            [RIGHT2LEFT]  = { 0,  0, -s,  0},
            [RIGHT2RIGHT] = { 0,  0,  s,  0},
            [UP2UP]       = { 0, -s,  0,  s},
            [UP2DOWN]     = { 0,  s,  0, -s},
            [DOWN2UP]     = { 0,  0,  0, -s},
            [DOWN2DOWN]   = { 0,  0,  0,  s},
        };
        int *p=d[arg.direction];
        Client *c=wm->cur_focus_client;
        if(wm->cur_layout == TILE)
            to_floating_area(wm, wm->cur_focus_client);
        if(is_valid_move_resize(wm, c, p[0], p[1], p[2], p[3]))
        {
            if(p[2]>=0 || (p[2]<0 && c->w>=-p[2]+s))
                c->x+=p[0], c->w+=p[2];
            if(p[3]>=0 || (p[3]<0 && c->h>=-p[3]+s))
                c->y+=p[1], c->h+=p[3];
            move_resize_client(wm, c, 0, 0, 0, 0);
        }
    }
}

/* 通過求窗口與屏幕是否有交集來判斷窗口是否已經在屏幕外，即是否合法。
 * 若滿足以下條件，則有交集：窗口與屏幕中心距≤窗口半邊長+屏幕半邊長。
 * 即：|x+w/2-0-sw/2|＜|w/2+sw/2| 且 |y+h/2-0-sh/2|＜|h/2+sh/2|。
 * 兩邊同乘以2，得：|2*x+w-sw|＜|w+sw| 且 |2*y+h-sh|＜|h+sh|。
 */
bool is_valid_move_resize(WM *wm, Client *c, int dx, int dy, int dw, int dh)
{
    int x=c->x+dx, y=c->y+dy, w=c->w+dw, h=c->h+dh,
        sw=wm->screen_width, sh=wm->screen_height;
    return w>0 && h>0 && abs(2*x+w-sw)<w+sw && abs(2*y+h-sh)<h+sh;
}

void quit_wm(WM *wm, XEvent *e, Func_arg unused)
{
    XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
    XClearWindow(wm->display, wm->root_win);
    XFlush(wm->display);
    XCloseDisplay(wm->display);
    exit(EXIT_SUCCESS);
}

void close_win(WM *wm, XEvent *e, Func_arg unused)
{
    if(wm->cur_focus_client != wm->clients)
    {   /* 刪除窗口會產生UnmapNotify事件，處理該事件時再刪除框架 */
        if(!send_event(wm, XInternAtom(wm->display, "WM_DELETE_WINDOW", False)))
            XDestroyWindow(wm->display, wm->cur_focus_client->win);
    }
    else
        fprintf(stderr, "錯誤：不能關閉根窗口！\n");
}

int send_event(WM *wm, Atom protocol)
{
	int i, n;
	Atom *protocols;
	XEvent event;

	if(XGetWMProtocols(wm->display, wm->cur_focus_client->win, &protocols, &n))
    {
        for(i=0; i<n && protocols[i]!=protocol; i++)
            ;
		XFree(protocols);
	}
	if(i < n)
    {
		event.type=ClientMessage;
		event.xclient.window=wm->cur_focus_client->win;
		event.xclient.message_type=XInternAtom(wm->display, "WM_PROTOCOLS", False);
		event.xclient.format=32;
		event.xclient.data.l[0]=protocol;
		event.xclient.data.l[1]=CurrentTime;
		XSendEvent(wm->display, wm->cur_focus_client->win, False, NoEventMask, &event);
	}
	return i<n;
}

void next_client(WM *wm, XEvent *e, Func_arg unused)
{
    if(wm->clients != wm->clients->next) /* 允許切換至根窗口 */
    {
        focus_client(wm, wm->cur_focus_client->prev);
        Client *c=wm->cur_focus_client;
        warp_pointer_for_key_press(wm, c, e->type);
    }
}

void prev_client(WM *wm, XEvent *e, Func_arg unused)
{
    if(wm->clients != wm->clients->next) /* 允許切換至根窗口 */
    {
        focus_client(wm, wm->cur_focus_client->next);
        Client *c=wm->cur_focus_client;
        warp_pointer_for_key_press(wm, c, e->type);
    }
}

void focus_client(WM *wm, Client *c)
{
    if(!c)
        fix_focus_client(wm);
    else if(c != wm->cur_focus_client)
        wm->prev_focus_client=wm->cur_focus_client, wm->cur_focus_client=c;

    Client *pp=wm->prev_focus_client, *pc=wm->cur_focus_client;
    if(pp != wm->clients)
    {
        if(pp->place_type == ICONIFY && wm->cur_layout != PREVIEW)
            unframe_icon(wm, &pp->icon);
        else
            update_frame(wm, pp);
    }
    if(pc != wm->clients)
    {
        if(pc->place_type == ICONIFY && wm->cur_layout != PREVIEW)
        {
            frame_icon(wm, &pc->icon);
            XSetInputFocus(wm->display, PointerRoot, RevertToPointerRoot, CurrentTime);
        }
        else
        {
            raise_client(wm);
            update_frame(wm, pc);
            XSetInputFocus(wm->display, pc->win, RevertToPointerRoot, CurrentTime);
        }
    }
}

void fix_focus_client(WM *wm)
{
    Client *c, *pp=wm->prev_focus_client, *pc=wm->cur_focus_client;
    if(!is_client(wm, pp))
    {
        if(pp->next != wm->clients)
            c=get_next_client(wm, pp);
        else
            c=get_prev_client(wm, pp);
        wm->prev_focus_client=(c ? c : wm->clients);
    }
    if(!is_client(wm, c))
    {
        if(pc->prev != wm->clients)
            c=get_prev_client(wm, pc);
        else
            c=get_next_client(wm, pc);
        wm->cur_focus_client=(c ? c : wm->clients);
    }
}

Client *get_next_client(WM *wm, Client *c)
{
    for(Client *p=c->next; p!=wm->cur_focus_client; p=p->next)
        if(p != wm->clients)
            return p;
    return NULL;
}

Client *get_prev_client(WM *wm, Client *c)
{
    for(Client *p=c->prev; p!=c; p=p->prev)
        if(p != wm->clients)
            return p;
    return NULL;
}

bool is_client(WM *wm, Client *c)
{
    for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
        if(p == c)
            return true;
    return false;
}
        
void unframe_icon(WM *wm, Icon *icon)
{
    XSetWindowBorderWidth(wm->display, icon->win, 0);
    XMoveWindow(wm->display, icon->win, icon->x, icon->y);
}

void frame_icon(WM *wm, Icon *icon)
{
    XSetWindowBorderWidth(wm->display, icon->win, BORDER_WIDTH);
    XSetWindowBorder(wm->display, icon->win, CURRENT_BORDER_COLOR);
    XMoveWindow(wm->display, icon->win,
        icon->x-BORDER_WIDTH, icon->y-2*BORDER_WIDTH);
}

void change_layout(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout != arg.layout)
    {
        if(wm->cur_layout == PREVIEW)
            iconify_all_for_vision(wm);
        if(arg.layout == PREVIEW)
            deiconify_all_for_vision(wm);
        wm->prev_layout=wm->cur_layout;
        wm->cur_layout=arg.layout;
        update_layout(wm);
        update_title_bar_layout(wm);
    }
}

void update_title_bar_layout(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        Rect r=get_title_area_rect(wm, c);
        XResizeWindow(wm->display, c->title_area, r.w, r.h);
    }
}

void pointer_focus_client(WM *wm, XEvent *e, Func_arg arg)
{
    focus_client(wm, win_to_client(wm, e->xbutton.window));
}

bool grab_pointer(WM *wm, XEvent *e)
{
    Cursor gc;
    XButtonEvent *be=&e->xbutton;
    Client *c=win_to_client(wm, be->window);
    int x=be->x_root, y=be->y_root, d;
    if(be->window == wm->root_win)
        gc=wm->cursors[ADJUST_LAYOUT_RATIO];
    else if(c)
    {
        if(be->window == c->title_area)
            gc=wm->cursors[MOVE];
        else if(be->window == c->frame)
            gc=wm->cursors[get_resize_incr(c, x, y, d, d, &d, &d, &d, &d)];
    }
    else
        return false;
    return XGrabPointer(wm->display, wm->root_win, False, POINTER_MASK,
        GrabModeAsync, GrabModeAsync, None, gc, CurrentTime) == GrabSuccess;
}

void apply_rules(WM *wm, Client *c)
{
    XClassHint ch={NULL, NULL};
    c->place_type=NORMAL;
    if(!XGetClassHint(wm->display, c->win, &ch))
        return ;

    WM_rule *p=RULES;
    for(size_t i=0; i<ARRAY_NUM(RULES); i++, p++)
        if( (!p->app_class || strstr(ch.res_class, p->app_class))
            && (!p->app_name || strstr(ch.res_name, p->app_name)))
            c->place_type=p->place_type;

    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);
}

void set_default_rect(WM *wm, Client *c)
{
    Window r;
    int x, y;
    unsigned int w, h, b, d;
    if(XGetGeometry(wm->display, c->win, &r, &x, &y, &w, &h, &b, &d))
        c->x=x, c->y=y, c->w=w, c->h=h;
    else
        c->x=wm->screen_width/4, c->y=wm->screen_height/4,
        c->w=wm->screen_width/2, c->h=wm->screen_height/2;
}

void adjust_n_main_max(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout == TILE)
    {
        wm->n_main_max+=arg.n;
        if(wm->n_main_max < 1)
            wm->n_main_max=1;
        update_layout(wm);
    }
}

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
void adjust_main_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout==TILE && wm->clients_n[NORMAL]>wm->n_main_max)
    {
        float ratio=wm->main_area_ratio+arg.change_ratio;
        int mw=ratio*wm->screen_width,
            sw=wm->screen_width*(1-wm->fixed_area_ratio)-mw;
        if(sw>=MOVE_RESIZE_INC && mw>=MOVE_RESIZE_INC)
        {
            wm->main_area_ratio=ratio;
            update_layout(wm);
        }
    }
}

/* 在次區域比例不變的情況下調整固定區域比例，固定區域和主區域比例此消彼長 */
void adjust_fixed_area_ratio(WM *wm, XEvent *e, Func_arg arg)
{ 
    if(wm->cur_layout==TILE && wm->clients_n[FIXED])
    {
        float ratio=wm->fixed_area_ratio+arg.change_ratio;
        int mw=wm->screen_width*(wm->main_area_ratio-arg.change_ratio),
            fw=wm->screen_width*ratio;
        if(mw>=MOVE_RESIZE_INC && fw>=MOVE_RESIZE_INC)
        {
            wm->main_area_ratio-=arg.change_ratio;
            wm->fixed_area_ratio=ratio;
            update_layout(wm);
        }
    }
}

void change_area(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=wm->cur_focus_client;
    if(c != wm->clients)
    {
        Area_type type=arg.area_type;
        if(wm->cur_layout == TILE)
        {
            switch(type)
            {
                case MAIN_AREA: to_main_area(wm, c); break;
                case SECOND_AREA: to_second_area(wm, c); break;
                case FIXED_AREA: to_fixed_area(wm, c); break;
                case FLOATING_AREA: to_floating_area(wm, c); break;
                case ICONIFY_AREA: iconify(wm, c); break;
            }
            warp_pointer_for_key_press(wm, c, e->type);
        }
        else if(wm->cur_layout==STACK && type==ICONIFY_AREA)
        {
            iconify(wm, c);
            warp_pointer_for_key_press(wm, c, e->type);
        }
    }
}

void warp_pointer_for_key_press(WM *wm, Client *c, int event_type)
{
    if(event_type == KeyPress)
    {
        Window win;
        int x, y;
        if(c == wm->clients)
            win=wm->taskbar.win, x=wm->taskbar.w/2, y=wm->taskbar.h/2;
        else
            win=c->win, x=c->w/2, y=c->h/2;
        XWarpPointer(wm->display, None, win, 0, 0, 0, 0, x, y);
    }
}

void to_main_area(WM *wm, Client *c)
{
    move_client(wm, c, wm->clients, NORMAL);
}

bool is_in_main_area(WM *wm, Client *c)
{
    Client *p=wm->clients->next;
    for(int i=0; i<wm->n_main_max; i++, p=p->next)
        if(p == c)
            return true;
    return false;
}

void del_client_node(Client *c)
{
    c->prev->next=c->next;
    c->next->prev=c->prev;
}

void add_client_node(Client *head, Client *c)
{
    c->prev=head;
    c->next=head->next;
    head->next=c;
    c->next->prev=c;
}

void to_second_area(WM *wm, Client *c)
{
    Client *to;

    if( (wm->clients_n[NORMAL] > wm->n_main_max ||
        (wm->clients_n[NORMAL]==wm->n_main_max && !is_in_main_area(wm, c)))
        && (to=get_second_area_head(wm)))
        move_client(wm, c, to, NORMAL);
}

Client *get_second_area_head(WM *wm)
{
    if(wm->clients_n[NORMAL] >= wm->n_main_max)
    {
        Client *head=wm->clients->next;
        for(int i=0; i<wm->n_main_max; i++)
            head=head->next;
        return head;
    }
    return NULL;
}

void to_fixed_area(WM *wm, Client *c)
{
    Client *to=get_area_head(wm, FIXED);
    if(c->place_type == FLOATING)
        to=to->next;
    move_client(wm, c, to, FIXED);
}

void to_floating_area(WM *wm, Client *c)
{
    move_client(wm, c, c, FLOATING);
}

void pointer_change_area(WM *wm, XEvent *e, Func_arg arg)
{
    Client *from=wm->cur_focus_client, *to;
    if(wm->cur_layout==TILE && from!=wm->clients && grab_pointer(wm, e))
    {
        XEvent ev;
        do
        {
            XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
            if(event_handlers[ev.type])
                event_handlers[ev.type](wm, &ev);
        }while((ev.type!=ButtonRelease || ev.xbutton.button!=e->xbutton.button));
        XUngrabPointer(wm->display, CurrentTime);

        /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
         * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
        to=win_to_client(wm, ev.xbutton.subwindow);
        if(to && to!=from)
            move_client(wm, from, to, to->place_type);
    }
}

int compare_client_order(WM *wm, Client *c1, Client *c2)
{
    if(c1 == c2)
        return 0;
    for(Client *c=c1; c!=wm->clients; c=c->next)
        if(c == c2)
            return -1;
    return 1;
}

void move_client(WM *wm, Client *from, Client *to, Place_type type)
{
    if(from != to)
    {
        del_client_node(from);
        if(compare_client_order(wm, from, to)==1 && to!=wm->clients)
            add_client_node(to->prev, from);
        else
            add_client_node(to, from);
    }
    update_client_n_and_place_type(wm, from, type);
    raise_client(wm);
    update_layout(wm);
}

/* 僅在移動窗口、聚焦窗口時才有可能需要提升 */
void raise_client(WM *wm)
{
    Client *c=wm->cur_focus_client;
    Window wins[]={wm->taskbar.win, c->frame};
    if(c->place_type == FLOATING)
        XRaiseWindow(wm->display, c->frame);
    else
        XRestackWindows(wm->display, wins, ARRAY_NUM(wins));
}

void frame_client(WM *wm, Client *c)
{
    Rect fr=get_frame_rect(c), tr=get_title_area_rect(wm, c);
    c->frame=XCreateSimpleWindow(wm->display, wm->root_win, fr.x, fr.y,
        fr.w, fr.h, BORDER_WIDTH, CURRENT_BORDER_COLOR, CURRENT_FRAME_COLOR);
    XSelectInput(wm->display, c->frame, FRAME_EVENT_MASK);
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
    {
        Rect br=get_button_rect(c, i);
        c->buttons[i]=XCreateSimpleWindow(wm->display, c->frame,
            br.x, br.y, br.w, br.h, 0, 0, CURRENT_BUTTON_COLOR);
        XSelectInput(wm->display, c->buttons[i], BUTTON_EVENT_MASK);
    }
    c->title_area=XCreateSimpleWindow(wm->display, c->frame,
        tr.x, tr.y, tr.w, tr.h, 0, 0, CURRENT_TITLE_AREA_COLOR);
    XSelectInput(wm->display, c->title_area, TITLE_AREA_EVENT_MASK);
    XAddToSaveSet(wm->display, c->win);
    XReparentWindow(wm->display, c->win, c->frame, 0, TITLE_BAR_HEIGHT);
    XMapWindow(wm->display, c->frame);
    XMapSubwindows(wm->display, c->frame);
}

Rect get_frame_rect(Client *c)
{
    return (Rect){c->x-BORDER_WIDTH, c->y-TITLE_BAR_HEIGHT-BORDER_WIDTH,
        c->w, c->h+TITLE_BAR_HEIGHT};
}

Rect get_title_area_rect(WM *wm, Client *c)
{
    int n[]={0, 1, 3, 7};
    return (Rect){0, 0, c->w-CLIENT_BUTTON_WIDTH*n[wm->cur_layout],
        TITLE_BAR_HEIGHT};
}

Rect get_button_rect(Client *c, size_t index)
{
    return (Rect){c->w-CLIENT_BUTTON_WIDTH*(CLIENT_BUTTON_N-index),
    (TITLE_BAR_HEIGHT-CLIENT_BUTTON_HEIGHT)/2,
    CLIENT_BUTTON_WIDTH, CLIENT_BUTTON_HEIGHT};
}

void update_title_area_text(WM *wm, Client *c)
{
    Rect tr=get_title_area_rect(wm, c);
    draw_string(wm, c->title_area, TITLE_TEXT_COLOR, LEFT,
        tr.x, tr.y, tr.w, tr.h, c->title_text);
}

bool update_title_button_text(WM *wm, Client *c, Window win)
{
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
    {
        if(win == c->buttons[i])
        {
            draw_string(wm, c->buttons[i], BUTTON_TEXT_COLOR, CENTER, 0, 0,
                CLIENT_BUTTON_WIDTH, CLIENT_BUTTON_HEIGHT, CLIENT_BUTTON_TEXT[i]);
            return true;
        }
    }
    return false;
}

void update_status_area_text(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    draw_string(wm, b->status_win, STATUS_AREA_TEXT_COLOR, RIGHT, 0, 0,
        b->w-TASKBAR_BUTTON_WIDTH*TASKBAR_BUTTON_N, b->h, b->status_text);
}

void move_resize_client(WM *wm, Client *c, int dx, int dy, unsigned int dw, unsigned int dh)
{
    c->x+=dx, c->y+=dy, c->w+=dw, c->h+=dh;
    Rect fr=get_frame_rect(c), tr=get_title_area_rect(wm, c);
    XMoveResizeWindow(wm->display, c->frame, fr.x, fr.y, fr.w, fr.h);
    XMoveResizeWindow(wm->display, c->win, 0, TITLE_BAR_HEIGHT, c->w, c->h);
    XMoveResizeWindow(wm->display, c->title_area, tr.x, tr.y, tr.w, tr.h);
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
    {
        Rect br=get_button_rect(c, i);
        XMoveResizeWindow(wm->display, c->buttons[i], br.x, br.y, br.w, br.h);
    }
}

void fix_win_rect(Client *c)
{
    c->x+=BORDER_WIDTH;
    c->y+=BORDER_WIDTH+TITLE_BAR_HEIGHT;
    if(c->w > 2*BORDER_WIDTH+WINS_SPACE)
        c->w-=2*BORDER_WIDTH+WINS_SPACE;
    if(c->h > 2*BORDER_WIDTH+WINS_SPACE+TITLE_BAR_HEIGHT)
        c->h-=2*BORDER_WIDTH+WINS_SPACE+TITLE_BAR_HEIGHT;
}

void update_frame(WM *wm, Client *c)
{
    if(c == wm->cur_focus_client)
    {
        XSetWindowBorder(wm->display, c->frame, CURRENT_BORDER_COLOR);
        update_win_background(wm, c->frame, CURRENT_FRAME_COLOR);
        update_win_background(wm, c->title_area, CURRENT_TITLE_AREA_COLOR);
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            update_win_background(wm, c->buttons[i], CURRENT_BUTTON_COLOR);
    }
    else
    {
        XSetWindowBorder(wm->display, c->frame, NORMAL_BORDER_COLOR);
        update_win_background(wm, c->frame, NORMAL_FRAME_COLOR);
        update_win_background(wm, c->title_area, NORMAL_TITLE_AREA_COLOR);
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            update_win_background(wm, c->buttons[i], NORMAL_BUTTON_COLOR);
    }
}

/* 在調用XSetWindowBackground之後，在收到下一個顯露事件或調用XClearWindow
 * 之前，背景不變。此處用發送顯露事件的方式使背景設置立即生效。*/
void update_win_background(WM *wm, Window win, unsigned long color)
{
    XEvent event={.xexpose={.type=Expose, .window=win}};
    XSetWindowBackground(wm->display, win, color);
    XSendEvent(wm->display, win, False, NoEventMask, &event);
}

Click_type get_click_type(WM *wm, Window win)
{
    Click_type i;
    Client *c;
    if(win == wm->root_win)
        return CLICK_ROOT;
    for(i=CLICK_TASKBAR_BUTTON_BEGIN; i<=CLICK_TASKBAR_BUTTON_END; i++)
        if(win == wm->taskbar.buttons[i-CLICK_TASKBAR_BUTTON_BEGIN])
            return i;
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(win == c->icon.win)
            return CLICK_ICON;
    if((c=win_to_client(wm, win)))
    {
        if(win == c->win)
            return CLICK_WIN;
        else if(win == c->frame)
            return CLICK_FRAME;
        else if(win == c->title_area)
            return CLICK_TITLE;
        else
            for(i=CLICK_CLIENT_BUTTON_BEGIN; i<=CLICK_CLIENT_BUTTON_END; i++)
                if(win == c->buttons[i-CLICK_CLIENT_BUTTON_BEGIN])
                    return i;
    }
    return UNDEFINED;
}

void handle_enter_notify(WM *wm, XEvent *e)
{
    XCrossingEvent *ce=&e->xcrossing;
    hint_enter_taskbar_button(wm, ce->window);
    hint_enter_client_button(wm, ce->window);
    hint_resizing(wm, ce->window, ce->x_root, ce->y_root);
    hint_motion(wm, ce->window, ce->x_root, ce->y_root);
}

void hint_enter_taskbar_button(WM *wm, Window win)
{
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
        if(win == wm->taskbar.buttons[i] && (i=TASKBAR_BUTTON_N))
            update_win_background(wm, win, ENTERED_TASKBAR_BUTTON_COLOR);
}

void hint_enter_client_button(WM *wm, Window win)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            if(win == c->buttons[i] && (i=CLIENT_BUTTON_N))
                update_win_background(wm, win,
                    i==CLOSE_WIN-CLICK_CLIENT_BUTTON_BEGIN ?
                    ENTERED_CLOSE_BUTTON_COLOR : ENTERED_NORMAL_BUTTON_COLOR);
}

void hint_resizing(WM *wm, Window win, int x, int y)
{
    int d;
    Client *c=win_to_client(wm, win);
    if(c && win==c->frame)
        XDefineCursor(wm->display, win,
            wm->cursors[get_resize_incr(c, x, y, d, d, &d, &d, &d, &d)]);
}

void hint_motion(WM *wm, Window win, int x, int y)
{
    Client *c=win_to_client(wm, win);
    if(c && win==c->title_area)
        XDefineCursor(wm->display, win, wm->cursors[MOVE]);
    else if(win == wm->root_win && (is_main_sec_space(wm, x) || is_main_fix_space(wm, x)))
        XDefineCursor(wm->display, win, wm->cursors[ADJUST_LAYOUT_RATIO]);
}

void handle_leave_notify(WM *wm, XEvent *e)
{
    hint_leave_taskbar_button(wm, e->xcrossing.window);
    hint_leave_client_button(wm, e->xcrossing.window);
    XUndefineCursor(wm->display, e->xcrossing.window);
}

void hint_leave_taskbar_button(WM *wm, Window win)
{
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
        if(win == wm->taskbar.buttons[i] && (i=TASKBAR_BUTTON_N))
            update_win_background(wm, win, TASKBAR_BUTTON_COLOR);
}

void hint_leave_client_button(WM *wm, Window win)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            if(win == c->buttons[i] && (i=CLIENT_BUTTON_N))
                update_win_background(wm, win, c==wm->cur_focus_client ?
                    CURRENT_BUTTON_COLOR : NORMAL_BUTTON_COLOR);
}

void maximize_client(WM *wm, XEvent *e, Func_arg unused)
{
    wm->prev_focus_client=wm->cur_focus_client;
    Client *c=wm->cur_focus_client;
    if(c != wm->clients)
    {
        c->x=BORDER_WIDTH;
        c->y=BORDER_WIDTH+TITLE_BAR_HEIGHT;
        c->w=wm->screen_width-2*BORDER_WIDTH;
        c->h=wm->screen_height-2*BORDER_WIDTH-TITLE_BAR_HEIGHT;
        if(wm->cur_layout == TILE)
            to_floating_area(wm, wm->cur_focus_client);
        move_resize_client(wm, c, 0, 0, 0, 0);
    }
}

void iconify(WM *wm, Client *c)
{
    create_icon(wm, c);
    XSelectInput(wm->display, c->icon.win, ButtonPressMask|ExposureMask);
    XMoveWindow(wm->display, c->icon.win, c->icon.x, c->icon.y);
    XMapWindow(wm->display, c->icon.win);
    XUnmapWindow(wm->display, c->frame);
    c->icon.place_type=c->place_type;
    update_client_n_and_place_type(wm, c, ICONIFY);
    update_layout(wm);
    if(c == wm->cur_focus_client)
    {
        focus_client(wm, NULL);
        update_frame(wm, c);
    }
}

void create_icon(WM *wm, Client *c)
{
    unsigned long mask=CWBackPixel;
    XSetWindowAttributes attr={.background_pixel=BLACK};
    XWMHints *hints=XGetWMHints(wm->display, c->win);
    c->icon.w=c->icon.h=wm->taskbar.h;
    if(hints)
    {
        if(hints->flags & IconWindowHint)
            c->icon.win=hints->icon_window;
        else if(hints->flags & IconPixmapHint)
            mask=CWBackPixmap, attr.background_pixmap=hints->icon_pixmap;
        get_drawable_size(wm, c->icon.win ? c->icon.win : hints->icon_pixmap,
            &c->icon.w, &c->icon.h);
    }
    set_icon_position(wm, c);
    if(c->icon.win == None)
        c->icon.win=XCreateWindow(wm->display, wm->root_win,
            c->icon.x, c->icon.y, c->icon.w, c->icon.h, 0, 0,
            CopyFromParent, CopyFromParent, mask, &attr);
}

void get_drawable_size(WM *wm, Drawable d, unsigned int *w, unsigned int *h)
{
    int x, y;
    unsigned int bw, dep;
    Window root;
    XGetGeometry(wm->display, d, &root, &x, &y, w, h, &bw, &dep);
}

void set_icon_position(WM *wm, Client *c)
{
    c->icon.x=TASKBAR_BUTTON_WIDTH*TASKBAR_BUTTON_N+BORDER_WIDTH;
    c->icon.y=wm->screen_height-c->icon.h;
    for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
        if(p!=c && p->icon.win)
            c->icon.x+=p->icon.w+BORDER_WIDTH;
}

void key_choose_client(WM *wm, XEvent *e, Func_arg arg)
{
    Client *c=wm->cur_focus_client;
    if(c != wm->clients)
    {
        if(wm->cur_layout == PREVIEW)
            choose_client_in_preview(wm, c);
        else if(c->place_type == ICONIFY)
            deiconify(wm, wm->cur_focus_client);
    }
}

void pointer_deiconify(WM *wm, XEvent *e, Func_arg arg)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(e->xbutton.window == c->icon.win)
        {
            deiconify(wm, c);
            break;
        }
    }
}

void deiconify(WM *wm, Client *c)
{
    XMapWindow(wm->display, c->frame);
    XUnmapWindow(wm->display, c->icon.win);
    update_client_n_and_place_type(wm, c, c->icon.place_type);
    update_layout(wm);
    for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
        if(p!=c && p->icon.win && p->icon.x>c->icon.x)
            XMoveWindow(wm->display, p->icon.win,
                p->icon.x-=c->icon.w+BORDER_WIDTH, p->icon.y);
    memset(&c->icon, 0, sizeof(c->icon));
    focus_client(wm, c);
}

void update_client_n_and_place_type(WM *wm, Client *c, Place_type type)
{
    wm->clients_n[c->place_type]--;
    c->place_type=type;
    wm->clients_n[c->place_type]++;
}

void pointer_move_client(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout==FULL || wm->cur_layout==PREVIEW || !grab_pointer(wm, e))
        return;
    int ox=e->xbutton.x_root, oy=e->xbutton.y_root, nx, ny, dx, dy;
    XEvent ev;
    if(wm->cur_layout == TILE)
        to_floating_area(wm, wm->cur_focus_client);
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, ny=ev.xmotion.y, dx=nx-ox, dy=ny-oy;
            if(abs(dx)>=MOVE_RESIZE_INC || abs(dy)>=MOVE_RESIZE_INC)
            {
                move_resize_client(wm, wm->cur_focus_client, dx, dy, 0, 0);
                ox=nx, oy=ny;
            }
        }
        else if(event_handlers[ev.type])
            event_handlers[ev.type](wm, &ev);
    }while(!(ev.type==ButtonRelease && ev.xbutton.button==e->xbutton.button));
    XUngrabPointer(wm->display, CurrentTime);
}

void pointer_resize_client(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout==FULL || wm->cur_layout==PREVIEW || !grab_pointer(wm, e))
        return;
    Client *c=wm->cur_focus_client;
    int ox=e->xbutton.x_root, oy=e->xbutton.y_root, nx, ny, dx, dy, dw, dh;
    XEvent ev;
    if(wm->cur_layout == TILE)
        to_floating_area(wm, wm->cur_focus_client);
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, ny=ev.xmotion.y, dx=nx-ox, dy=ny-oy;
            if(abs(dx)>=MOVE_RESIZE_INC || abs(dy)>=MOVE_RESIZE_INC)
            {
                get_resize_incr(c, ox, oy, nx, ny, &dx, &dy, &dw, &dh);
                move_resize_client(wm, c, dx, dy, dw, dh);
                ox=nx, oy=ny;
            }
        }
        else if(event_handlers[ev.type])
            event_handlers[ev.type](wm, &ev);
    }while(!(ev.type==ButtonRelease && ev.xbutton.button==e->xbutton.button));
    XUngrabPointer(wm->display, CurrentTime);
}

Pointer_act get_resize_incr(Client *c, int ox, int oy, int nx, int ny, int *dx, int *dy, int *dw, int *dh)
{
    int bw=BORDER_WIDTH, cl=MOVE_RESIZE_INC, lx=c->x-bw, rx=c->x+c->w+bw,
        ty=c->y-TITLE_BAR_HEIGHT-bw, by=c->y+c->h+bw;
    if(ox>=lx && ox<lx+cl && oy>=ty && oy<ty+cl) /* 左上邊框 */
        return TOP_LEFT_RESIZE+0*(*dx=nx-ox, *dy=ny-oy, *dw=-*dx, *dh=-*dy);
    else if(ox>=rx-cl && ox<rx && oy>=ty && oy<ty+cl) /* 右上邊框 */
        return TOP_RIGHT_RESIZE+0*(*dx=0, *dy=ny-oy, *dw=nx-ox, *dh=-*dy);
    else if(ox>=lx && ox<lx+cl && oy>=by-cl && oy<by) /* 左下邊框 */
        return BOTTOM_LEFT_RESIZE+0*(*dx=nx-ox, *dy=0, *dw=-*dx, *dh=ny-oy);
    else if(ox>=rx-cl && ox<rx && oy>=by-cl && oy<by) /* 右下邊框 */
        return BOTTOM_RIGHT_RESIZE+0*(*dx=0, *dy=0, *dw=nx-ox, *dh=ny-oy);
    else if(oy>=ty && oy<ty+bw) /* 上邊框 */
        return VERT_RESIZE+0*(*dx=0, *dy=ny-oy, *dw=0, *dh=-*dy);
    else if(oy>=by-bw && oy<by) /* 下邊框 */
        return VERT_RESIZE+0*(*dx=0, *dy=0, *dw=0, *dh=ny-oy);
    else if(ox>=lx && ox<lx+bw) /* 左邊框 */
        return HORIZ_RESIZE+0*(*dx=nx-ox, *dy=0, *dw=-*dx, *dh=0);
    else if(ox>=rx-bw && ox<rx) /* 右邊框 */
        return HORIZ_RESIZE+0*(*dx=0, *dy=0, *dw=nx-ox, *dh=0);
    else
        return NO_OP+0*(*dx=0, *dy=0, *dw=0, *dh=0);
}

void adjust_layout_ratio(WM *wm, XEvent *e, Func_arg arg)
{
    if(wm->cur_layout!=TILE || !grab_pointer(wm, e))
        return;
    int ox=e->xbutton.x_root, nx, dx;
    XEvent ev;
    do /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            nx=ev.xmotion.x, dx=nx-ox;
            if(abs(dx)>=MOVE_RESIZE_INC && change_layout_ratio(wm, ox, nx))
                update_layout(wm), ox=nx;
        }
        else if(event_handlers[ev.type])
            event_handlers[ev.type](wm, &ev);
    }while(!(ev.type==ButtonRelease && ev.xbutton.button==e->xbutton.button));
    XUngrabPointer(wm->display, CurrentTime);
}

bool change_layout_ratio(WM *wm, int ox, int nx)
{
    double dr;
    dr=1.0*(nx-ox)/wm->screen_width;
    if(is_main_sec_space(wm, ox))
        wm->main_area_ratio-=dr;
    else if(is_main_fix_space(wm, ox))
        wm->main_area_ratio+=dr, wm->fixed_area_ratio-=dr;
    else
        return false;
    return true;
}

bool is_main_sec_space(WM *wm, int x)
{
    unsigned int w=wm->screen_width*(1-wm->main_area_ratio-wm->fixed_area_ratio);
    return (x>=w-WINS_SPACE && x<w);
}

bool is_main_fix_space(WM *wm, int x)
{
    unsigned int w=wm->screen_width*(1-wm->fixed_area_ratio);
    return (x>=w-WINS_SPACE && x<w);
}
