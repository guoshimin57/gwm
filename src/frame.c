/* *************************************************************************
 *     frame.c：實現客戶窗口框架。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "button.h"
#include "config.h"
#include "prop.h"
#include "icccm.h"
#include "tooltip.h"
#include "frame.h"

#define FRAME_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    ExposureMask|ButtonPressMask|CROSSING_MASK|FocusChangeMask)
#define TITLEBAR_EVENT_MASK (BUTTON_MASK|ExposureMask|CROSSING_MASK)

struct _titlebar_tag // 標題欄
{
    Widget base;
    char *title;
    Button *logo;
    Button *buttons[TITLE_BUTTON_N]; //標題區按鈕
    Menu *menu;
};

struct _frame_tag // 客戶窗口裝飾
{
    Widget base;
    Window cwin;
    Titlebar *titlebar;
};

static void frame_ctor(Frame *frame, Widget *parent, int x, int y, int w, int h, int titlebar_h, int border_w, const char *title, Imlib_Image image);
static Titlebar *titlebar_new(Widget *parent, int x, int y, int w, int h, const char *title, Imlib_Image image);
static void titlebar_ctor(Titlebar *titlebar, Widget *parent, int x, int y, int w, int h, const char *title, Imlib_Image image);
static void titlebar_set_method(Widget *widget);
static void frame_dtor(Frame *frame);
static void titlebar_del(Titlebar *titlebar);
static void titlebar_dtor(Titlebar *titlebar);
static void titlebar_buttons_show(Titlebar *titlebar);
static int titlebar_get_button_n(void);
static Rect titlebar_get_button_rect(const Titlebar *titlebar, size_t index);
static Rect titlebar_get_title_rect(const Titlebar *titlebar);

Frame *frame_new(Widget *parent, int x, int y, int w, int h, int titlebar_h, int border_w, const char *title, Imlib_Image image)
{
    Frame *frame=Malloc(sizeof(Frame));
    frame_ctor(frame, parent, x, y, w, h, titlebar_h, border_w, title, image);
    
    /* 以下是同時設置窗口前景和背景透明度的非EWMH標準方法：
    unsigned long opacity = (unsigned long)(0xfffffffful);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(xinfo.display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(xinfo.display, c->frame, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&opacity, 1L);
    */

    XAddToSaveSet(xinfo.display, frame->cwin);
    XReparentWindow(xinfo.display, frame->cwin, WIDGET_WIN(frame), 0, titlebar_h);
    set_client_leader(WIDGET_WIN(frame), frame->cwin);
    XSelectInput(xinfo.display, WIDGET_WIN(frame), FRAME_EVENT_MASK);

    return frame;
}

static void frame_ctor(Frame *frame, Widget *parent, int x, int y, int w, int h, int titlebar_h, int border_w, const char *title, Imlib_Image image)
{
    widget_ctor(WIDGET(frame), NULL, WIDGET_TYPE_FRAME, CLIENT_FRAME, x, y, w, h);
    widget_set_border_width(WIDGET(frame), border_w);
    widget_set_border_color(WIDGET(frame), get_widget_color(WIDGET(frame)));
    frame->cwin=WIDGET_WIN(parent);
    frame->titlebar=NULL;
    if(cfg->set_frame_prop)
        copy_prop(WIDGET_WIN(frame), frame->cwin);
    if(titlebar_h)
        frame->titlebar=titlebar_new(WIDGET(frame), 0, 0, w, titlebar_h, title, image);
}

static Titlebar *titlebar_new(Widget *parent, int x, int y, int w, int h, const char *title, Imlib_Image image)
{
    Titlebar *titlebar=Malloc(sizeof(Titlebar));

    titlebar_ctor(titlebar, parent, x, y, w, h, title, image);
    titlebar_show(WIDGET(titlebar));

    return titlebar;
}

static void titlebar_ctor(Titlebar *titlebar, Widget *parent, int x, int y, int w, int h, const char *title, Imlib_Image image)
{
    widget_ctor(WIDGET(titlebar), parent, WIDGET_TYPE_TITLEBAR, TITLEBAR, x, y, w, h);
    titlebar_set_method(WIDGET(titlebar));
    titlebar->title=copy_string(title);
    WIDGET_TOOLTIP(titlebar)=(Widget *)tooltip_new(WIDGET(titlebar), title);

    titlebar->logo=button_new(WIDGET(titlebar), TITLE_LOGO, 0, 0, h, h, NULL);
    WIDGET_TOOLTIP(titlebar->logo)=(Widget *)tooltip_new(WIDGET(titlebar->logo), cfg->tooltip[TITLE_LOGO]);
    button_set_icon(BUTTON(titlebar->logo), image, NULL, "∨");

    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=titlebar_get_button_rect(titlebar, i);
        Widget_id id=TITLE_BUTTON_BEGIN+i;
        titlebar->buttons[i]=button_new(WIDGET(titlebar), id,
            br.x, br.y, br.w, br.h, cfg->title_button_text[i]);
        WIDGET_TOOLTIP(titlebar->buttons[i])=(Widget *)tooltip_new(WIDGET(titlebar->buttons[i]), cfg->tooltip[id]);
    }

    titlebar->menu=menu_new(WIDGET(titlebar->logo), CLIENT_MENU,
        cfg->client_menu_item_icon, cfg->client_menu_item_symbol,
        cfg->client_menu_item_label, CLIENT_MENU_ITEM_N, 1);
    widget_set_poppable(WIDGET(titlebar->menu), true);

    XSelectInput(xinfo.display, WIDGET_WIN(titlebar), TITLEBAR_EVENT_MASK);
}

static void titlebar_set_method(Widget *widget)
{
    widget->show=titlebar_show;
    widget->update_fg=titlebar_update_fg;
}

static Rect titlebar_get_button_rect(const Titlebar *titlebar, size_t index)
{
    int tw=WIDGET_W(titlebar), w=cfg->title_button_width, h=get_font_height_by_pad();
    return (Rect){tw-w*(TITLE_BUTTON_N-index), 0, w, h};
}

void frame_del(Frame *frame)
{
    int bw=frame->base.border_w, th=(frame->titlebar ? WIDGET_H(frame->titlebar) : 0);

    XReparentWindow(xinfo.display, frame->cwin, xinfo.root_win, 
        WIDGET_X(frame)+bw, WIDGET_Y(frame)+th+bw);
    frame_dtor(frame);
    widget_del(WIDGET(frame));
}

static void frame_dtor(Frame *frame)
{
    if(frame->titlebar)
        titlebar_del(frame->titlebar), frame->titlebar=NULL;
}

static void titlebar_del(Titlebar *titlebar)
{
    titlebar_dtor(titlebar);
    widget_del(WIDGET(titlebar));
}

static void titlebar_dtor(Titlebar *titlebar)
{
    Free(titlebar->title);
    button_del(WIDGET(titlebar->logo)), titlebar->logo=NULL;
    for(size_t i=0; i<TITLE_BUTTON_N; i++)
        button_del(WIDGET(titlebar->buttons[i])), titlebar->buttons[i]=NULL;
    menu_del(titlebar->menu), titlebar->menu=NULL;
}

void frame_move_resize(Frame *frame, int x, int y, int w, int h)
{
    Titlebar *p=frame->titlebar;
    if(p)
    {
        widget_move_resize(WIDGET(p), WIDGET_X(p), WIDGET_Y(p), w, WIDGET_H(p));
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
        {
            Rect br=titlebar_get_button_rect(p, i);
            widget_move_resize(WIDGET(p->buttons[i]), br.x, br.y, br.w, br.h);
        }
    }
    widget_move_resize(WIDGET(frame), x, y, w, h);
}

bool frame_has_win(const Frame *frame, Window win)
{
    if(win == WIDGET_WIN(frame))
        return true;
    if(frame->titlebar)
    {
        if(win == WIDGET_WIN(frame->titlebar))
            return true;
        if(win == WIDGET_WIN(frame->titlebar->logo))
            return true;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            if(win == WIDGET_WIN(frame->titlebar->buttons[i]))
                return true;
    }
    return false;
}

void frame_set_state_unfocused(Frame *frame, int value)
{
    WIDGET_STATE(frame).unfocused=value;
    if(frame->titlebar)
    {
        WIDGET_STATE(frame->titlebar).unfocused=value;
        WIDGET_STATE(frame->titlebar->logo).unfocused=value;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            WIDGET_STATE(frame->titlebar->buttons[i]).unfocused=value;
    }
}

void frame_update_bg(const Frame *frame)
{
    if(frame->base.border_w)
        widget_set_border_color(WIDGET(frame),
            get_widget_color(WIDGET(frame)));
    if(frame->titlebar)
    {
        widget_update_bg(WIDGET(frame->titlebar));
        widget_update_bg(WIDGET(frame->titlebar->logo));
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            widget_update_bg(WIDGET(frame->titlebar->buttons[i]));
    }
}

Menu *frame_get_menu(const Frame *frame)
{
    return (frame && frame->titlebar) ? frame->titlebar->menu : NULL;
}

int frame_get_titlebar_height(const Frame *frame)
{
    return frame->titlebar ? WIDGET_H(frame->titlebar) : 0;
}

void titlebar_toggle(Frame *frame, const char *title, Imlib_Image image)
{
    if(frame->titlebar)
        titlebar_del(frame->titlebar), frame->titlebar=NULL;
    else
    {
        frame->titlebar=titlebar_new(WIDGET(frame),
            0, 0, frame->base.w, get_font_height_by_pad(), title, image);
        widget_set_state(WIDGET(frame->titlebar), WIDGET_STATE(frame));
    }
}

void titlebar_show(Widget *widget)
{
    widget_show(widget);
    titlebar_buttons_show((Titlebar *)widget);
}

static void titlebar_buttons_show(Titlebar *titlebar)
{
    int n=titlebar_get_button_n();
    for(int i=0; i<TITLE_BUTTON_N; i++)
    {
        if(i<TITLE_BUTTON_N-n)
            widget_hide(WIDGET(titlebar->buttons[i]));
        else
            widget_show(WIDGET(titlebar->buttons[i]));
    }
}

static int titlebar_get_button_n(void)
{
    int buttons_n[]={[PREVIEW]=1, [STACK]=3, [TILE]=7};
    return buttons_n[get_gwm_current_layout()];
}

void titlebar_update_fg(const Widget *widget)
{
    Titlebar *titlebar=(Titlebar *)widget;
    Rect r=titlebar_get_title_rect(titlebar);
    Str_fmt f={r.x, r.y, r.w, r.h, CENTER, true, false, 0,
        get_text_color(widget)};
    draw_string(WIDGET_WIN(titlebar), titlebar->title, &f);
}

static Rect titlebar_get_title_rect(const Titlebar *titlebar)
{
    int n=titlebar_get_button_n(), bw=cfg->title_button_width, w=bw*n;
    return (Rect){bw, 0, titlebar->base.w-bw-w, titlebar->base.h};
}

void titlebar_update_layout(const Frame *frame)
{
    titlebar_buttons_show(frame->titlebar);
    titlebar_update_fg(WIDGET(frame->titlebar));
}

void frame_change_title(const Frame *frame, const char *title)
{
    if(frame->titlebar == NULL)
        return;
    Free(frame->titlebar->title);
    frame->titlebar->title=copy_string(title);
    tooltip_change_tip(TOOLTIP(WIDGET_TOOLTIP(frame->titlebar)), title);
    titlebar_update_fg(WIDGET(frame->titlebar));
}

void frame_change_logo(const Frame *frame, Imlib_Image image)
{
    button_change_icon(frame->titlebar->logo, image, NULL, NULL);
    button_update_fg(WIDGET(frame->titlebar->logo));
}
