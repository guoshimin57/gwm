/* *************************************************************************
 *     gwm.c：實現窗口管理器的主要部分。
 *     版權 (C) 2020 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

KEYBINDS keybinds_list[]=
{
    {CMD_KEY,    XK_t,            exec,                       SH_CMD("lxterminal")},
    {CMD_KEY,    XK_f,            exec,                       SH_CMD("xdg-open ~")},
    {CMD_KEY,    XK_w,            exec,                       SH_CMD("xwininfo -wm >log")},
    {CMD_KEY,    XK_p,            exec,                       SH_CMD("dmenu_run")},
    {CMD_KEY,    XK_q,            exec,                       SH_CMD("qq")},
    {CMD_KEY,    XK_s,            exec,                       SH_CMD("stardict")},
    {WM_KEY,     XK_Up,           key_move_resize_client,     {.direction=UP}},
    {WM_KEY,     XK_Down,         key_move_resize_client,     {.direction=DOWN}},
    {WM_KEY,     XK_Left,         key_move_resize_client,     {.direction=LEFT}},
    {WM_KEY,     XK_Right,        key_move_resize_client,     {.direction=RIGHT}},
    {WM_KEY,     XK_bracketleft,  key_move_resize_client,     {.direction=UP2UP}},
    {WM_KEY,     XK_bracketright, key_move_resize_client,     {.direction=UP2DOWN}},
    {WM_KEY,     XK_semicolon,    key_move_resize_client,     {.direction=DOWN2UP}},
    {WM_KEY,     XK_quoteright,   key_move_resize_client,     {.direction=DOWN2DOWN}},
    {WM_KEY,     XK_9,            key_move_resize_client,     {.direction=LEFT2LEFT}},
    {WM_KEY,     XK_0,            key_move_resize_client,     {.direction=LEFT2RIGHT}},
    {WM_KEY,     XK_minus,        key_move_resize_client,     {.direction=RIGHT2LEFT}},
    {WM_KEY,     XK_equal,        key_move_resize_client,     {.direction=RIGHT2RIGHT}},
    {WM_KEY,     XK_Delete,       quit_wm,                    {0}},
    {WM_KEY,     XK_c,            close_win,                  {0}},
    {WM_KEY,     XK_Tab,          next_win,                   {0}},
    {WM_KEY,     XK_f,            change_layout,              {.layout=FULL}},
    {WM_KEY,     XK_p,            change_layout,              {.layout=PREVIEW}},
    {WM_KEY,     XK_s,            change_layout,              {.layout=STACK}},
    {WM_KEY,     XK_t,            change_layout,              {.layout=TILE}},
    {WM_KEY,     XK_i,            adjust_n_main_max,          {.n=1}},
    {WM_SKEY,    XK_i,            adjust_n_main_max,          {.n=-1}},
    {WM_KEY,     XK_m,            adjust_main_area_ratio,     {.change_ratio=0.01}},
    {WM_SKEY,    XK_m,            adjust_main_area_ratio,     {.change_ratio=-0.01}},
    {WM_KEY,     XK_x,            adjust_fixed_area_ratio,    {.change_ratio=0.01}},
    {WM_SKEY,    XK_x,            adjust_fixed_area_ratio,    {.change_ratio=-0.01}},
    {WM_KEY,     XK_F1,           key_change_area,            {.area_type=MAIN_AREA}},
    {WM_KEY,     XK_F2,           key_change_area,            {.area_type=SECOND_AREA}},
    {WM_KEY,     XK_F3,           key_change_area,            {.area_type=FIXED_AREA}},
    {WM_KEY,     XK_F4,           key_change_area,            {.area_type=FLOATING_AREA}},
};

BUTTONBINDS buttonbinds_list[]=
{
    {WM_KEY, Button1, pointer_move_resize_win, {.resize_flag=false}},
    {WM_KEY, Button3, pointer_move_resize_win, {.resize_flag=true}},
    {WM_SKEY, Button1, pointer_change_area, {0}},
};

WM_RULE rules[]=
{
    {"Stardict", "stardict", FLOATING},
    {"Qq", "qq", FIXED},
    {"Peek", "peek", FLOATING},
};

void (*event_handlers[])(WM *, XEvent *)=
{
    [ButtonPress]       = handle_button_press,
    [ConfigureRequest]  = handle_config_request,
    [DestroyNotify]     = handle_destroy_notify,
    [Expose]            = handle_expose,
    [KeyPress]          = handle_key_press,
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
    wm->black=BlackPixel(wm->display, wm->screen);
    wm->white=WhitePixel(wm->display, wm->screen);
	wm->mod_map=XGetModifierMapping(wm->display);
    wm->root_win=RootWindow(wm->display, wm->screen);
    wm->gc=XCreateGC(wm->display, wm->root_win, 0, NULL);
    wm->layout=TILE;
    wm->main_area_ratio=DEFAULT_MAIN_AREA_RATIO;
    wm->fixed_area_ratio=DEFAULT_FIXED_AREA_RATIO;
    wm->n_main_max=2;
}

void set_wm(WM *wm)
{
    XSetErrorHandler(my_x_error_handler);
    XSelectInput(wm->display, wm->root_win, ROOT_EVENT_MASK);
    create_font_set(wm);
    create_status_bar(wm);
    create_clients(wm);
    update_layout(wm);
    grab_keys(wm);
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

void create_status_bar(WM *wm)
{
    STATUS_BAR *b=&wm->status_bar;
    b->x=0;
    b->y=wm->screen_height-STATUS_BAR_HEIGHT;
    b->w=wm->screen_width;
    b->h=STATUS_BAR_HEIGHT;
    b->win=XCreateSimpleWindow(wm->display, wm->root_win, b->x, b->y,
        b->w, b->h, 0, 0, GREY21);
    b->text="gwm";
    XSelectInput(wm->display, wm->status_bar.win, ExposureMask);
    XMapRaised(wm->display, wm->status_bar.win);
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

    wm->focus_client=wm->clients=malloc_s(sizeof(CLIENT));
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
    return ( win != wm->status_bar.win
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
    if(c->place_type == FLOATING)
        set_default_rect(wm, c);
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
        case STACK: set_stack_layout(wm); break;
        case TILE: set_tile_layout(wm); break;
        default: break;
    }
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
        XMoveResizeWindow(wm->display, c->win, c->x, c->y, c->w, c->h);
}

void set_full_layout(WM *wm)
{
    for(CLIENT *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        c->w=wm->screen_width;
        c->h=wm->screen_height;
        c->x=c->y=0;
    }
}

void set_preview_layout(WM *wm)
{
    int i=0, rows, cols, w, h, ch=(wm->screen_height-wm->status_bar.h);
    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=wm->n; cols++)
        if(cols*cols >= wm->n)
            break;
    rows=(cols-1)*cols>=wm->n ? cols-1 : cols;
    w=wm->screen_width/cols;
    h=ch/rows;
    i=wm->n-1;
    for(CLIENT *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        c->x=(i%cols)*w;
        c->y=(i/cols)*h;
        /* 下邊和右邊的窗口佔用剩餘空間 */
        c->w=(i+1)%cols ? w : w+(wm->screen_width-w*cols);
        c->h=i<cols*(rows-1) ? h : h+(ch-h*rows);
        i--;
    }
}

void set_stack_layout(WM *wm)
{
    for(CLIENT *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(c->w >= 2*MOVE_RESIZE_INC)
            c->w-=MOVE_RESIZE_INC;
        if(c->h >= 2*MOVE_RESIZE_INC)
            c->h-=MOVE_RESIZE_INC;
    }
}

void set_tile_layout(WM *wm)
{
    CLIENT *c;
    unsigned int i, j, mw, sw, fw, mh, sh, fh, h; /* m=main, s=sec, f=fixed */

    mw=wm->main_area_ratio*wm->screen_width;
    fw=wm->screen_width*wm->fixed_area_ratio;
    sw=wm->screen_width-fw-mw;
    h=wm->screen_height-wm->status_bar.h;
    mh=wm->n_normal>=wm->n_main_max ? h/wm->n_main_max : (wm->n_normal ? h/wm->n_normal : h);
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
    }
}

void grab_keys(WM *wm)
{
    unsigned int num_lock_mask=get_num_lock_mask(wm);
    unsigned int masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    KeyCode code;
    XUngrabKey(wm->display, AnyKey, AnyModifier, wm->root_win);
    for(size_t i=0; i<ARRAY_NUM(keybinds_list); i++)
        if((code=XKeysymToKeycode(wm->display, keybinds_list[i].keysym)))
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabKey(wm->display, code, keybinds_list[i].modifier|masks[j],
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
    XUngrabButton(wm->display, AnyButton, AnyModifier, c->win);
    for(size_t i=0; i<ARRAY_NUM(buttonbinds_list); i++)
        XGrabButton(wm->display, buttonbinds_list[i].button,
            buttonbinds_list[i].modifier, c->win, False, BUTTON_MASK,
            GrabModeAsync, GrabModeAsync, None, None);
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
    BUTTONBINDS bb;
	for(size_t i=0; i<ARRAY_NUM(buttonbinds_list); i++)
    {
        bb=buttonbinds_list[i];
		if( bb.button == e->xbutton.button
            && is_equal_modifier_mask(wm, bb.modifier, e->xbutton.state)
            && bb.func)
            bb.func(wm, e, bb.arg);
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
        config_managed_win(wm, c);
    else
        config_unmanaged_win(wm, &cr);
}

void config_managed_win(WM *wm, CLIENT *c)
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
        if(c->win == win)
            return c;
    return NULL;
}

void handle_destroy_notify(WM *wm, XEvent *e)
{
    CLIENT *c;
    if((c=win_to_client(wm, e->xdestroywindow.window)))
    {
        bool flag=(c->place_type!=FLOATING);
        del_client(wm, e->xdestroywindow.window);
        if(wm->layout!=STACK && (wm->layout!=TILE || flag))
            update_layout(wm);
    }
}
 
void del_client(WM *wm, Window win)
{
    CLIENT *c=win_to_client(wm, win);
    if(c)
    {
        del_client_node(c);
        update_n_for_del(wm, c);
        free(c);
        focus_client(wm, wm->clients);
    }
}

void handle_expose(WM *wm, XEvent *e)
{
    Window win=e->xexpose.window;
    STATUS_BAR *b=&wm->status_bar;

    if(win == b->win)
        draw_string(wm, b->win, wm->white, CENTER, 0, 0, b->w, b->h, b->text);
}

void handle_key_press(WM *wm, XEvent *e)
{
    int n;
    KeyCode kc=e->xkey.keycode;
	KeySym *keysym=XGetKeyboardMapping(wm->display, kc, 1, &n);
    KEYBINDS kb;
	for(size_t i=0; i<ARRAY_NUM(keybinds_list); i++)
    {
        kb=keybinds_list[i];
		if( *keysym == kb.keysym
            && is_equal_modifier_mask(wm, kb.modifier, e->xkey.state)
            && kb.func)
            kb.func(wm, e, kb.arg);
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
        CLIENT *c;
        add_client(wm, win);
        c=win_to_client(wm, win);
        if(wm->layout==STACK || (wm->layout==TILE && c->place_type==FLOATING))
            XMoveResizeWindow(wm->display, c->win, c->x, c->y, c->w, c->h);
        else
            update_layout(wm);
    }
}

void handle_unmap_notify(WM *wm, XEvent *e)
{
    CLIENT *c;
    if((c=win_to_client(wm, e->xunmap.window)))
    {
        bool flag=(c->place_type!=FLOATING);
        del_client(wm, e->xunmap.window);
        if(wm->layout!=STACK && (wm->layout!=TILE || flag))
            update_layout(wm);
    }
}

void handle_property_notify(WM *wm, XEvent *e)
{
    STATUS_BAR *b=&wm->status_bar;
    XTextProperty name;
    if( e->xproperty.window==wm->root_win && e->xproperty.atom==XA_WM_NAME
        && XGetTextProperty(wm->display, wm->root_win, &name, XA_WM_NAME))
            draw_string(wm, b->win, wm->white, CENTER,
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
        CLIENT *c=wm->focus_client;
        prepare_for_move_resize(wm);
        if(is_valid_move_resize(wm, c, p[0], p[1], p[2], p[3]))
            XMoveResizeWindow(wm->display, c->win,
                c->x+=p[0], c->y+=p[1], c->w+=p[2], c->h+=p[3]);
    }
}

void prepare_for_move_resize(WM *wm)
{
    CLIENT *c=wm->focus_client;
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

bool is_valid_move_resize(WM *wm, CLIENT *c, int dx, int dy, int dw, int dh)
{
    int x=c->x+dx, y=c->y+dy, w=c->w+dw, h=c->h+dh,
        sw=wm->screen_width, sh=wm->screen_height;
    /* 通過求窗口與屏幕是否有交集來判斷窗口是否已經在屏幕外，即是否合法。
     * 若滿足以下條件，則有交集：窗口與屏幕中心距≤窗口半邊長+屏幕半邊長。
     * 即：|x+w/2-0-sw/2|＜|w/2+sw/2| 且 |y+h/2-0-sh/2|＜|h/2+sh/2|。
     * 兩邊同乘以2，得：|2*x+w-sw|＜|w+sw| 且 |2*y+h-sh|＜|h+sh|。
     */
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
    if(wm->focus_client->win != wm->root_win)
    {
        bool flag=(wm->focus_client->place_type!=FLOATING);
        if(!send_event(wm, XInternAtom(wm->display, "WM_DELETE_WINDOW", False)))
        {
            XGrabServer(wm->display);
            XKillClient(wm->display, wm->focus_client->win);
            XUngrabServer(wm->display);
        }
        del_client(wm, wm->focus_client->win);
        if(wm->layout!=STACK && (wm->layout!=TILE || flag))
            update_layout(wm);
    }
    else
        fprintf(stderr, "錯誤：不能關閉根窗口！\n");
}

int send_event(WM *wm, Atom protocol)
{
	int i, n;
	Atom *protocols;
	XEvent event;

	if(XGetWMProtocols(wm->display, wm->focus_client->win, &protocols, &n))
    {
        for(i=0; i<n; i++)
        {
            if(protocols[i] == protocol)
                break;
        }
		XFree(protocols);
	}
	if(i < n)
    {
		event.type=ClientMessage;
		event.xclient.window=wm->focus_client->win;
		event.xclient.message_type=XInternAtom(wm->display, "WM_PROTOCOLS", False);
		event.xclient.format=32;
		event.xclient.data.l[0]=protocol;
		event.xclient.data.l[1]=CurrentTime;
		XSendEvent(wm->display, wm->focus_client->win, False, NoEventMask, &event);
	}
	return i<n ;
}

void next_win(WM *wm, XEvent *e, FUNC_ARG unused)
{
    if(wm->n > 1)
        focus_client(wm, wm->focus_client->prev);
}

void focus_client(WM *wm, CLIENT *c)
{
    if(!c)
        c=wm->clients;
    wm->focus_client=c;
    if(c != wm->clients)
        raise_client(wm);
    XSetInputFocus(wm->display, c->win, RevertToParent, CurrentTime);
}

void change_layout(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->layout != arg.layout)
    {
        wm->layout=arg.layout;
        update_layout(wm);
    }
}

void pointer_move_resize_win(WM *wm, XEvent *e, FUNC_ARG arg)
{
    CLIENT *c;
    XEvent ev;
    Window focus_win;
    int i=0, old_x, old_y, new_x, new_y, x_sign, y_sign, w_sign, h_sign;

    if(!grab_pointer_for_move_resize(wm)) return;
    if(!query_pointer_for_move_resize(wm, &old_x, &old_y, &focus_win)) return;
    c=win_to_client(wm, focus_win);
    focus_client(wm, c);
    c=wm->focus_client;
    if(wm->layout==FULL || wm->layout==PREVIEW) return;
    get_rect_sign(wm, old_x, old_y, arg.resize_flag, 
        &x_sign, &y_sign, &w_sign, &h_sign);

    do
    {
        XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
        if(ev.type == MotionNotify)
        {
            if(!i++)
                prepare_for_move_resize(wm);
            new_x=ev.xmotion.x, new_y=ev.xmotion.y;
            c->x+=x_sign*(new_x-old_x), c->y+=y_sign*(new_y-old_y);
            c->w+=w_sign*(new_x-old_x), c->h+=h_sign*(new_y-old_y);
            XMoveResizeWindow(wm->display, c->win, c->x, c->y, c->w, c->h);
            old_x=new_x, old_y=new_y;
        }
        /* 因設置了獨享定位器且XMaskEvent會阻塞，故應處理按、放按鈕之間的事件 */
        else if(event_handlers[ev.type])
            event_handlers[ev.type](wm, &ev);
    }while((ev.type!=ButtonRelease || ev.xbutton.button!=e->xbutton.button));
    XUngrabPointer(wm->display, CurrentTime);
}

bool grab_pointer_for_move_resize(WM *wm)
{
    switch (XGrabPointer(wm->display, wm->root_win, False, POINTER_MASK,
                GrabModeAsync, GrabModeAsync, None, None, CurrentTime))
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
    CLIENT *c=wm->focus_client;

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
    WM_RULE *r;

    c->place_type=NORMAL;
    if(!XGetClassHint(wm->display, c->win, &ch))
        return ;
    for(size_t i=0; i<ARRAY_NUM(rules); i++)
    {
        r=rules+i;
        if( (!r->app_class || strstr(ch.res_class, r->app_class))
            && (!r->app_name || strstr(ch.res_name, r->app_name)))
            c->place_type=r->place_type;
    }

    if(ch.res_class) XFree(ch.res_class);
    if(ch.res_name) XFree(ch.res_name);
}

void set_default_rect(WM *wm, CLIENT *c)
{
    c->x=wm->screen_width/4;
    c->y=wm->screen_height/4;
    c->w=wm->screen_width/2;
    c->h=wm->screen_height/2;
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

void key_change_area(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->focus_client != wm->clients)
    {
        AREA_TYPE type=arg.area_type;
        switch(type)
        {
            case MAIN_AREA: to_main_area(wm); break;
            case SECOND_AREA: to_second_area(wm); break;
            case FIXED_AREA: to_fixed_area(wm); break;
            case FLOATING_AREA: to_floating_area(wm); break;
        }
    }
}

void to_main_area(WM *wm)
{
    if(wm->layout == TILE)
    {
        move_client(wm, wm->focus_client, wm->clients, NORMAL);
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
    CLIENT *to, *from=wm->focus_client;

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
    CLIENT *to, *from=wm->focus_client;
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
    CLIENT *from=wm->focus_client;
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
    if(wm->layout==TILE && grab_pointer_for_move_resize(wm))
    {
        CLIENT *from=win_to_client(wm, e->xbutton.window), *to;
        focus_client(wm, from);
        from=wm->focus_client;
        if(from != wm->clients)
        {
            XEvent ev;
            do
            {
                XMaskEvent(wm->display, ROOT_EVENT_MASK, &ev);
                if(event_handlers[ev.type])
                    event_handlers[ev.type](wm, &ev);
            }while((ev.type!=ButtonRelease || ev.xbutton.button!=e->xbutton.button));
            XUngrabPointer(wm->display, CurrentTime);

            /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在的按下按鈕時
             * 定位器所有的窗口的外邊。因此，接收事件的是根窗口 */
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
    if(from->place_type != type)
        raise_client(wm);
    from->place_type=type;
    update_n_for_add(wm, from);
}

/* 僅在移動窗口、聚焦窗口時才有可能需要提升 */
void raise_client(WM *wm)
{
    CLIENT *c=wm->focus_client;
    Window wins[]={wm->status_bar.win, c->win};
    if(c->place_type == FLOATING)
        XRaiseWindow(wm->display, c->win);
    else
        XRestackWindows(wm->display, wins, ARRAY_NUM(wins));
}
