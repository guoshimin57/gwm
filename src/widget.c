/* *************************************************************************
 *     widget.c：實現構件相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "gwm.h"
#include "font.h"
#include "misc.h"
#include "list.h"
#include "drawable.h"
#include "grab.h"
#include "widget.h"

#define WIDGET_STATE_NORMAL ((Widget_state){0})

typedef struct widget_node_tag
{
    List list;
    Widget *widget;
} Widget_node;

static void widget_reg(Widget *widget);
static Widget_node *widget_node_new(Widget *widget);
static void widget_unreg(Widget *widget);
static void widget_node_del(Widget_node *node);
static void widget_set_method(Widget *widget);
static void widget_dtor(Widget *widget);
static bool is_func_click(const Widget_id id, const Buttonbind *bind, XButtonEvent *be);
static Color_id state_to_color_id(Widget_state state);
static int get_pointer_x(void);

static Widget_node *widget_list=NULL;

static void widget_reg(Widget *widget)
{
    if(!widget_list)
    {
        widget_list=widget_node_new(NULL);
        LIST_INIT(widget_list);
    }
    Widget_node *p=widget_node_new(widget);
    LIST_ADD(p, widget_list);
}

static Widget_node *widget_node_new(Widget *widget)
{
    Widget_node *p=Malloc(sizeof(Widget_node));
    p->widget=widget;
    return p;
}

static void widget_unreg(Widget *widget)
{
    if(!widget_list)
        return;

    LIST_FOR_EACH_SAFE(Widget_node, p, widget_list)
        if(p->widget == widget)
            { widget_node_del(p); break; }
}

static void widget_node_del(Widget_node *node)
{
    LIST_DEL(node);
    free(node);
}

Widget *widget_find(Window win)
{
    LIST_FOR_EACH(Widget_node, p, widget_list)
        if(p->widget->win == win)
            return p->widget;
    return NULL;
}

void update_all_widget_bg(void)
{
    LIST_FOR_EACH(Widget_node, p, widget_list)
        p->widget->update_bg(p->widget);
}

Widget *widget_new(Widget *parent, Widget_type type, Widget_id id, int x, int y, int w, int h)
{
    Widget *widget=Malloc(sizeof(Widget));

    widget_ctor(widget, parent, type, id, x, y, w, h);

    return widget;
}

void widget_ctor(Widget *widget, Widget *parent, Widget_type type, Widget_id id, int x, int y, int w, int h)
{
    unsigned long bg;
    Window pwin = parent ? parent->win : xinfo.root_win;

    widget->type=type;
    widget->id=id;
    widget->state=WIDGET_STATE_NORMAL;
    bg=get_widget_color(widget);
    if(widget->id==CLIENT_WIN || widget->id==ROOT_WIN)
        widget->win=None;
    else
        widget->win=create_widget_win(pwin, x, y, w, h, 0, 0, bg);
    widget->x=x, widget->y=y, widget->w=w, widget->h=h, widget->border_w=0;
    widget->poppable=false;
    widget->draggable=false;
    widget->parent=parent;
    widget->tooltip=NULL;
    widget_set_method(widget);
    XSelectInput(xinfo.display, widget->win, WIDGET_EVENT_MASK);
    widget_reg(widget);
}

static void widget_set_method(Widget *widget)
{
    widget->del=widget_del;
    widget->show=widget_show;
    widget->hide=widget_hide;
    widget->update_bg=widget_update_bg;
    widget->update_fg=widget_update_fg;
}

void widget_del(Widget *widget)
{
    widget_dtor(widget);
    if(widget->tooltip)
        widget->tooltip->del(widget->tooltip), widget->tooltip=NULL;
    Free(widget);
}

static void widget_dtor(Widget *widget)
{
    if(widget->type != WIDGET_TYPE_CLIENT)
        XDestroyWindow(xinfo.display, widget->win);
    widget_unreg(widget);
}

void widget_set_state(Widget *widget, Widget_state state)
{
    widget->state=state;
}

void widget_set_border_width(Widget *widget, int width)
{
    widget->border_w=width;
    XSetWindowBorderWidth(xinfo.display, widget->win, width);
}

void widget_set_border_color(const Widget *widget, unsigned long pixel)
{
    XSetWindowBorder(xinfo.display, widget->win, pixel);
}

void widget_show(Widget *widget)
{
    XMapWindow(xinfo.display, widget->win);
    XMapSubwindows(xinfo.display, widget->win);
}

void widget_hide(const Widget *widget)
{
    XUnmapWindow(xinfo.display, widget->win);
}

void widget_resize(Widget *widget, int w, int h)
{
    widget->w=w, widget->h=h;
    XResizeWindow(xinfo.display, widget->win, w, h);
}

void widget_move_resize(Widget *widget, int x, int y, int w, int h)
{
    widget_set_rect(widget, x, y, w, h);
    XMoveResizeWindow(xinfo.display, widget->win, x, y, w, h);
}

void widget_update_bg(const Widget *widget)
{
    update_win_bg(widget->win, get_widget_color(widget), None);
}

void widget_update_fg(const Widget *widget)
{
    UNUSED(widget);
}

void widget_set_rect(Widget *widget, int x, int y, int w, int h)
{
    widget->x=x, widget->y=y, widget->w=w, widget->h=h;
}

Rect widget_get_outline(const Widget *widget)
{
    int bw=widget->border_w;
    return (Rect){widget->x, widget->y, widget->w+2*bw, widget->h+2*bw};
}

void widget_set_poppable(Widget *widget, bool poppable)
{
    widget->poppable=poppable;
}

bool widget_get_poppable(const Widget *widget)
{
    return widget->poppable;
}

void widget_set_draggable(Widget *widget, bool draggable)
{
    widget->draggable=draggable;
}

bool widget_get_draggable(const Widget *widget)
{
    return widget->draggable;
}

bool widget_is_viewable(const Widget *widget)
{
    XWindowAttributes a;
    return XGetWindowAttributes(xinfo.display, widget->win, &a) && a.map_state==IsViewable;
}

Widget *get_popped_widget(void)
{
    LIST_FOR_EACH(Widget_node, p, widget_list)
        if(widget_get_poppable(p->widget) && widget_is_viewable(p->widget))
            return p->widget;
    return NULL;
}

void hide_popped_widget(const Widget *popped, const Widget *clicked)
{
    if(popped != clicked)
        popped->hide(popped);
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

/* 坐標均相對於根窗口, 後四個參數是將要彈出的窗口的坐標和尺寸 */
void set_popup_pos(const Widget *widget, bool near_pointer, int *px, int *py, int pw, int ph)
{
    int h=0, bw=0, sw=xinfo.screen_width, sh=xinfo.screen_height;
    Window child, root=xinfo.root_win, win=WIDGET_WIN(widget);

    get_geometry(win, NULL, NULL, NULL, &h, &bw, NULL);
    XTranslateCoordinates(xinfo.display, win, root, 0, 0, px, py, &child);
    if(near_pointer)
        *px=get_pointer_x();

    // 優先考慮右邊顯示彈窗；若不夠位置，則考慮左邊顯示；再不濟則從屏幕左邊開始顯示
    *px = *px+pw<sw ? *px : (*px-pw>0 ? *px-pw : 0);
    /* 優先考慮下邊顯示彈窗；若不夠位置，則考慮上邊顯示；再不濟則從屏幕上邊開始顯示。
       並且彈出窗口與點擊窗口錯開一個像素，以便從視覺上有所區分。*/
    *py = *py+(h+bw+ph)<sh ? *py+h+bw+1: (*py-bw-ph>0 ? *py-bw-ph-1 : 0);
}

static int get_pointer_x(void)
{
    Window win=xinfo.root_win, root, child;
    int rx=0, ry, x, y;
    unsigned int mask;

    XQueryPointer(xinfo.display, win, &root, &child, &rx, &ry, &x, &y, &mask);

    return rx;
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

bool is_valid_click(const Widget *widget, const Buttonbind *bind, XButtonEvent *be)
{
    Widget_id id = widget ? widget->id :
        (be->window==xinfo.root_win ? ROOT_WIN : UNUSED_WIDGET_ID);

    if(!is_func_click(id, bind, be))
        return false;

    if(!widget || widget_get_draggable(widget))
        return true;

    if(!grab_pointer(be->window, CHOOSE))
        return false;

    XEvent ev;
    do
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        handle_event(&ev);
    }while(!is_match_button_release(be, &ev.xbutton));
    XUngrabPointer(xinfo.display, CurrentTime);

    return is_equal_modifier_mask(be->state, ev.xbutton.state)
        && is_pointer_on_win(ev.xbutton.window);
}

static bool is_func_click(const Widget_id id, const Buttonbind *bind, XButtonEvent *be)
{
    return (bind->widget_id == id
        && bind->button == be->button
        && is_equal_modifier_mask(bind->modifier, be->state));
}

unsigned long get_widget_color(const Widget *widget)
{
    Color_id cid = widget ? state_to_color_id(widget->state) : COLOR_NORMAL;
    return find_widget_color(cid);
}

XftColor get_text_color(const Widget *widget)
{
    Color_id cid = widget ? state_to_color_id(widget->state) : COLOR_NORMAL;
    return find_text_color(cid);
}

static Color_id state_to_color_id(Widget_state state)
{
    if(state.active)  return COLOR_ACTIVE; 
    if(state.warn)    return COLOR_WARN; 
    if(state.hot)     return state.chosen ? COLOR_HOT_CHOSEN : COLOR_HOT; 
    if(state.urgent)  return COLOR_URGENT; 
    if(state.attent)  return COLOR_ATTENT; 
    if(state.chosen)  return COLOR_CHOSEN; 
    if(state.unfocused) return COLOR_UNFOCUSED; 
    return COLOR_NORMAL;
}
