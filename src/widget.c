/* *************************************************************************
 *     widget.c：實現構件相關功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

typedef struct widget_node_tag
{
    Widget *widget;
    struct widget_node_tag *next;
} Widget_node;

static void reg_widget(Widget *widget);
static void unreg_widget(Widget *widget);
static Widget_color_id get_widget_color_id(const Widget *widget);
static void alloc_widget_color(const char *color_name, XColor *color);
static void alloc_text_color(const char *color_name, XftColor *color);
static unsigned int get_num_lock_mask(void);
static unsigned int get_valid_mask(unsigned int mask);
static unsigned int get_modifier_mask(KeySym key_sym);
static XColor widget_color[COLOR_THEME_N][WIDGET_COLOR_N]; // 構件顏色
static XftColor text_color[COLOR_THEME_N][TEXT_COLOR_N]; // 文本顏色
static Cursor cursors[POINTER_ACT_N]; // 光標

static Widget_node *widget_list=NULL;

static void reg_widget(Widget *widget)
{
    Widget_node *p=malloc_s(sizeof(Widget_node));
    p->widget=widget;
    p->next=widget_list;
    widget_list=p;
}

static void unreg_widget(Widget *widget)
{
    for(Widget_node *prev=NULL, *p=widget_list; p; prev=p, p=p->next)
    {
        if(p->widget == widget)
        {
            if(prev == NULL)
                widget_list=p->next;
            else
                prev->next=p->next;
            free(p);
            break;
        }
    }
}

Widget *win_to_widget(Window win)
{
    for(Widget_node *p=widget_list; p; p=p->next)
        if(p->widget->win == win)
            return p->widget;
    return NULL;
}

Widget *create_widget(Widget_id id, Widget_type type, Widget_state state, Window parent, int x, int y, int w, int h)
{
    Widget *widget=malloc_s(sizeof(Widget));

    init_widget(widget, id, type, state, parent, x, y, w, h);

    return widget;
}

void init_widget(Widget *widget, Widget_id id, Widget_type type, Widget_state state, Window parent, int x, int y, int w, int h)
{
    unsigned long bg;
    widget->id=id;
    widget->type=type;
    widget->state=state;
    widget->parent=parent;
    bg=get_widget_color(get_widget_color_id(widget));
    if(widget->id != CLIENT_WIN)
        widget->win=create_widget_win(parent, x, y, w, h, 0, 0, bg);
    widget->x=x, widget->y=y, widget->w=w, widget->h=h;
    widget->tooltip=NULL;

    XSelectInput(xinfo.display, widget->win, WIDGET_EVENT_MASK);
    reg_widget(widget);
}

void set_widget_tooltip(Widget *widget, const char *tooltip)
{
    widget->tooltip=copy_string(tooltip);
}

void set_widget_border_width(const Widget *widget, int width)
{
    XSetWindowBorderWidth(xinfo.display, widget->win, width);
}

void set_widget_border_color(const Widget *widget, unsigned long pixel)
{
    XSetWindowBorder(xinfo.display, widget->win, pixel);
}

void destroy_widget(Widget *widget)
{
    unreg_widget(widget);
    XDestroyWindow(xinfo.display, widget->win);
    vfree(widget->tooltip, widget, NULL);
}

void show_widget(const Widget *widget)
{
    XMapWindow(xinfo.display, widget->win);
    XMapSubwindows(xinfo.display, widget->win);
}

void hide_widget(const Widget *widget)
{
    XUnmapWindow(xinfo.display, widget->win);
}

void move_resize_widget(Widget *widget, int x, int y, int w, int h)
{
    widget->x=x, widget->y=y, widget->w=w, widget->h=h;;
    XMoveResizeWindow(xinfo.display, widget->win, x, y, w, h);
}

void update_widget_bg(const Widget *widget)
{
    update_win_bg(widget->win, get_widget_color(get_widget_color_id(widget)), None);
}

static Widget_color_id get_widget_color_id(const Widget *widget)
{
    if(widget->state.disable) return DISABLE_WIDGET_COLOR;
    if(widget->state.warn)    return WARN_WIDGET_COLOR;
    if(widget->state.active)  return ACTIVE_WIDGET_COLOR;
    if(widget->state.hot)     return HOT_WIDGET_COLOR;
    if(widget->state.urgent)  return URGENT_WIDGET_COLOR;
    if(widget->state.attent)  return ATTENT_WIDGET_COLOR;
    if(widget->state.chosen)  return CHOSEN_WIDGET_COLOR;
    if(widget->state.focus)   return FOCUS_WIDGET_COLOR; 
    return NORMAL_WIDGET_COLOR;
}

Widget_color_id get_widget_border_color_id(const Widget *widget)
{
    return widget->state.focus ? CURRENT_BORDER_COLOR : NORMAL_BORDER_COLOR;
}

Text_color_id get_widget_fg_id(const Widget *widget)
{
    if(widget->state.disable) return DISABLE_WIDGET_TEXT_COLOR;
    if(widget->state.warn)    return WARN_WIDGET_TEXT_COLOR;
    if(widget->state.active)  return ACTIVE_WIDGET_TEXT_COLOR;
    if(widget->state.hot)     return HOT_WIDGET_TEXT_COLOR;
    if(widget->state.urgent)  return URGENT_WIDGET_TEXT_COLOR;
    if(widget->state.attent)  return ATTENT_WIDGET_TEXT_COLOR;
    if(widget->state.chosen)  return CHOSEN_WIDGET_TEXT_COLOR;
    if(widget->state.focus)   return FOCUS_WIDGET_TEXT_COLOR; 
    return NORMAL_WIDGET_TEXT_COLOR;
}

static void alloc_widget_color(const char *color_name, XColor *color)
{
    XParseColor(xinfo.display, xinfo.colormap, color_name, color); 
    XAllocColor(xinfo.display, xinfo.colormap, color);
}

static void alloc_text_color(const char *color_name, XftColor *color)
{
    XftColorAllocName(xinfo.display, xinfo.visual, xinfo.colormap, color_name, color);
}

void alloc_color(void)
{
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Widget_color_id j=0; j<WIDGET_COLOR_N; j++)
            alloc_widget_color(cfg->widget_color_name[i][j], &widget_color[i][j]);
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Text_color_id j=0; j<TEXT_COLOR_N; j++)
            alloc_text_color(cfg->text_color_name[i][j], &text_color[i][j]);
}

unsigned long get_widget_color(Widget_color_id id)
{
    float wo=cfg->widget_opacity[cfg->color_theme][id];
    unsigned long rgb=widget_color[cfg->color_theme][id].pixel;

    return ((rgb & 0x00ffffff) | ((unsigned long)(0xff*wo))<<24);
}

XftColor get_widget_fg(Text_color_id id)
{
    return text_color[cfg->color_theme][id];
}

Window create_widget_win(Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel)
{
    XSetWindowAttributes attr;
    attr.colormap=xinfo.colormap;
    attr.border_pixel=border_pixel;
    attr.background_pixel=bg_pixel;
    attr.override_redirect=True;

    Window win=XCreateWindow(xinfo.display, parent, x, y, w, h, border_w, xinfo.depth,
        InputOutput, xinfo.visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

    return win;
}

void update_hint_win_for_info(const Widget *widget, const char *info)
{
    int x, y, rx, ry, pad=get_font_pad(),
        w=0, h=get_font_height_by_pad();

    get_string_size(info, &w, NULL);
    w+=pad*2;
    if(widget)
    {
        Window r, c;
        unsigned int m;
        if(!XQueryPointer(xinfo.display, widget->win, &r, &c, &rx, &ry, &x, &y, &m))
            return;
        set_pos_for_click(widget->win, rx, &x, &y, w, h);
    }
    else
        x=(xinfo.screen_width-w)/2, y=(xinfo.screen_height-h)/2;
    XMoveResizeWindow(xinfo.display, xinfo.hint_win, x, y, w, h);
    XMapRaised(xinfo.display, xinfo.hint_win);
    Str_fmt f={0, 0, w, h, CENTER, true, false, 0, get_widget_fg(HINT_TEXT_COLOR)};
    draw_string(xinfo.hint_win, info, &f);
}

void set_xic(Window win, XIC *ic)
{
    if(xinfo.xim == NULL)
        return;
    if((*ic=XCreateIC(xinfo.xim, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
        XNClientWindow, win, NULL)) == NULL)
        fprintf(stderr, _("錯誤：窗口（0x%lx）輸入法設置失敗！"), win);
    else
        XSetICFocus(*ic);
}

KeySym look_up_key(XIC xic, XKeyEvent *e, wchar_t *keyname, size_t n)
{
	KeySym ks;
    if(xic)
        XwcLookupString(xic, e, keyname, n, &ks, 0);
    else
    {
        char kn[n];
        XLookupString(e, kn, n, &ks, 0);
        mbstowcs(keyname, kn, n);
    }
    return ks;
}

void create_hint_win(void)
{
    xinfo.hint_win=create_widget_win(xinfo.root_win, 0, 0, 1, 1, 0, 0,
        get_widget_color(HINT_WIN_COLOR));
    XSelectInput(xinfo.display, xinfo.hint_win, ExposureMask);
}

void create_client_menu(void)
{
    client_menu=create_menu(CLIENT_MENU, xinfo.root_win,
        cfg->client_menu_item_icon, cfg->client_menu_item_symbol,
        cfg->client_menu_item_label, CLIENT_MENU_ITEM_N, 1);
}

void create_cursors(void)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        cursors[i]=XCreateFontCursor(xinfo.display, cfg->cursor_shape[i]);
}

void set_cursor(Window win, Pointer_act act)
{
    XDefineCursor(xinfo.display, win, cursors[act]);
}

void free_cursors(void)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        XFreeCursor(xinfo.display, cursors[i]);
}

void grab_keys(void)
{
    unsigned int num_lock_mask=get_num_lock_mask();
    unsigned int masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    KeyCode code;
    XUngrabKey(xinfo.display, AnyKey, AnyModifier, xinfo.root_win);
    for(const Keybind *kb=cfg->keybind; kb->func; kb++)
        if((code=XKeysymToKeycode(xinfo.display, kb->keysym)))
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabKey(xinfo.display, code, kb->modifier|masks[j],
                    xinfo.root_win, True, GrabModeAsync, GrabModeAsync);
}

static unsigned int get_num_lock_mask(void)
{
	XModifierKeymap *m=XGetModifierMapping(xinfo.display);
    KeyCode code=XKeysymToKeycode(xinfo.display, XK_Num_Lock);

    if(code)
        for(int i=0; i<8; i++)
            for(int j=0; j<m->max_keypermod; j++)
                if(m->modifiermap[i*m->max_keypermod+j] == code)
                    { XFreeModifiermap(m); return (1<<i); }
    return 0;
}
    
void grab_buttons(Window win)
{
    unsigned int num_lock_mask=get_num_lock_mask(),
                 masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};

    XUngrabButton(xinfo.display, AnyButton, AnyModifier, win);
    for(const Buttonbind *b=cfg->buttonbind; b->func; b++)
    {
        if(b->widget_id == CLIENT_WIN)
        {
            int m=is_equal_modifier_mask(0, b->modifier) ?
                GrabModeSync : GrabModeAsync;
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabButton(xinfo.display, b->button, b->modifier|masks[j],
                    win, False, BUTTON_MASK, m, m, None, None);
        }
    }
}

bool is_equal_modifier_mask(unsigned int m1, unsigned int m2)
{
    return (get_valid_mask(m1) == get_valid_mask(m2));
}

static unsigned int get_valid_mask(unsigned int mask)
{
    return (mask & ~(LockMask|get_modifier_mask(XK_Num_Lock))
        & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask));
}

static unsigned int get_modifier_mask(KeySym key_sym)
{
    KeyCode kc;
    if((kc=XKeysymToKeycode(xinfo.display, key_sym)) != 0)
    {
        for(int i=0; i<8*xinfo.mod_map->max_keypermod; i++)
            if(xinfo.mod_map->modifiermap[i] == kc)
                return 1 << (i/xinfo.mod_map->max_keypermod);
        fprintf(stderr, _("錯誤：找不到指定的鍵符號相應的功能轉換鍵！\n"));
    }
    else
        fprintf(stderr, _("錯誤：指定的鍵符號不存在對應的鍵代碼！\n"));
    return 0;
}

bool grab_pointer(Window win, Pointer_act act)
{
    return XGrabPointer(xinfo.display, win, False, POINTER_MASK,
        GrabModeAsync, GrabModeAsync, None, cursors[act], CurrentTime)
        == GrabSuccess;
}
