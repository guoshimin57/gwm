/* *************************************************************************
 *     frame.c：實現客戶窗口框架。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

#define FRAME_EVENT_MASK (SubstructureRedirectMask|SubstructureNotifyMask| \
    ExposureMask|ButtonPressMask|CROSSING_MASK|FocusChangeMask)
#define TITLEBAR_EVENT_MASK (ButtonPressMask|ExposureMask|CROSSING_MASK)

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

static Titlebar *create_titlebar(Widget *parent, Widget_state state, int x, int y, int w, int h, const char *title, Imlib_Image image);
static void set_titlebar_method(Widget *widget);
static void destroy_titlebar(Titlebar *titlebar);
static Rect get_titlebar_button_rect(const Titlebar *titlebar, size_t index);
static Rect get_title_rect(const Titlebar *titlebar);
static int get_cur_titlebar_button_n(void);

Frame *create_frame(Widget *parent, Widget_state state, int x, int y, int w, int h, int titlebar_h, int border_w, const char *title, Imlib_Image image)
{
    Frame *frame=malloc_s(sizeof(Frame));
    int fx=x-border_w, fy=y-titlebar_h-border_w, fw=w, fh=h+titlebar_h;

    init_widget(WIDGET(frame), NULL, CLIENT_FRAME, state, fx, fy, fw, fh);
    set_widget_border_width(WIDGET(frame), border_w);
    set_widget_border_color(WIDGET(frame), get_widget_color(state));
    frame->cwin=WIDGET_WIN(parent);
    frame->titlebar=NULL;
    XSelectInput(xinfo.display, WIDGET_WIN(frame), FRAME_EVENT_MASK);
    if(cfg->set_frame_prop)
        copy_prop(WIDGET_WIN(frame), frame->cwin);
    if(titlebar_h)
        frame->titlebar=create_titlebar(WIDGET(frame), state, 0, 0, w, titlebar_h, title, image);
    XAddToSaveSet(xinfo.display, frame->cwin);
    XReparentWindow(xinfo.display, frame->cwin, WIDGET_WIN(frame), 0, titlebar_h);
    set_client_leader(WIDGET_WIN(frame), frame->cwin);
    return frame;
    
    /* 以下是同時設置窗口前景和背景透明度的非EWMH標準方法：
    unsigned long opacity = (unsigned long)(0xfffffffful);
    Atom XA_NET_WM_WINDOW_OPACITY = XInternAtom(xinfo.display, "_NET_WM_WINDOW_OPACITY", False);
    XChangeProperty(xinfo.display, c->frame, XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
        PropModeReplace, (unsigned char *)&opacity, 1L);
    */
}

static Titlebar *create_titlebar(Widget *parent, Widget_state state, int x, int y, int w, int h, const char *title, Imlib_Image image)
{
    Titlebar *titlebar=malloc_s(sizeof(Titlebar));

    init_widget(WIDGET(titlebar), parent, TITLEBAR, state, x, y, w, h);
    set_titlebar_method(WIDGET(titlebar));
    titlebar->title=copy_string(title);
    create_tooltip(WIDGET(titlebar), title);

    titlebar->logo=create_button(WIDGET(titlebar), TITLE_LOGO, state, 0, 0, h, h, NULL);
    create_tooltip(WIDGET(titlebar->logo), cfg->tooltip[TITLE_LOGO]);
    set_button_icon(BUTTON(titlebar->logo), image, NULL, "∨");

    for(size_t i=0; i<TITLE_BUTTON_N; i++)
    {
        Rect br=get_titlebar_button_rect(titlebar, i);
        Widget_id id=TITLE_BUTTON_BEGIN+i;
        titlebar->buttons[i]=create_button(WIDGET(titlebar), id, state,
            br.x, br.y, br.w, br.h, cfg->title_button_text[i]);
        create_tooltip(WIDGET(titlebar->buttons[i]), cfg->tooltip[id]);
    }

    titlebar->menu=create_menu(WIDGET(titlebar->logo), CLIENT_MENU,
        cfg->client_menu_item_icon, cfg->client_menu_item_symbol,
        cfg->client_menu_item_label, CLIENT_MENU_ITEM_N, 1);

    XSelectInput(xinfo.display, WIDGET_WIN(titlebar), TITLEBAR_EVENT_MASK);
    show_widget(WIDGET(titlebar));

    return titlebar;
}

static void set_titlebar_method(Widget *widget)
{
    widget->update_fg=update_titlebar_fg;
}

static Rect get_titlebar_button_rect(const Titlebar *titlebar, size_t index)
{
    int tw=WIDGET_W(titlebar), w=cfg->title_button_width, h=get_font_height_by_pad();
    return (Rect){tw-w*(TITLE_BUTTON_N-index), 0, w, h};
}

void destroy_frame(Frame *frame)
{
    int bw=frame->base.border_w, th=(frame->titlebar ? WIDGET_H(frame->titlebar) : 0);

    XReparentWindow(xinfo.display, frame->cwin, xinfo.root_win, 
        WIDGET_X(frame)+bw, WIDGET_Y(frame)+th+bw);

    if(frame->titlebar)
        destroy_titlebar(frame->titlebar);
    destroy_widget(WIDGET(frame));
}

static void destroy_titlebar(Titlebar *titlebar)
{
    vfree(titlebar->title);
    destroy_button(titlebar->logo);
    for(size_t i=0; i<TITLE_BUTTON_N; i++)
        destroy_button(titlebar->buttons[i]);
    destroy_menu(titlebar->menu);
    destroy_widget(WIDGET(titlebar));
}

void move_resize_frame(Frame *frame, int x, int y, int w, int h)
{
    Titlebar *p=frame->titlebar;
    if(p)
    {
        move_resize_widget(WIDGET(p), WIDGET_X(p), WIDGET_Y(p), w, WIDGET_H(p));
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
        {
            Rect br=get_titlebar_button_rect(p, i);
            move_resize_widget(WIDGET(p->buttons[i]), br.x, br.y, br.w, br.h);
        }
    }
    move_resize_widget(WIDGET(frame), x, y, w, h);
}

Rect get_frame_rect_by_win(const Frame *frame, int x, int y, int w, int h)
{
    int bw=frame->base.border_w, bh=(frame->titlebar ? WIDGET_H(frame->titlebar) : 0);
    return (Rect){x-bw, y-bh-bw, w, h+bh};
}

Rect get_win_rect_by_frame(const Frame *frame)
{
    int bw=frame->base.border_w, bh=(frame->titlebar ? WIDGET_H(frame->titlebar) : 0);
    return (Rect){WIDGET_X(frame)+bw, WIDGET_Y(frame)+bh+bw, WIDGET_W(frame), WIDGET_H(frame)-bh};
}

bool is_frame_part(const Frame *frame, Window win)
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

void set_frame_state_current(Frame *frame, int value)
{
    WIDGET_STATE(frame).current=value;
    if(frame->titlebar)
    {
        WIDGET_STATE(frame->titlebar).current=value;
        WIDGET_STATE(frame->titlebar->logo).current=value;
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            WIDGET_STATE(frame->titlebar->buttons[i]).current=value;
    }
}

void update_frame_bg(const Frame *frame)
{
    if(frame->base.border_w)
        set_widget_border_color(WIDGET(frame),
            get_widget_color(WIDGET_STATE(frame)));
    if(frame->titlebar)
    {
        update_widget_bg(WIDGET(frame->titlebar));
        update_widget_bg(WIDGET(frame->titlebar->logo));
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            update_widget_bg(WIDGET(frame->titlebar->buttons[i]));
    }
}

Menu *get_frame_menu(const Frame *frame)
{
    return (frame && frame->titlebar) ? frame->titlebar->menu : NULL;
}

Titlebar *get_frame_titlebar(const Frame *frame)
{
    return frame->titlebar;
}

void toggle_titlebar(Frame *frame, const char *title, Imlib_Image image)
{
    if(frame->titlebar)
        destroy_titlebar(frame->titlebar);
    else
        frame->titlebar=create_titlebar(WIDGET(frame), frame->base.state,
            0, 0, frame->base.w, get_font_height_by_pad(), title, image);
}

void update_titlebar_fg(const Widget *widget)
{
    Titlebar *titlebar=(Titlebar *)widget;
    Rect r=get_title_rect(titlebar);
    Str_fmt f={r.x, r.y, r.w, r.h, CENTER, true, false, 0,
        get_widget_fg(WIDGET_STATE(titlebar))};
    draw_string(WIDGET_WIN(titlebar), titlebar->title, &f);
}

static Rect get_title_rect(const Titlebar *titlebar)
{
    int n=get_cur_titlebar_button_n(), bw=cfg->title_button_width, w=bw*n;
    return (Rect){bw, 0, titlebar->base.w-bw-w, titlebar->base.h};
}

void update_titlebar_layout(const Frame *frame)
{
    int n=get_cur_titlebar_button_n();
    for(int i=0; i<TITLE_BUTTON_N; i++)
    {
        if(i<TITLE_BUTTON_N-n)
            hide_widget(WIDGET(frame->titlebar->buttons[i]));
        else
            show_widget(WIDGET(frame->titlebar->buttons[i]));
    }
    update_titlebar_fg(WIDGET(frame->titlebar));
}

static int get_cur_titlebar_button_n(void)
{
    int layout=cfg->default_layout;
    get_gwm_current_layout(&layout);
    int buttons_n[]={[PREVIEW]=1, [STACK]=3, [TILE]=7};
    return buttons_n[layout];
}

void change_title(const Frame *frame, const char *title)
{
    vfree(frame->titlebar->title);
    frame->titlebar->title=copy_string(title);
    change_tooltip_tip(TOOLTIP(WIDGET_TOOLTIP(frame->titlebar)), title);
    update_titlebar_fg(WIDGET(frame->titlebar));
}

void change_frame_logo(const Frame *frame, Imlib_Image image)
{
    change_button_icon(frame->titlebar->logo, image, NULL, NULL);
    update_button_fg(WIDGET(frame->titlebar->logo));
}
