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

static void alloc_widget_color(const char *color_name, XColor *color);
static void alloc_text_color(const char *color_name, XftColor *color);
static unsigned int get_num_lock_mask(void);
static unsigned int get_valid_mask(unsigned int mask);
static unsigned int get_modifier_mask(KeySym key_sym);
static XColor widget_color[COLOR_THEME_N][WIDGET_COLOR_N]; // 構件顏色
static XftColor text_color[COLOR_THEME_N][TEXT_COLOR_N]; // 文本顏色
static Cursor cursors[POINTER_ACT_N]; // 光標

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
        for(Widget_color j=0; j<WIDGET_COLOR_N; j++)
            alloc_widget_color(cfg->widget_color_name[i][j], &widget_color[i][j]);
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Text_color j=0; j<TEXT_COLOR_N; j++)
            alloc_text_color(cfg->text_color_name[i][j], &text_color[i][j]);
}

unsigned long get_widget_color(Widget_color wc)
{
    float wo=cfg->widget_opacity[cfg->color_theme][wc];
    unsigned long rgb=widget_color[cfg->color_theme][wc].pixel;

    return ((rgb & 0x00ffffff) | ((unsigned long)(0xff*wo))<<24);
}

XftColor get_text_color(Text_color color_id)
{
    return text_color[cfg->color_theme][color_id];
}

Window create_widget_win(Widget_type type, Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel)
{
    XSetWindowAttributes attr;
    attr.colormap=xinfo.colormap;
    attr.border_pixel=border_pixel;
    attr.background_pixel=bg_pixel;
    attr.override_redirect=True;

    Window win=XCreateWindow(xinfo.display, parent, x, y, w, h, border_w, xinfo.depth,
        InputOutput, xinfo.visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

    set_gwm_widget_type(win, type);

    return win;
}

Widget_type get_widget_type(Window win)
{
    CARD32 type;
    return get_gwm_widget_type(win, &type) ? type : NON_WIDGET;
}

void update_hint_win_for_info(Window hover, const char *info)
{
    int x, y, rx, ry, pad=get_font_pad(),
        w=0, h=get_font_height_by_pad();

    get_string_size(info, &w, NULL);
    w+=pad*2;
    if(hover)
    {
        Window r, c;
        unsigned int m;
        if(!XQueryPointer(xinfo.display, hover, &r, &c, &rx, &ry, &x, &y, &m))
            return;
        set_pos_for_click(hover, rx, &x, &y, w, h);
    }
    else
        x=(xinfo.screen_width-w)/2, y=(xinfo.screen_height-h)/2;
    XMoveResizeWindow(xinfo.display, xinfo.hint_win, x, y, w, h);
    XMapRaised(xinfo.display, xinfo.hint_win);
    Str_fmt f={0, 0, w, h, CENTER, true, false, 0,
        get_text_color(HINT_TEXT_COLOR)};
    draw_string(xinfo.hint_win, info, &f);
}

void draw_icon(Drawable d, Imlib_Image image, const char *name, int size)
{
    if(image)
        draw_image(image, d, 0, 0, size, size);
    else
    {
        char s[2]={name[0], '\0'};
        XftColor color=get_text_color(CLASS_TEXT_COLOR);
        Str_fmt fmt={0, 0, size, size, CENTER, false, false, 0, color};
        draw_string(d, s, &fmt);
    }
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
    xinfo.hint_win=create_widget_win(HINT_WIN, xinfo.root_win, 0, 0, 1, 1, 0, 0,
        get_widget_color(HINT_WIN_COLOR));
    XSelectInput(xinfo.display, xinfo.hint_win, ExposureMask);
}

void create_client_menu(void)
{
    Widget_type types[CLIENT_MENU_ITEM_N];
    for(int i=0; i<CLIENT_MENU_ITEM_N; i++)
        types[i]=CLIENT_MENU_ITEM_BEGIN+i;
    client_menu=create_menu(CLIENT_MENU, types, cfg->client_menu_item_text,
        CLIENT_MENU_ITEM_N, 1);
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
        if(b->widget_type == CLIENT_WIN)
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
