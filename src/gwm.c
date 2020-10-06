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
    {CMD_KEY,    XK_t,            exec,            SH_CMD("lxterminal")},
    {CMD_KEY,    XK_f,            exec,            SH_CMD("xdg-open ~")},
    {CMD_KEY,    XK_w,            exec,            SH_CMD("xwininfo -wm >log")},
    {CMD_KEY,    XK_p,            exec,            SH_CMD("dmenu_run")},
    {CMD_KEY,    XK_q,            exec,            SH_CMD("qq")},
    {CMD_KEY,    XK_s,            exec,            SH_CMD("stardict")},
    {WM_KEY,     XK_Up,           key_move_win,    {.direction=up}},
    {WM_KEY,     XK_Down,         key_move_win,    {.direction=down}},
    {WM_KEY,     XK_Left,         key_move_win,    {.direction=left}},
    {WM_KEY,     XK_Right,        key_move_win,    {.direction=right}},
    {WM_KEY,     XK_bracketleft,  key_resize_win,  {.direction=up2up}},
    {WM_KEY,     XK_bracketright, key_resize_win,  {.direction=up2down}},
    {WM_KEY,     XK_semicolon,    key_resize_win,  {.direction=down2up}},
    {WM_KEY,     XK_quoteright,   key_resize_win,  {.direction=down2down}},
    {WM_KEY,     XK_9,            key_resize_win,  {.direction=left2left}},
    {WM_KEY,     XK_0,            key_resize_win,  {.direction=left2right}},
    {WM_KEY,     XK_minus,        key_resize_win,  {.direction=right2left}},
    {WM_KEY,     XK_equal,        key_resize_win,  {.direction=right2right}},
    {WM_KEY,     XK_Delete,       quit_wm,         {0}},
    {WM_KEY,     XK_c,            close_win,       {0}},
    {WM_KEY,     XK_Tab,          next_win,        {0}},
    {WM_KEY,     XK_backslash,    toggle_float,    {0}},
    {WM_KEY,     XK_f,            change_layout,   {.layout=full}},
    {WM_KEY,     XK_p,            change_layout,   {.layout=preview}},
    {WM_KEY,     XK_s,            change_layout,   {.layout=stack}},
    {WM_KEY,     XK_t,            change_layout,   {.layout=tile}},
};

BUTTONBINDS buttonbinds_list[]=
{
    {WM_KEY, Button1, pointer_move_resize_win, {.resize_flag=false}},
    {WM_KEY, Button3, pointer_move_resize_win, {.resize_flag=true}},
};

WM_RULE rules[]=
{
    {"Stardict", "stardict", floating},
    {"Qq", "qq", fixed},
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
		fprintf(stderr, "gwm: cannot open display\n");
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
    wm->layout=tile;
    wm->main_area_ratio=0.5;
    wm->fixed_area_ratio=0.25;
}

void set_wm(WM *wm)
{
    XSetErrorHandler(my_x_error_handler);
    XSelectInput(wm->display, wm->root_win, SubstructureRedirectMask
        |SubstructureNotifyMask|PropertyChangeMask|ButtonPressMask);
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
        fprintf(stderr, "已經有其他窗口管理器在運行！\n");
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
    wm->font_set=XCreateFontSet(wm->display, "*-24-*", &missing_charset_list,
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
        b->w, b->h, 0, 0, wm->white);
    XSelectInput(wm->display, wm->status_bar.win, ExposureMask);
    XMapRaised(wm->display, wm->status_bar.win);
}

void print_error_msg(Display *display, XErrorEvent *e)
{
    fprintf(stderr, "X錯誤：資源號=%#lx, 請求量=%lu, 錯誤碼=%d, "
        "主請求碼=%d, 次請求碼=%d\n", e->resourceid, e->serial,
        e->error_code, e->request_code, e->minor_code);
}

void create_clients(WM *wm)
{
    Window root, parent, *child=NULL;
    unsigned int n;

    /* 頭插法生成帶表頭結點的雙向循環鏈表 */
    wm->focus_client=wm->clients=Malloc(sizeof(CLIENT));
    wm->clients->win=wm->root_win;
    wm->n=wm->n_normal=wm->n_float=wm->n_fixed=0;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &n))
    {
        for(size_t i=0; i<n; i++)
        {
            XWindowAttributes attr;
            if( child[i] != wm->status_bar.win
                && XGetWindowAttributes(wm->display, child[i], &attr)
                && attr.map_state != IsUnmapped
                && get_state_hint(wm, child[i]) == NormalState
                && !attr.override_redirect)
                add_client(wm, child[i]);
        }
        XFree(child);
    }
    else
        fprintf(stderr, "查詢窗口清單失敗！\n");
}

void *Malloc(size_t size)
{
    void *p=malloc(size);
    if(p == NULL)
    {
        fprintf(stderr, "申請內存失敗！\n");
        exit(EXIT_FAILURE);
    }
    return p;
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
    c=Malloc(sizeof(CLIENT));
    c->win=win;
    c->prev=wm->clients;
    c->next=wm->clients->next;
    wm->clients->next=c;
    c->next->prev=c;
    wm->focus_client=c;
    c->place_type=normal;
    grab_buttons(wm);
    apply_rules(wm, c);
}

void update_layout(WM *wm)
{
    if(wm->n == 0)
        return ;
    switch(wm->layout)
    {
        case full: set_full_layout(wm); break;
        case preview: set_preview_layout(wm); break;
        case stack: set_stack_layout(wm); break;
        case tile: set_tile_layout(wm); break;
        default: break;
    }
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
        XMoveResizeWindow(wm->display, c->win, c->x, c->y, c->w, c->h);
}

void set_full_layout(WM *wm)
{
    CLIENT *c=wm->focus_client;
    c->w=wm->screen_width;
    c->h=wm->screen_height;
    c->x=c->y=0;
    XRaiseWindow(wm->display, c->win);
}

void set_preview_layout(WM *wm)
{
    int i=0, rows, cols, w, h;
    /* 行、列数量尽量相近，以保证窗口比例基本不变 */
    for(cols=1; cols<=wm->n; cols++)
        if(cols*cols >= wm->n)
            break;
    rows=(cols-1)*cols>=wm->n ? cols-1 : cols;
    w=wm->screen_width/cols;
    h=(wm->screen_height-wm->status_bar.h)/rows;
    i=wm->n-1;
    for(CLIENT *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        c->x=(i%cols)*w;
        c->y=(i/cols)*h;
        /* 下邊和右邊的窗口佔用剩餘空間 */
        c->w=(i+1)%cols ? w : w+(wm->screen_width-w*cols);
        c->h=i<cols*(rows-1) ? h : h+(wm->screen_height-wm->status_bar.h-h*rows);
        i--;
    }
}

void set_stack_layout(WM *wm)
{
    for(CLIENT *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(c->w >= 2*RESIZE_INC)
            c->w-=RESIZE_INC;
        if(c->h >= 2*RESIZE_INC)
            c->h-=RESIZE_INC;
    }
}

void set_tile_layout(WM *wm)
{
    CLIENT *c, *mc=NULL;
    unsigned int i, j, mw, sw, fw, mh, sh, fh;

    mw=wm->main_area_ratio*wm->screen_width;
    fw=wm->screen_width*wm->fixed_area_ratio;
    sw=wm->screen_width-fw-mw;
    mh=wm->screen_height-wm->status_bar.h;
    fh=wm->n_fixed ? mh/wm->n_fixed : mh;
    sh=wm->n_normal>1 ? mh/(wm->n_normal-1) : mh;
    if(wm->n_fixed == 0) mw+=fw;

    for(i=0, j=0, c=wm->clients->next; c!=wm->clients; c=c->next)
    {
        if(c->place_type == fixed)
            c->x=mw+sw, c->y=i++*fh, c->w=fw, c->h=fh;
        else if(c->place_type == normal)
        {
            if(j)
                c->x=0, c->y=(j-1)*sh, c->w=sw, c->h=sh;
            else
                c->x=sw, c->y=0, c->w=mw, c->h=mh, mc=c;
            j++;
        }
    }
    if(j == 1 && mc)
        mc->x=0, mc->y=0, mc->w=mw+sw, mc->h=mh;
    raise_float_wins(wm);
}

void grab_keys(WM *wm)
{
    KeyCode code;
    XUngrabKey(wm->display, AnyKey, AnyModifier, wm->root_win);
    for(size_t i=0; i<ARRAY_NUM(keybinds_list); i++)
        if ((code=XKeysymToKeycode(wm->display, keybinds_list[i].keysym)))
            XGrabKey(wm->display, code, keybinds_list[i].modifier,
                wm->root_win, True, GrabModeAsync, GrabModeAsync);
}

void grab_buttons(WM *wm)
{
    Window win=wm->focus_client->win;
    XUngrabButton(wm->display, AnyButton, AnyModifier, win);
    for(size_t i=0; i<ARRAY_NUM(buttonbinds_list); i++)
        XGrabButton(wm->display, buttonbinds_list[i].button,
            buttonbinds_list[i].modifier, win, False,
            ButtonPressMask|ButtonReleaseMask,
            GrabModeAsync, GrabModeAsync, None, None);
}

void handle_events(WM *wm)
{
	XEvent e;
	while(1)
    {
        XNextEvent(wm->display, &e);
        switch(e.type)
        {
            case ButtonPress : handle_button_press(wm, &e); break;
            case ConfigureRequest : handle_config_request(wm, &e); break;
            case DestroyNotify : handle_destroy_notify(wm, &e); break;
            case Expose : handle_expose(wm, &e); break;
            case KeyPress : handle_key_press(wm, &e); break;
            case MapRequest : handle_map_request(wm, &e); break;
            case UnmapNotify : handle_unmap_notify(wm, &e); break;
            case PropertyNotify : handle_property_notify(wm , &e); break;
        }
    }
}

void handle_button_press(WM *wm, XEvent *e)
{
    BUTTONBINDS bb;
	for(size_t i=0; i<ARRAY_NUM(buttonbinds_list); i++)
    {
        bb=buttonbinds_list[i];
		if( bb.button == e->xbutton.button
            && (get_valid_mask(wm, bb.modifier)
                == get_valid_mask(wm, e->xbutton.state))
            && bb.func)
            bb.func(wm, e, bb.arg);
    }
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
    if(win_to_client(wm, e->xdestroywindow.window))
    {
        del_client(wm, e->xdestroywindow.window);
        update_layout(wm);
        XSetInputFocus(wm->display, wm->focus_client->win, RevertToParent, CurrentTime);
    }
}
 
void del_client(WM *wm, Window win)
{
    CLIENT *c=win_to_client(wm, win);
    if(c)
    {
        wm->n--;
        if(c->place_type == fixed)
            wm->n_fixed--;
        else if(c->place_type == floating)
            wm->n_float--;
        else
            wm->n_normal--;
        c->prev->next=c->next;
        c->next->prev=c->prev;
        if(c == wm->focus_client)
            wm->focus_client=wm->focus_client->next;
        free(c);
    }
}

void handle_expose(WM *wm, XEvent *eent)
{
    XSetForeground(wm->display, wm->gc, wm->black);
    STATUS_BAR *b=&wm->status_bar;
    XClearWindow(wm->display, b->win);
    draw_string_in_center(wm, b->win, wm->font_set, wm->gc, 0, 0, b->w, b->h, "gwm");
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
            && get_valid_mask(wm, kb.modifier) == get_valid_mask(wm, e->xkey.state)
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
        fprintf(stderr, "找不到指定的鍵符號相應的功能轉換鍵");
    }
    else
        fprintf(stderr, "指定的鍵符號不存在對應的鍵代碼");
    return 0;
}

void handle_map_request(WM *wm, XEvent *e)
{
    XMapWindow(wm->display, e->xmaprequest.window);
    XWindowAttributes attr;
    if( e->xmaprequest.window != wm->status_bar.win
        && XGetWindowAttributes(wm->display, e->xmaprequest.window, &attr)
        && get_state_hint(wm, e->xmaprequest.window) == NormalState
        && !attr.override_redirect
        && !win_to_client(wm, e->xmaprequest.window))
    {
        add_client(wm, e->xmaprequest.window);
        update_layout(wm);
        XSetInputFocus(wm->display, wm->focus_client->win, RevertToParent, CurrentTime);
    }
    XRaiseWindow(wm->display, wm->status_bar.win);
}

void handle_unmap_notify(WM *wm, XEvent *e)
{
    if(win_to_client(wm, e->xunmap.window))
    {
        del_client(wm, e->xunmap.window);
        update_layout(wm);
        XSetInputFocus(wm->display, wm->focus_client->win, RevertToParent, CurrentTime);
    }
}

void handle_property_notify(WM *wm, XEvent *e)
{
    STATUS_BAR *b=&wm->status_bar;
    if(e->xproperty.window==wm->root_win && e->xproperty.atom==XA_WM_NAME)
    {
        XTextProperty name;
        XGetTextProperty(wm->display, wm->root_win, &name, XA_WM_NAME);
        XSetForeground(wm->display, wm->gc, wm->black);
        XClearWindow(wm->display, b->win);
        draw_string_in_center(wm, b->win, wm->font_set, wm->gc, 0, 0, b->w, b->h, (char *)name.value);
    }
}

void draw_string_in_center(WM *wm, Drawable drawable, XFontSet font_set, GC gc, int x, int y, unsigned int w, unsigned h, const char *str)
{
    if(str && str[0]!='\0')
    {
        XRectangle ink, logical;
        XmbTextExtents(wm->font_set, str, strlen(str), &ink, &logical);
        XmbDrawString(wm->display, drawable, wm->font_set, wm->gc, x+w/2-logical.width/2, y+h/2+logical.height/2, str, strlen(str));
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

void key_move_win(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->layout==full || wm->layout==preview) return;

    CLIENT *c=wm->focus_client;
    DIRECTION d=arg.direction;

    prepare_for_move_resize(wm);
    if(d==up && c->y+c->h>MOVE_INC)
        XMoveWindow(wm->display, c->win, c->x, c->y-=MOVE_INC);
    else if(d==down && wm->screen_height-wm->status_bar.h-c->y>MOVE_INC)
        XMoveWindow(wm->display, c->win, c->x, c->y+=MOVE_INC);
    else if(d==left && c->x+c->w>MOVE_INC)
        XMoveWindow(wm->display, c->win, c->x-=MOVE_INC, c->y);
    else if(d==right && wm->screen_width-c->x>MOVE_INC)
        XMoveWindow(wm->display, c->win, c->x+=MOVE_INC, c->y);
}

void prepare_for_move_resize(WM *wm)
{
    CLIENT *c=wm->focus_client;
    if(c->place_type==normal && wm->layout!=stack) 
    {
        c->place_type=floating;
        wm->n_normal--;
        wm->n_float++;
        update_layout(wm);
    }
}

void key_resize_win(WM *wm, XEvent *e, FUNC_ARG arg)
{
    if(wm->layout==full || wm->layout==preview) return;

    Display *disp=wm->display;
    CLIENT *c=wm->focus_client;
    DIRECTION d=arg.direction;
    unsigned int s=RESIZE_INC;

    prepare_for_move_resize(wm);
    if(d==up2up && c->y>s)
        XMoveResizeWindow(disp, c->win, c->x, c->y-=s, c->w, c->h+=s);
    else if(d==up2down && c->h>s)
        XMoveResizeWindow(disp, c->win, c->x, c->y+=s, c->w, c->h-=s);
    else if(d==down2up && c->h>s)
        XResizeWindow(disp, c->win, c->w, c->h-=s);
    else if(d==down2down && wm->screen_height-wm->status_bar.h-c->y-c->h>s)
        XResizeWindow(disp, c->win, c->w, c->h+=s);
    else if(d==left2left && c->x>s)
        XMoveResizeWindow(disp, c->win, c->x-=s, c->y, c->w+=s, c->h);
    else if(d==left2right && c->w>s)
        XMoveResizeWindow(disp, c->win, c->x+=s, c->y, c->w-=s, c->h);
    else if(d==right2left && c->w>s)
        XResizeWindow(disp, c->win, c->w-=s, c->h);
    else if(d==right2right && wm->screen_width-c->x-c->w>s)
        XResizeWindow(disp, c->win, c->w+=s, c->h);
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
        if(!send_event(wm, XInternAtom(wm->display, "WM_DELETE_WINDOW", False)))
        {
            XGrabServer(wm->display);
            XKillClient(wm->display, wm->focus_client->win);
            XUngrabServer(wm->display);
        }
        del_client(wm, wm->focus_client->win);
        update_layout(wm);
        XSetInputFocus(wm->display, wm->focus_client->win, RevertToParent, CurrentTime);
    }
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
    {
        CLIENT *c=wm->focus_client;
        c=(c->next==wm->clients) ? wm->clients->next : c->next;
        focus_client(wm, c);
    }
}

void focus_client(WM *wm, CLIENT *c)
{
    wm->focus_client=c;
    if(wm->focus_client != wm->clients)
        XRaiseWindow(wm->display, wm->focus_client->win);
    XRaiseWindow(wm->display, wm->status_bar.win);
    XSetInputFocus(wm->display, c->win, RevertToParent, CurrentTime);
}

/* 僅提升被遮擋的懸浮窗口似乎是一個好主意，但實際上計算遮擋關系相當
 * 復雜，而且一般情況下懸浮窗口數量較少，還不如將所有懸浮窗口提升 */
void raise_float_wins(WM *wm)
{
    for(CLIENT *c=wm->clients->next; c!=wm->clients; c=c->next)
        if(c->place_type == floating)
            XRaiseWindow(wm->display, c->win);
    if(wm->focus_client != wm->clients)
        XRaiseWindow(wm->display, wm->focus_client->win);
    XRaiseWindow(wm->display, wm->status_bar.win);
}

void toggle_float(WM *wm, XEvent *e, FUNC_ARG unused)
{
    CLIENT *c=wm->focus_client;
    c->place_type=(c->place_type==floating) ? normal : floating;
    if(c->place_type == floating)
    {
        wm->n_normal--;
        wm->n_float++;
        set_default_rect(wm, c);
        XMoveResizeWindow(wm->display, c->win, c->x, c->y, c->w, c->h);
    }
    else
    {
        wm->n_normal++;
        wm->n_float--;
    }
    update_layout(wm);
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
    int old_x, old_y, new_x, new_y, x_sign, y_sign, w_sign, h_sign;
    bool first=true;

    if(!grab_pointer_for_move_resize(wm)) return;
    if(!query_pointer_for_move_resize(wm, &old_x, &old_y, &focus_win)) return;
    c=win_to_client(wm, focus_win);
    c=c?c:wm->clients;
    focus_client(wm, c);
    if(wm->layout==full || wm->layout==preview) return;
    get_rect_sign(wm, old_x, old_y, arg.resize_flag, 
        &x_sign, &y_sign, &w_sign, &h_sign);

    do
    {
        XMaskEvent(wm->display, POINTER_MASK|SubstructureRedirectMask, &ev);
        switch(ev.type)
        {
            case ConfigureRequest : handle_config_request(wm, &ev); break;
            case MotionNotify:
                if(c == wm->clients)
                {
                    fprintf(stderr, "錯誤：不能移動根窗口或改變根窗口的尺寸！\n");
                    XUngrabPointer(wm->display, CurrentTime);
                    return ;
                }
                if(first)
                {
                    prepare_for_move_resize(wm);
                    first=false;
                }

                new_x=ev.xmotion.x, new_y=ev.xmotion.y;
                c->x+=x_sign*(new_x-old_x), c->y+=y_sign*(new_y-old_y);
                c->w+=w_sign*(new_x-old_x), c->h+=h_sign*(new_y-old_y);
                XMoveResizeWindow(wm->display, c->win, c->x, c->y, c->w, c->h);
                old_x=new_x, old_y=new_y;
        }
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
        fprintf(stderr, "錯誤：定位指針不在當前屏幕！");
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

    if(!XGetClassHint(wm->display, c->win, &ch))
        return ;
    for(size_t i=0; i<ARRAY_NUM(rules); i++)
    {
        r=rules+i;
        if( (!r->app_class || strstr(ch.res_class, r->app_class))
            && (!r->app_name || strstr(ch.res_name, r->app_name)))
            c->place_type=r->place_type;
    }
    wm->n++;
    if(c->place_type == fixed)
        wm->n_fixed++;
    else if(c->place_type == floating)
    {
        wm->n_float++;
        set_default_rect(wm, c);
    }
    else
        wm->n_normal++;

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
