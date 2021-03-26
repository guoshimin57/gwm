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

void init_wm(WM *wm)
{
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fprintf(stderr, "warning: no locale support\n");
	if(!(wm->display=XOpenDisplay(NULL)))
    {
		fprintf(stderr, "error: cannot open display\n");
        exit(EXIT_FAILURE);
    }
    wm->screen=DefaultScreen(wm->display);
    wm->screen_width=DisplayWidth(wm->display, wm->screen);
    wm->screen_height=DisplayHeight(wm->display, wm->screen);
	wm->mod_map=XGetModifierMapping(wm->display);
    wm->root_win=RootWindow(wm->display, wm->screen);
    wm->gc=XCreateGC(wm->display, wm->root_win, 0, NULL);
    wm->layout=DEFAULT_LAYOUT;
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
    exec(wm, NULL, (FUNC_ARG)SH_CMD("[ -x "AUTOSTART" ] && "AUTOSTART));
}

int my_x_error_handler(Display *display, XErrorEvent *e)
{
    if( e->request_code == X_ChangeWindowAttributes
        && e->error_code == BadAccess)
    {
        fprintf(stderr, "錯誤：已經有其他窗口管理器在運行！\n");
        exit(EXIT_FAILURE);
    }
    print_error_msg(display, e);
	if( e->error_code == BadWindow
        || (e->request_code == X_ConfigureWindow
            && e->error_code == BadMatch))
		return -1;
    return 0;
}

void create_font_set(WM *wm)
{
    char **missing_charset_list;
    int missing_charset_count;
    char *missing_charset_def_str;
    wm->font_set=XCreateFontSet(wm->display, FONT_SET, &missing_charset_list,
        &missing_charset_count, &missing_charset_def_str);
}

void create_cursors(WM *wm)
{
    wm->cursors[MOVE_CURSOR]=XCreateFontCursor(wm->display, XC_fleur);
    wm->cursors[RESIZE_CURSOR]=XCreateFontCursor(wm->display, XC_sizing);
}

void create_taskbar(WM *wm)
{
    TASKBAR *b=&wm->taskbar;
    b->x=0;
    b->y=wm->screen_height-TASKBAR_HEIGHT;
    b->w=wm->screen_width;
    b->h=TASKBAR_HEIGHT;
    b->win=XCreateSimpleWindow(wm->display, wm->root_win, b->x, b->y,
        b->w, b->h, 0, 0, TASKBAR_COLOR);
    XSelectInput(wm->display, b->win, SubstructureRedirectMask|SubstructureNotifyMask|ExposureMask);
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        b->buttons[i]=XCreateSimpleWindow(wm->display, b->win,
            b->w-TASKBAR_BUTTON_WIDTH*(TASKBAR_BUTTON_N-i), 0, TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT, 0, 0, TASKBAR_BUTTON_COLOR);
        XSelectInput(wm->display, b->buttons[i], ButtonPressMask|ExposureMask|EnterWindowMask|LeaveWindowMask);
    }
    create_status_bar(wm);
    XMapRaised(wm->display, b->win);
    XMapSubwindows(wm->display, b->win);
}

void create_status_bar(WM *wm)
{
    STATUS_BAR *b=&wm->taskbar.status_bar;
    b->x=0, b->y=0;
    b->w=wm->taskbar.w-TASKBAR_BUTTON_WIDTH*TASKBAR_BUTTON_N;
    b->h=TASKBAR_HEIGHT;
    b->win=XCreateSimpleWindow(wm->display, wm->taskbar.win, b->x, b->y,
        b->w, b->h, 0, 0, STATUS_BAR_COLOR);
    b->text="gwm";
    XSelectInput(wm->display, b->win, ExposureMask);
}

void print_error_msg(Display *display, XErrorEvent *e)
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

    wm->cur_focus_client=wm->prev_focus_client=wm->clients=malloc_s(sizeof(CLIENT));
    wm->clients->win=wm->root_win;
    wm->n=wm->n_normal=wm->n_float=wm->n_fixed=0;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &n))
    {
        for(size_t i=0; i<n; i++)
            if(is_wm_win(wm, child[i]))
                add_client(wm, child[i]);
        XFree(child);
    }
    else
        fprintf(stderr, "錯誤：查詢窗口清單失敗！\n");
}

void *malloc_s(size_t size)
{
    void *p=malloc(size);
    if(p == NULL)
    {
        fprintf(stderr, "錯誤：申請內存失敗！\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

bool is_wm_win(WM *wm, Window win)
{
    XWindowAttributes attr;
    return ( win != wm->taskbar.win
        && XGetWindowAttributes(wm->display, win, &attr)
        && attr.map_state != IsUnmapped
        && get_state_hint(wm, win) == NormalState
        && !attr.override_redirect);
}

int get_state_hint(WM *wm, Window w)
{
    XWMHints *p=XGetWMHints(wm->display, w);
    int state=(p && (p->flags & StateHint)) ? p->initial_state : -1;
    if(p)
        XFree(p);
    return state;
}

void add_client(WM *wm, Window win)
{
    CLIENT *c;
    c=malloc_s(sizeof(CLIENT));
    c->win=win;
    apply_rules(wm, c);
    add_client_node(get_area_head(wm, c->place_type), c);
    update_n_for_add(wm, c);
    set_default_rect(wm, c);
    frame_client(wm, c);
    focus_client(wm, c);
    grab_buttons(wm, c);
}

CLIENT *get_area_head(WM *wm, PLACE_TYPE type)
{
    int i, n;
    CLIENT *head=wm->clients;
    switch(type)
    {
        case NORMAL: n=0; break;
        case FIXED: n=wm->n_normal; break;
        case FLOATING: n=wm->n_normal+wm->n_fixed; break;
    }
    for(i=0; i<n; i++)
        head=head->next;
    return head;
}

void update_layout(WM *wm)
{
    if(wm->n == 0)
        return ;
    switch(wm->layout)
    {
        case FULL: set_full_layout(wm); break;
        case PREVIEW: set_preview_layout(wm); break;
        case TILE: set_tile_layout(wm); break;
        default: return;
    }
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
        do_move_resize_client(wm, c, 0, 0, 0, 0);
}

void set_full_layout(WM *wm)
{
    CLIENT *c=wm->cur_focus_client;
    c->x=c->y=0, c->w=wm->screen_width, c->h=wm->screen_height;
}

void set_preview_layout(WM *wm)
{
    int i=wm->n-1, rows, cols, w, h, ch=(wm->screen_height-wm->taskbar.h);
    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=wm->n; cols++)
        if(cols*cols >= wm->n)
            break;
    rows=(cols-1)*cols>=wm->n ? cols-1 : cols;
    w=wm->screen_width/cols;
    h=ch/rows;
    for(CLIENT *c=wm->clients->prev; c!=wm->clients; c=c->prev, i--)
    {
        c->x=(i%cols)*w;
        c->y=(i/cols)*h;
        /* 下邊和右邊的窗口佔用剩餘空間 */
        c->w=(i+1)%cols ? w : w+(wm->screen_width-w*cols);
        c->h=i<cols*(rows-1) ? h : h+(ch-h*rows);
        fix_win_rect(c);
    }
}

void set_tile_layout(WM *wm)
{
    CLIENT *c;
    unsigned int i, j, mw, sw, fw, mh, sh, fh, h; /* m=main, s=sec, f=fixed */

    mw=wm->main_area_ratio*wm->screen_width;
    fw=wm->screen_width*wm->fixed_area_ratio;
    sw=wm->screen_width-fw-mw;
    h=wm->screen_height-wm->taskbar.h;
    mh=wm->n_normal>=wm->n_main_max ? h/wm->n_main_max :
        (wm->n_normal ? h/wm->n_normal : h);
    fh=wm->n_fixed ? h/wm->n_fixed : h;
    sh=wm->n_normal>wm->n_main_max ? h/(wm->n_normal-wm->n_main_max) : h;
    if(wm->n_fixed == 0) mw+=fw, fw=0;
    if(wm->n_normal <= wm->n_main_max) mw+=sw, sw=0;

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
        if(c->place_type != FLOATING)
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
	XModifierKeymap *m=XGetModifierMapping(wm->display);
    KeyCode code=XKeysymToKeycode(wm->display, XK_Num_Lock);
    if(code)
    {
        for(size_t i=0; i<8; i++)
        {
            for(size_t j=0; j<m->max_keypermod; j++)
            {
                if(m->modifiermap[i*m->max_keypermod+j] == code)
                {
                    XFreeModifiermap(m);
                    return (1<<i);
                }
            }
        }
    }
    return 0;
}
    
void grab_buttons(WM *wm, CLIENT *c)
{
    unsigned int num_lock_mask=get_num_lock_mask(wm),
                 masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    BUTTONBIND *p=BUTTONBINDS;
    XUngrabButton(wm->display, AnyButton, AnyModifier, c->win);
    for(size_t i=0; i<ARRAY_NUM(BUTTONBINDS); i++, p++)
    {
        if(p->click_type == CLICK_WIN)
        {
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
            {
                if(is_equal_modifier_mask(wm, 0, p->modifier))
                    XGrabButton(wm->display, p->button, p->modifier|masks[j],
                        c->win, False, BUTTON_MASK,
                        GrabModeSync, GrabModeSync, None, None);
                else
                    XGrabButton(wm->display, p->button, p->modifier|masks[j],
                        c->win, False, BUTTON_MASK,
                        GrabModeSync, GrabModeSync, None, None);
            }
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
    CLIENT *c=win_to_client(wm, e->xbutton.window);
    if(c)
        focus_client(wm, c);
    BUTTONBIND *p=BUTTONBINDS;
    CLICK_TYPE type=get_click_type(wm, e->xbutton.window);
	for(size_t i=0; i<ARRAY_NUM(BUTTONBINDS); i++, p++)
    {
        if( p->click_type == type 
            && p->button == e->xbutton.button
            && is_equal_modifier_mask(wm, p->modifier, e->xbutton.state))
        {
            if(p->func)
                p->func(wm, e, p->arg);
            if(type == CLICK_WIN)
                XAllowEvents(wm->display, ReplayPointer, CurrentTime);
            break;
        }
    }
}
 
bool is_equal_modifier_mask(WM *wm, unsigned int m1, unsigned int m2)
{
    return (get_valid_mask(wm, m1) == get_valid_mask(wm, m2));
}

void handle_config_request(WM *wm, XEvent *e)
{
    XConfigureRequestEvent cr=e->xconfigurerequest;
    CLIENT *c=win_to_client(wm, cr.window);

    if(c)
        config_managed_client(wm, c);
    else
        config_unmanaged_win(wm, &cr);
}

void config_managed_client(WM *wm, CLIENT *c)
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

CLIENT *win_to_client(WM *wm, Window win)
{
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(win==c->win || win==c->frame || win==c->title_area)
            return c;
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            if(win == c->buttons[i])
                return c;
    }
        
    return NULL;
}

void del_client(WM *wm, CLIENT *c)
{
    if(c)
    {
        del_client_node(c);
        update_n_for_del(wm, c);
        free(c);
        focus_client(wm, NULL);
    }
}

void handle_expose(WM *wm, XEvent *e)
{
    Window win=e->xexpose.window;
    STATUS_BAR *b=&wm->taskbar.status_bar;

    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        if(win == wm->taskbar.buttons[i])
        {
            draw_string(wm, win, TASKBAR_BUTTON_TEXT_COLOR, CENTER, 0, 0,
                TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT, TASKBAR_BUTTON_TEXT[i]);
            return;
        }
    }

    if(win == b->win)
        draw_string(wm, b->win, STATUS_BAR_TEXT_COLOR, CENTER,
            0, 0, b->w, b->h, b->text);
    else
        for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
            if(is_part_of_title_bar(c, win))
                update_title_bar_text(wm, c, win);
}

void handle_key_press(WM *wm, XEvent *e)
{
    int n;
    KeyCode kc=e->xkey.keycode;
	KeySym *keysym=XGetKeyboardMapping(wm->display, kc, 1, &n);
    KEYBIND *p=KEYBINDS;
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
    if(is_wm_win(wm, win))
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
    CLIENT *c=win_to_client(wm, e->xunmap.window);
    if(c && e->xunmap.event==c->frame && e->xunmap.window==c->win)
    {
        XDestroyWindow(wm->display, c->frame);
        del_client(wm, c);
        update_layout(wm);
    }
}

void handle_property_notify(WM *wm, XEvent *e)
{
    STATUS_BAR *b=&wm->taskbar.status_bar;
    XTextProperty name;
    if( e->xproperty.window==wm->root_win && e->xproperty.atom==XA_WM_NAME
        && XGetTextProperty(wm->display, wm->root_win, &name, XA_WM_NAME))
            draw_string(wm, b->win, STATUS_BAR_TEXT_COLOR, CENTER,
                0, 0, b->w, b->h, b->text=(char *)name.value);
}

void draw_string(WM *wm, Drawable drawable, unsigned long color, DIRECTION d, int x, int y, unsigned int w, unsigned h, const char *str)
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

void exec(WM *wm, XEvent *e, FUNC_ARG arg)
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

void key_move_resize_client(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->layout==TILE || wm->layout==STACK)
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
        CLIENT *c=wm->cur_focus_client;
        prepare_for_move_resize(wm);
        if(is_valid_move_resize(wm, c, p[0], p[1], p[2], p[3]))
        {
            if(p[2]>=0 || (p[2]<0 && c->w>=-p[2]+s))
                c->x+=p[0], c->w+=p[2];
            if(p[3]>=0 || (p[3]<0 && c->h>=-p[3]+s))
                c->y+=p[1], c->h+=p[3];
            do_move_resize_client(wm, c, 0, 0, 0, 0);
        }
    }
}

void prepare_for_move_resize(WM *wm)
{
    CLIENT *c=wm->cur_focus_client;
    if(c == wm->clients)
    {
        fprintf(stderr, "錯誤：不能移動根窗口或改變根窗口的尺寸！\n");
        return;
    }
    if(wm->layout==TILE && c->place_type!=FLOATING)
    {
        update_n_for_del(wm, c);
        c->place_type=FLOATING;
        update_n_for_add(wm, c);
        update_layout(wm);
        raise_client(wm);
    }
}

/* 通過求窗口與屏幕是否有交集來判斷窗口是否已經在屏幕外，即是否合法。
 * 若滿足以下條件，則有交集：窗口與屏幕中心距≤窗口半邊長+屏幕半邊長。
 * 即：|x+w/2-0-sw/2|＜|w/2+sw/2| 且 |y+h/2-0-sh/2|＜|h/2+sh/2|。
 * 兩邊同乘以2，得：|2*x+w-sw|＜|w+sw| 且 |2*y+h-sh|＜|h+sh|。
 */
bool is_valid_move_resize(WM *wm, CLIENT *c, int dx, int dy, int dw, int dh)
{
    int x=c->x+dx, y=c->y+dy, w=c->w+dw, h=c->h+dh,
        sw=wm->screen_width, sh=wm->screen_height;
    return w>0 && h>0 && abs(2*x+w-sw)<w+sw && abs(2*y+h-sh)<h+sh;
}

void quit_wm(WM *wm, XEvent *e, FUNC_ARG unused)
{
    XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
    XClearWindow(wm->display, wm->root_win);
    XFlush(wm->display);
    XCloseDisplay(wm->display);
    exit(EXIT_SUCCESS);
}

void close_win(WM *wm, XEvent *e, FUNC_ARG unused)
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

void next_client(WM *wm, XEvent *e, FUNC_ARG unused)
{
    if(wm->n) /* 允許切換至根窗口 */
    {
        focus_client(wm, wm->cur_focus_client->prev);
        CLIENT *c=wm->cur_focus_client;
        if(e->type == KeyPress)
            XWarpPointer(wm->display, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
    }
}

void prev_client(WM *wm, XEvent *e, FUNC_ARG unused)
{
    if(wm->n) /* 允許切換至根窗口 */
    {
        focus_client(wm, wm->cur_focus_client->next);
        CLIENT *c=wm->cur_focus_client;
        if(e->type == KeyPress)
            XWarpPointer(wm->display, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
    }
}

void focus_client(WM *wm, CLIENT *c)
{
    if(c == wm->cur_focus_client)
        return;
    if(c)
        wm->prev_focus_client=wm->cur_focus_client, wm->cur_focus_client=c;
    else
        fix_focus_client(wm);
    if(wm->prev_focus_client != wm->clients)
        update_frame(wm, wm->prev_focus_client);
    if(wm->cur_focus_client != wm->clients)
    {
        raise_client(wm);
        update_frame(wm, wm->cur_focus_client);
    }
}

void fix_focus_client(WM *wm)
{
    CLIENT *c;
    if(!is_client(wm, wm->prev_focus_client))
    {
        if(wm->prev_focus_client->next != wm->clients)
            c=get_next_client(wm, wm->prev_focus_client);
        else
            c=get_prev_client(wm, wm->prev_focus_client);
        wm->prev_focus_client=(c ? c : wm->clients);
    }
    if(!is_client(wm, wm->cur_focus_client))
    {
        if(wm->cur_focus_client->prev != wm->clients)
            c=get_prev_client(wm, wm->cur_focus_client);
        else
            c=get_next_client(wm, wm->cur_focus_client);
        wm->cur_focus_client=(c ? c : wm->clients);
    }
}

CLIENT *get_next_client(WM *wm, CLIENT *c)
{
    unsigned int i=0;
    for(CLIENT *p=wm->cur_focus_client->next; i<=wm->n; p=p->next, i++)
        if(p != wm->clients)
            return p;
    return NULL;
}

CLIENT *get_prev_client(WM *wm, CLIENT *c)
{
    unsigned int i=0;
    for(CLIENT *p=c->prev; i<=wm->n; p=p->prev, i++)
        if(p != wm->clients)
            return p;
    return NULL;
}

bool is_client(WM *wm, CLIENT *c)
{
    for(CLIENT *p=wm->clients->next; p!=wm->clients; p=p->next)
        if(p == c)
            return true;
    return false;
}
        
void change_layout(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->layout != arg.layout)
    {
        wm->layout=arg.layout;
        update_taskbar_layout(wm);
        update_layout(wm);
        update_title_bar_layout(wm);
    }
}

void update_taskbar_layout(WM *wm)
{
    if(wm->layout == FULL)
        XUnmapWindow(wm->display, wm->taskbar.win);
    else
    {
        int n=0;
        Window *b=wm->taskbar.buttons;
        XMapWindow(wm->display, wm->taskbar.win);
        if(wm->layout == TILE)
            for(size_t i=ADJUST_MAIN; i<=ADJUST_N_MAIN; i++)
                XMapWindow(wm->display, b[i-CLICK_TASKBAR_BUTTON_BEGIN]), n++;
        else
            for(size_t i=ADJUST_MAIN; i<=ADJUST_N_MAIN; i++)
                XUnmapWindow(wm->display, b[i-CLICK_TASKBAR_BUTTON_BEGIN]), n--;
        STATUS_BAR *s=&wm->taskbar.status_bar;
        s->w+=n*TASKBAR_BUTTON_WIDTH;
        XResizeWindow(wm->display, s->win, s->w, s->h);
    }
}

void update_title_bar_layout(WM *wm)
{
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
            if(should_button_visible(wm, i))
                XMapWindow(wm->display, c->buttons[i]);
            else
                XUnmapWindow(wm->display, c->buttons[i]);
        RECT r=get_title_area_rect(wm, c);
        XResizeWindow(wm->display, c->title_area, r.w, r.h);
    }
}

void pointer_focus_client(WM *wm, XEvent *e, FUNC_ARG arg)
{
    focus_client(wm, win_to_client(wm, e->xbutton.window));
}

void pointer_move_resize_client(WM *wm, XEvent *e, FUNC_ARG arg)
{
    XEvent ev;
    int i=0, old_x=e->xbutton.x_root, old_y=e->xbutton.y_root, new_x, new_y,
        x_sign, y_sign, w_sign, h_sign, dw, dh;
    CLIENT *c=wm->cur_focus_client;
    if(wm->layout==FULL || wm->layout==PREVIEW) return;
    get_rect_sign(wm, old_x, old_y, arg.resize_flag, 
        &x_sign, &y_sign, &w_sign, &h_sign);

    if(!grab_pointer_for_move_resize(wm, arg.resize_flag)) return;
    do
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            if(!i++)
                prepare_for_move_resize(wm);
            new_x=ev.xmotion.x, new_y=ev.xmotion.y;
            dw=w_sign*(new_x-old_x), dh=h_sign*(new_y-old_y);
            if(dw>=0 || (dw<0 && c->w>=-dw+MOVE_RESIZE_INC))
                c->x+=x_sign*(new_x-old_x), c->w+=dw;
            if(dh>=0 || (dh<0 && c->h>=-dh+MOVE_RESIZE_INC))
                c->y+=y_sign*(new_y-old_y), c->h+=dh;
            do_move_resize_client(wm, c, 0, 0, 0, 0);
            old_x=new_x, old_y=new_y;
        }
        /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
        else if(event_handlers[ev.type])
            event_handlers[ev.type](wm, &ev);
    }while(!(ev.type==ButtonRelease && ev.xbutton.button==e->xbutton.button));
    XUngrabPointer(wm->display, CurrentTime);
}

bool grab_pointer_for_move_resize(WM *wm, bool resize_flag)
{
    Cursor c=resize_flag ? wm->cursors[RESIZE_CURSOR] : wm->cursors[MOVE_CURSOR];
    switch (XGrabPointer(wm->display, wm->root_win, False, POINTER_MASK,
                GrabModeAsync, GrabModeAsync, None, c, CurrentTime))
    {
        case GrabSuccess : return true;
        case GrabNotViewable :
            fprintf(stderr, "錯誤：定位器限定窗口不可顯！"); return false;
        case AlreadyGrabbed :
            fprintf(stderr, "錯誤：別的程序主動獨享了定位器！"); return false;
        case GrabFrozen :
            fprintf(stderr, "錯誤：獨享請求發生的時間不合理！"); return false;
        default : return false;
    }
}

bool query_pointer_for_move_resize(WM *wm, int *x, int *y, Window *win)
{
    Window root_win;
    int root_x, root_y;
    unsigned int mask;
    if(XQueryPointer(wm->display, wm->root_win, &root_win, win,
            &root_x, &root_y, x, y, &mask) == False)
    {
        fprintf(stderr, "錯誤：定位指針不在當前屏幕！\n");
        return false;
    }
    else
        return true;
}

/* 功能：根據定位器坐標（px和py），獲取窗口坐標和尺寸的變化量的符號
 * 說明：
 *     把聚焦窗口等分爲四行四列的矩形區域。若定位器初始位置在對角線區域，則可
 * 雙向調節窗口尺寸。否則單向調節。具體規定如下圖所示：
 *     ---------------------------------
 *     | dx dy |  0 dy |  0 dy |  0 dy |
 *     | -w -h |  0 -h |  0 -h | +w -h |
 *     ---------------------------------
 *     | dx  0 | dx dy |  0 dy |  0  0 |
 *     | -w  0 | -w -h | +w -h | +w  0 |
 *     ---------------------------------
 *     | dx  0 | dx  0 |  0  0 |  0  0 |
 *     | -w  0 | -w +h | +w +h | +w  0 |
 *     ---------------------------------             ---------
 *     | dx  0 |  0  0 |  0  0 |  0  0 |             | xs ys |
 *     | -w +h |  0 +h |  0 +h | +w +h |             | ws hs |
 *     ---------------------------------             ---------
 *                區域行爲規定                      區域標注說明
 *               ==============                    ==============
 *     注：
 *         1、表中dx、dy分別表示聚焦窗口左上角坐標變化量不爲0。如爲0，則
 *            直接在相應位置標明。對於X而言，不爲0意味着聚焦窗口要移動。
 *         2、表中+w、+h分別表示聚焦窗口寬度、高度的變化量不爲0，且與相應
 *            的dx、dy正負號相同。如爲0，則直接在相應位置標明。
 *         3、表中-w、-h分別表示聚焦窗口寬度、高度的變化量不爲0，且與相應
 *            的dx、dy正負號相反。如爲0，則直接在相應位置標明。
 */
void get_rect_sign(WM *wm, int px, int py, bool resize_flag, int *xs, int *ys, int *ws, int *hs)
{
    CLIENT *c=wm->cur_focus_client;

    if(resize_flag)
        *xs=*ys=*ws=*hs=0;
    else
    {   *xs=*ys=1, *ws=*hs=0; return; }

    if(px < c->x+c->w/4)
        *xs=1, *ws=-1;
    else if(px < c->x+c->w/2)
    {   if(py >= c->y+c->h/4 && py < c->y+c->h*3/4) *xs=1, *ws=-1; }
    else if(px < c->x+c->w*3/4)
    {   if(py >= c->y+c->h/4 && py < c->y+c->h*3/4) *ws=1; }
    else
        *ws=1;

    if(py < c->y+c->h/4)
        *ys=1, *hs=-1;
    else if(py < c->y+c->h/2)
    {   if(px >= c->x+c->w/4 && px < c->x+c->w*3/4) *ys=1, *hs=-1; }
    else if(py < c->y+c->h*3/4)
    {   if(px >= c->x+c->w/4 && px < c->x+c->w*3/4) *hs=1; }
    else
        *hs=1;
}

void apply_rules(WM *wm, CLIENT *c)
{
    XClassHint ch={NULL, NULL};
    c->place_type=NORMAL;
    if(!XGetClassHint(wm->display, c->win, &ch))
        return ;

    WM_RULE *p=RULES;
    for(size_t i=0; i<ARRAY_NUM(RULES); i++, p++)
    {
        if( (!p->app_class || strstr(ch.res_class, p->app_class))
            && (!p->app_name || strstr(ch.res_name, p->app_name)))
            c->place_type=p->place_type;
    }

    if(ch.res_class)
        XFree(ch.res_class);
    if(ch.res_name)
        XFree(ch.res_name);
}

void set_default_rect(WM *wm, CLIENT *c)
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

void adjust_n_main_max(WM *wm, XEvent *e, FUNC_ARG arg)
{
    wm->n_main_max+=arg.n;
    if(wm->n_main_max < 1)
        wm->n_main_max=1;
    update_layout(wm);
}

/* 在固定區域比例不變的情況下調整主區域比例，主、次區域比例此消彼長 */
void adjust_main_area_ratio(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->n_normal > wm->n_main_max)
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
void adjust_fixed_area_ratio(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->n_fixed)
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

void change_area(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->cur_focus_client != wm->clients)
    {
        AREA_TYPE type=arg.area_type;
        switch(type)
        {
            case MAIN_AREA: to_main_area(wm); break;
            case SECOND_AREA: to_second_area(wm); break;
            case FIXED_AREA: to_fixed_area(wm); break;
            case FLOATING_AREA: to_floating_area(wm); break;
        }
        CLIENT *c=wm->cur_focus_client;
        if(e->type == KeyPress)
            XWarpPointer(wm->display, None, c->win, 0, 0, 0, 0, c->w/2, c->h/2);
    }
}

void to_main_area(WM *wm)
{
    if(wm->layout == TILE)
    {
        move_client(wm, wm->cur_focus_client, wm->clients, NORMAL);
        update_layout(wm);
    }
}

bool is_in_main_area(WM *wm, CLIENT *c)
{
    CLIENT *p=wm->clients->next;
    for(int i=0; i<wm->n_main_max; i++, p=p->next)
        if(p == c)
            return true;
    return false;
}

void del_client_node(CLIENT *c)
{
    c->prev->next=c->next;
    c->next->prev=c->prev;
}

void update_n_for_del(WM *wm, CLIENT *c)
{
    wm->n--;
    switch(c->place_type)
    {
        case NORMAL: wm->n_normal--; break;
        case FIXED: wm->n_fixed--; break;
        case FLOATING: wm->n_float--; break;
    }
}

void add_client_node(CLIENT *head, CLIENT *c)
{
    c->prev=head;
    c->next=head->next;
    head->next=c;
    c->next->prev=c;
}

void update_n_for_add(WM *wm, CLIENT *c)
{
    wm->n++;
    switch(c->place_type)
    {
        case NORMAL: wm->n_normal++; break;
        case FIXED: wm->n_fixed++; break;
        case FLOATING: wm->n_float++; break;
    }
}

void to_second_area(WM *wm)
{
    CLIENT *to, *from=wm->cur_focus_client;

    if( wm->layout == TILE
        && (wm->n_normal > wm->n_main_max
        || (wm->n_normal==wm->n_main_max && !is_in_main_area(wm, from)))
        && (to=get_second_area_head(wm)))
    {
        move_client(wm, from, to, NORMAL);
        update_layout(wm);
    }
}

CLIENT *get_second_area_head(WM *wm)
{
    if(wm->n_normal >= wm->n_main_max)
    {
        CLIENT *head=wm->clients->next;
        for(int i=0; i<wm->n_main_max; i++)
            head=head->next;
        return head;
    }
    return NULL;
}

void to_fixed_area(WM *wm)
{
    CLIENT *to, *from=wm->cur_focus_client;
    if(wm->layout == TILE || from->place_type==FIXED)
    {
        to=get_area_head(wm, FIXED);
        if(from->place_type == FLOATING)
            to=to->next;
        move_client(wm, from, to, FIXED);
        update_layout(wm);
    }
}

void to_floating_area(WM *wm)
{
    CLIENT *from=wm->cur_focus_client;
    if(from->place_type == FLOATING)
        return;
    set_floating_size(from);
    move_client(wm, from, from, FLOATING);
    update_layout(wm);
}

void set_floating_size(CLIENT *c)
{
    if(c->w >= 2*MOVE_RESIZE_INC)
        c->w-=MOVE_RESIZE_INC;
    if(c->h >= 2*MOVE_RESIZE_INC)
        c->h-=MOVE_RESIZE_INC;
}

void pointer_change_area(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->layout==TILE)
    {
        CLIENT *from=wm->cur_focus_client, *to;
        if(from!=wm->clients && grab_pointer_for_move_resize(wm, false))
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
            {
                move_client(wm, from, to, to->place_type);
                update_layout(wm);
            }
        }
    }
}

int compare_client_order(WM *wm, CLIENT *c1, CLIENT *c2)
{
    if(c1 == c2)
        return 0;
    for(CLIENT *c=c1; c!=wm->clients; c=c->next)
        if(c == c2)
            return -1;
    return 1;
}

void move_client(WM *wm, CLIENT *from, CLIENT *to, PLACE_TYPE type)
{
    if(from != to)
    {
        del_client_node(from);
        if(compare_client_order(wm, from, to)==1 && to!=wm->clients)
            add_client_node(to->prev, from);
        else
            add_client_node(to, from);
    }
    update_n_for_del(wm, from);
    from->place_type=type;
    update_n_for_add(wm, from);
    raise_client(wm);
}

/* 僅在移動窗口、聚焦窗口時才有可能需要提升 */
void raise_client(WM *wm)
{
    CLIENT *c=wm->cur_focus_client;
    Window wins[]={wm->taskbar.win, c->frame};
    if(c->place_type == FLOATING)
        XRaiseWindow(wm->display, c->frame);
    else
        XRestackWindows(wm->display, wins, ARRAY_NUM(wins));
}

void frame_client(WM *wm, CLIENT *c)
{
    RECT fr=get_frame_rect(c), tr=get_title_area_rect(wm, c);
    c->frame=XCreateSimpleWindow(wm->display, wm->root_win, fr.x, fr.y,
        fr.w, fr.h, BORDER_WIDTH, CURRENT_BORDER_COLOR, CURRENT_FRAME_COLOR);
    XSelectInput(wm->display, c->frame,
        SubstructureRedirectMask|SubstructureNotifyMask|ExposureMask|ButtonPressMask);
    c->title_area=XCreateSimpleWindow(wm->display, c->frame,
        tr.x, tr.y, tr.w, tr.h, 0, 0, CURRENT_TITLE_AREA_COLOR);
    XSelectInput(wm->display, c->title_area, ButtonPressMask|ExposureMask);
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
    {
        RECT br=get_button_rect(c, i);
        c->buttons[i]=XCreateSimpleWindow(wm->display, c->frame,
            br.x, br.y, br.w, br.h, 0, 0, CURRENT_BUTTON_COLOR);
        XSelectInput(wm->display, c->buttons[i],
            ButtonPressMask|ExposureMask|EnterWindowMask|LeaveWindowMask);
    }
    XAddToSaveSet(wm->display, c->win);
    XReparentWindow(wm->display, c->win, c->frame, 0, TITLE_BAR_HEIGHT);
    XMapWindow(wm->display, c->frame);
    XMapSubwindows(wm->display, c->frame);
}

RECT get_frame_rect(CLIENT *c)
{
    return (RECT){c->x-BORDER_WIDTH, c->y-TITLE_BAR_HEIGHT-BORDER_WIDTH,
        c->w, c->h+TITLE_BAR_HEIGHT};
}

RECT get_title_area_rect(WM *wm, CLIENT *c)
{
    return (RECT){0, 0,
        c->w-CLIENT_BUTTON_WIDTH*get_visible_button_count(wm), TITLE_BAR_HEIGHT};
}

int get_visible_button_count(WM *wm)
{
    int n=0;
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
        if(should_button_visible(wm, i))
            n++;
    return n;
}

RECT get_button_rect(CLIENT *c, size_t index)
{
    return (RECT){c->w-CLIENT_BUTTON_WIDTH*(CLIENT_BUTTON_N-index),
    (TITLE_BAR_HEIGHT-CLIENT_BUTTON_HEIGHT)/2, CLIENT_BUTTON_WIDTH, CLIENT_BUTTON_HEIGHT};
}

bool is_part_of_title_bar(CLIENT *c, Window win)
{
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
        if(win == c->buttons[i])
            return true;
    if(win == c->title_area)
        return true;
    return false;
}

void update_title_bar_text(WM *wm, CLIENT *c, Window win)
{
    char *title_str=NULL;
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
    {
        if(win == c->buttons[i])
        {
            draw_string(wm, c->buttons[i], BUTTON_TEXT_COLOR, CENTER, 0, 0,
                CLIENT_BUTTON_WIDTH, CLIENT_BUTTON_HEIGHT, CLIENT_BUTTON_TEXT[i]);
            return ;
        }
    }
    if(win==c->title_area && XFetchName(wm->display, c->win, &title_str))
    {
        RECT tr=get_title_area_rect(wm, c);
        draw_string(wm, c->title_area, TITLE_TEXT_COLOR, LEFT,
            tr.x, tr.y, tr.w, tr.h, title_str);
        XFree(title_str);
    }
}

void do_move_resize_client(WM *wm, CLIENT *c, int dx, int dy, unsigned int dw, unsigned int dh)
{
    c->x+=dx, c->y+=dy, c->w+=dw, c->h+=dh;
    RECT fr=get_frame_rect(c), tr=get_title_area_rect(wm, c);
    XMoveResizeWindow(wm->display, c->frame, fr.x, fr.y, fr.w, fr.h);
    XMoveResizeWindow(wm->display, c->win, 0, TITLE_BAR_HEIGHT, c->w, c->h);
    XMoveResizeWindow(wm->display, c->title_area, tr.x, tr.y, tr.w, tr.h);
    for(size_t i=0; i<CLIENT_BUTTON_N; i++)
    {
        RECT br=get_button_rect(c, i);
        XMoveResizeWindow(wm->display, c->buttons[i], br.x, br.y, br.w, br.h);
    }
}

void fix_win_rect(CLIENT *c)
{
    c->x+=BORDER_WIDTH;
    c->y+=BORDER_WIDTH+TITLE_BAR_HEIGHT;
    if(c->w > 2*BORDER_WIDTH+WINS_SPACE)
        c->w-=2*BORDER_WIDTH+WINS_SPACE;
    if(c->h > 2*BORDER_WIDTH+WINS_SPACE+TITLE_BAR_HEIGHT)
        c->h-=2*BORDER_WIDTH+WINS_SPACE+TITLE_BAR_HEIGHT;
}

void update_frame(WM *wm, CLIENT *c)
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

CLICK_TYPE get_click_type(WM *wm, Window win)
{
    CLICK_TYPE i;
    CLIENT *c;
    for(i=CLICK_TASKBAR_BUTTON_BEGIN; i<=CLICK_TASKBAR_BUTTON_END; i++)
        if(win == wm->taskbar.buttons[i-CLICK_TASKBAR_BUTTON_BEGIN])
            return i;
    if((c=win_to_client(wm, win)))
    {
        if(win == c->win)
            return CLICK_WIN;
        else if(win == c->frame)
            return CLICK_FRAME;
        else if(win == c->title_area)
            return CLICK_TITLE;
        else
        {
            for(i=CLICK_CLIENT_BUTTON_BEGIN; i<=CLICK_CLIENT_BUTTON_END; i++)
                if(win == c->buttons[i-CLICK_CLIENT_BUTTON_BEGIN])
                    return i;
        }
    }
    return UNDEFINED;
}

void handle_enter_notify(WM *wm, XEvent *e)
{
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        if(e->xcrossing.window == wm->taskbar.buttons[i])
        {
            update_win_background(wm, e->xcrossing.window, ENTERED_TASKBAR_BUTTON_COLOR);
            break;
        }
    }

    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
        {
            if(e->xcrossing.window == c->buttons[i])
            {
                update_win_background(wm, e->xcrossing.window,
                    i==CLOSE_WIN-CLICK_CLIENT_BUTTON_BEGIN ?
                    ENTERED_CLOSE_BUTTON_COLOR : ENTERED_NORMAL_BUTTON_COLOR);
                break;
            }
        }
    }
}

void handle_leave_notify(WM *wm, XEvent *e)
{
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        if(e->xcrossing.window == wm->taskbar.buttons[i])
        {
            update_win_background(wm, e->xcrossing.window, TASKBAR_BUTTON_COLOR);
            break;
        }
    }
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        for(size_t i=0; i<CLIENT_BUTTON_N; i++)
        {
            if(e->xcrossing.window == c->buttons[i])
            {
                update_win_background(wm, e->xcrossing.window,
                    c==wm->cur_focus_client ?
                    CURRENT_BUTTON_COLOR : NORMAL_BUTTON_COLOR);
                break;
            }
        }
    }
}

void maximize_client(WM *wm, XEvent *e, FUNC_ARG unused)
{
    wm->prev_focus_client=wm->cur_focus_client;
    CLIENT *c=wm->cur_focus_client;
    if(c != wm->clients)
    {
        c->x=BORDER_WIDTH;
        c->y=BORDER_WIDTH+TITLE_BAR_HEIGHT;
        c->w=wm->screen_width-2*BORDER_WIDTH;
        c->h=wm->screen_height-2*BORDER_WIDTH-TITLE_BAR_HEIGHT;
        prepare_for_move_resize(wm);
        do_move_resize_client(wm, c, 0, 0, 0, 0);
    }
}

void minimize_client(WM *wm, XEvent *e, FUNC_ARG unused)
{
    wm->prev_focus_client=wm->cur_focus_client;
    CLIENT *c=wm->cur_focus_client;
    if(c != wm->clients)
    {
        c->x=BORDER_WIDTH;
        c->y=wm->taskbar.y+TITLE_BAR_HEIGHT+BORDER_WIDTH;
        c->w=wm->screen_width/4-2*BORDER_WIDTH;
        c->h=MOVE_RESIZE_INC;
        prepare_for_move_resize(wm);
        do_move_resize_client(wm, c, 0, 0, 0, 0);
    }
}

bool should_button_visible(WM *wm, size_t index)
{
    return (wm->layout==TILE || (wm->layout==STACK && index>=MIN_WIN-CLICK_CLIENT_BUTTON_BEGIN)
        || (wm->layout==PREVIEW && index==CLOSE_WIN-CLICK_CLIENT_BUTTON_BEGIN));
}
