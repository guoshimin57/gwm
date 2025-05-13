/* *************************************************************************
 *     taskbar.c：實現任務欄相關的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <X11/Xatom.h>
#include "button.h"
#include "misc.h"
#include "menu.h"
#include "config.h"
#include "gwm.h"
#include "image.h"
#include "list.h"
#include "prop.h"
#include "ewmh.h"
#include "drawable.h"
#include "tooltip.h"
#include "taskbar.h"

typedef struct // 縮微窗口按鈕
{
    Button *button;
    Window cwin; // 縮微客戶窗口
    List list;
} Cbutton;

typedef struct // 縮微窗口欄
{
    Widget base;
    Cbutton *cbuttons;
} Iconbar;

typedef struct // 狀態欄
{
    Widget base;
    char *label;
} Statusbar;

struct _taskbar_tag // 任務欄
{
    Widget base;
    int urgency_n[DESKTOP_N], attent_n[DESKTOP_N]; // 各桌面緊急、注意提示窗口數
    Button *buttons[TASKBAR_BUTTON_N];
    Iconbar *iconbar;
    Statusbar *statusbar;
    Menu *act_center;
};

static void taskbar_ctor(Widget *parent, int x, int y, int w, int h);
static Rect taskbar_compute_iconbar_rect(void);
static Rect taskbar_compute_statusbar_rect(const char *label);
static char *get_statusbar_label(void);
static void taskbar_dtor(void);
static void taskbar_buttons_new(void);
static void taskbar_buttons_del(void);
static bool taskbar_button_is_chosen(Widget_id id);
static void taskbar_buttons_update_bg(void);
static Cbutton *cbutton_new(Widget *parent, int x, int y, int w, int h, Window cwin);
static void cbutton_del(Cbutton *cbutton);
static void cbutton_ctor(Cbutton *cbutton, Widget *parent, int x, int y, int w, int h, Window cwin);
static void cbutton_dtor(Cbutton *cbutton);
static void cbutton_set_icon(Cbutton *cbutton);
static Cbutton *iconbar_find_cbutton(const Iconbar *iconbar, Window cwin);
static Iconbar *iconbar_new(Widget *parent, int x, int y, int w, int h);
static void iconbar_ctor(Iconbar *iconbar, Widget *parent, int x, int y, int w, int h);
static void iconbar_set_method(Widget *widget);
static void iconbar_del(Iconbar *iconbar);
static void iconbar_dtor(Iconbar *iconbar);
static void iconbar_add_cbutton(Iconbar *iconbar, Window cwin);
static void iconbar_del_cbutton(Iconbar *iconbar, Window cwin);
static void iconbar_update(Iconbar *iconbar);
static bool iconbar_has_similar_cbutton(Iconbar *iconbar, const Cbutton *cbutton);
static void iconbar_update_bg(const Widget *widget);
static Statusbar *statusbar_new(Widget *parent, int x, int y, int w, int h, const char *label);
static void statusbar_ctor(Statusbar *statusbar, Widget *parent, int x, int y, int w, int h, const char *label);
static void statusbar_set_method(Widget *widget);
static void statusbar_del(Statusbar *statusbar);
static void statusbar_dtor(Statusbar *statusbar);
static void statusbar_update_fg(const Widget *widget);
static Menu *act_center_new(void);

static Taskbar *taskbar=NULL; // 每個WM自身只有一個任務欄

Taskbar *get_taskbar(void)
{
    return taskbar;
}

void taskbar_new(Widget *parent, int x, int y, int w, int h)
{
    taskbar=Malloc(sizeof(Taskbar));
    taskbar_ctor(parent, x, y, w, h);
}

static void taskbar_ctor(Widget *parent, int x, int y, int w, int h)
{
    Rect r;

    widget_ctor(WIDGET(taskbar), parent, WIDGET_TYPE_TASKBAR, TASKBAR, x, y, w, h);
    taskbar_buttons_new();

    r=taskbar_compute_iconbar_rect();
    taskbar->iconbar=iconbar_new(WIDGET(taskbar), r.x, r.y, r.w, r.h);

    char *label=get_statusbar_label();
    r=taskbar_compute_statusbar_rect(label);
    taskbar->statusbar=statusbar_new(WIDGET(taskbar), r.x, r.y, r.w, r.h, label);
    free(label);

    taskbar->act_center=act_center_new();

    XSelectInput(xinfo.display, WIDGET_WIN(taskbar), CROSSING_MASK);
}

static Rect taskbar_compute_iconbar_rect(void)
{
    int bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N;
    return (Rect){bw, 0, WIDGET_W(taskbar)-bw, WIDGET_H(taskbar)};
}

static Rect taskbar_compute_statusbar_rect(const char *label)
{
    int w=0;

    get_string_size(label, &w, NULL);
    w += 2*get_font_pad();
    if(w > cfg->statusbar_width_max)
        w=cfg->statusbar_width_max;
    else if(w == 0)
        w=1;

    return (Rect){WIDGET_W(taskbar)-w, 0, w, WIDGET_H(taskbar)};
}

static char *get_statusbar_label(void)
{
    char *label=get_text_prop(xinfo.root_win, XA_WM_NAME);
    return label ? label : copy_string("gwm");
}

void taskbar_del(void)
{
    taskbar_dtor();
    widget_del(WIDGET(taskbar));
}

static void taskbar_dtor(void)
{
    taskbar_buttons_del();
    iconbar_del(taskbar->iconbar);
    statusbar_del(taskbar->statusbar);
    menu_del(taskbar->act_center);
}

static void taskbar_buttons_new(void)
{
    int w=cfg->taskbar_button_width, h=WIDGET_H(taskbar);

    for(int i=0; i<TASKBAR_BUTTON_N; i++)
    {
        Widget_id id=TASKBAR_BUTTON_BEGIN+i;
        Widget_state state={.chosen=taskbar_button_is_chosen(id), .unfocused=0};
        taskbar->buttons[i]=button_new(WIDGET(taskbar), id,
            w*i, 0, w, h, cfg->widget_labels[id]);
        button_set_icon(taskbar->buttons[i], NULL,
            cfg->widget_icon_names[id], cfg->widget_symbols[id]);
        widget_set_state(WIDGET(taskbar->buttons[i]), state);
        set_tooltip(WIDGET(taskbar->buttons[i]), cfg->tooltip[id]);
    }
}

static void taskbar_buttons_del(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        button_del(WIDGET(taskbar->buttons[i])), taskbar->buttons[i]=NULL;
}

static bool taskbar_button_is_chosen(Widget_id id)
{
    unsigned int n=get_net_current_desktop(), lay=get_gwm_layout();

    return (id==DESKTOP_BUTTON_BEGIN+n || id==LAYOUT_BUTTON_BEGIN+lay);
}

void taskbar_update_bg(void)
{
    taskbar_buttons_update_bg();
    iconbar_update_bg(WIDGET(taskbar->iconbar));
    widget_update_bg(WIDGET(taskbar->statusbar));
    menu_update_bg(WIDGET(taskbar->act_center));
}

static void taskbar_buttons_update_bg(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        WIDGET_STATE(taskbar->buttons[i]).chosen=
            taskbar_button_is_chosen(TASKBAR_BUTTON_BEGIN+i);
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        widget_update_bg(WIDGET(taskbar->buttons[i]));
}

void taskbar_set_urgency(const Client *c)
{
    unsigned int cur_desktop=get_net_current_desktop();
    int incr = is_on_cur_desktop(c) ? -1 : 1;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( i!=cur_desktop && is_on_desktop_n(c, i)
            && taskbar->urgency_n[i]+incr >= 0)
            taskbar->urgency_n[i] += incr;
    taskbar_buttons_update_bg();
}

void taskbar_set_attention(const Client *c)
{
    unsigned int cur_desktop=get_net_current_desktop();
    int incr = is_on_cur_desktop(c) ? -1 : 1;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( i!=cur_desktop && is_on_desktop_n(c, i)
            && taskbar->attent_n[i]+incr >= 0)
            taskbar->attent_n[i] += incr;
    taskbar_buttons_update_bg();
}

void taskbar_add_client(Window cwin)
{
    iconbar_add_cbutton(taskbar->iconbar, cwin);
}

void taskbar_remove_client(Window cwin)
{
    iconbar_del_cbutton(taskbar->iconbar, cwin);
}

static Cbutton *cbutton_new(Widget *parent, int x, int y, int w, int h, Window cwin)
{
    Cbutton *cbutton=Malloc(sizeof(Cbutton));
    cbutton_ctor(cbutton, parent, x, y, w, h, cwin);
    return cbutton;
}

static void cbutton_del(Cbutton *cbutton)
{
    cbutton_dtor(cbutton);
    free(cbutton);
}

static void cbutton_ctor(Cbutton *cbutton, Widget *parent, int x, int y, int w, int h, Window cwin)
{
    char *icon_title=get_icon_title_text(cwin, "");

    cbutton->button=button_new(parent, CLIENT_ICON,
        x, y, w, h, icon_title);
    button_set_align(cbutton->button, CENTER_LEFT);

    cbutton->cwin=cwin;

    cbutton_set_icon(cbutton);
    WIDGET_TOOLTIP(cbutton->button)=(Widget *)tooltip_new(WIDGET(cbutton->button), icon_title);
    free(icon_title);
}

static void cbutton_dtor(Cbutton *cbutton)
{
    button_del(WIDGET(cbutton->button));
    cbutton->button=NULL;
}

static void cbutton_set_icon(Cbutton *cbutton)
{
    XClassHint class_hint={NULL, NULL};

    XGetClassHint(xinfo.display, cbutton->cwin, &class_hint);
    Imlib_Image image=get_win_icon_image(cbutton->cwin);

    button_set_icon(cbutton->button, image, class_hint.res_name, NULL);
    vXFree(class_hint.res_name, class_hint.res_class);
}

Window taskbar_get_client_win(const Window button_win)
{
    for(unsigned int i=0; i<DESKTOP_N; i++)
        LIST_FOR_EACH(Cbutton, cb, taskbar->iconbar->cbuttons)
            if(WIDGET_WIN(cb->button) == button_win)
                return cb->cwin;
    return None;
}

static Cbutton *iconbar_find_cbutton(const Iconbar *iconbar, Window cwin)
{
    LIST_FOR_EACH(Cbutton, p, iconbar->cbuttons)
        if(p->cwin == cwin)
            return p;
    return NULL;
}

static Iconbar *iconbar_new(Widget *parent, int x, int y, int w, int h)
{
    Iconbar *iconbar=Malloc(sizeof(Iconbar));
    iconbar_ctor(iconbar, parent, x, y, w, h);
    iconbar_set_method(WIDGET(iconbar));
    return iconbar;
}

static void iconbar_ctor(Iconbar *iconbar, Widget *parent, int x, int y, int w, int h)
{
    widget_ctor(WIDGET(iconbar), parent, WIDGET_TYPE_ICONBAR, ICONBAR, x, y, w, h);
    iconbar->cbuttons=Malloc(sizeof(Cbutton));
    LIST_INIT(iconbar->cbuttons);
}

static void iconbar_set_method(Widget *widget)
{
    widget->update_bg=iconbar_update_bg;
}

static void iconbar_del(Iconbar *iconbar)
{
    iconbar_dtor(iconbar);
    widget_del(WIDGET(iconbar));
}

static void iconbar_dtor(Iconbar *iconbar)
{
    LIST_FOR_EACH_SAFE(Cbutton, c, iconbar->cbuttons)
        cbutton_del(c);
    Free(iconbar->cbuttons);
}

static void iconbar_add_cbutton(Iconbar *iconbar, Window cwin)
{
    int h=WIDGET_H(iconbar), w=h;
    Cbutton *c=cbutton_new(WIDGET(iconbar), 0, 0, w, h, cwin);
    LIST_ADD(c, iconbar->cbuttons);
    iconbar_update(iconbar);
    WIDGET(c->button)->show(WIDGET(c->button));
}

static void iconbar_del_cbutton(Iconbar *iconbar, Window cwin)
{
    LIST_FOR_EACH_SAFE(Cbutton, c, iconbar->cbuttons)
    {
        if(c->cwin == cwin)
        {
            LIST_DEL(c);
            cbutton_del(c);
            iconbar_update(iconbar);
            break;
        }
    }
}

static void iconbar_update(Iconbar *iconbar)
{
    int x=0, w=0, h=WIDGET_H(iconbar), wi=h, wl=0, pad=get_font_pad();

    LIST_FOR_EACH(Cbutton, c, iconbar->cbuttons)
    {
        Button *b=c->button;
        if(iconbar_has_similar_cbutton(iconbar, c))
        {
            get_string_size(button_get_label(b), &wl, NULL);
            w=MIN(wi+wl+2*pad, cfg->iconbar_width_max);
        }
        else
            w=wi;
        widget_move_resize(WIDGET(b), x, WIDGET_Y(b), w, WIDGET_H(b)); 
        x+=w+cfg->icon_gap;
    }
}

static bool iconbar_has_similar_cbutton(Iconbar *iconbar, const Cbutton *cbutton)
{
    bool result=false;
    XClassHint ch={NULL, NULL}, ph;

    if(!XGetClassHint(xinfo.display, cbutton->cwin, &ch))
        return false;

    LIST_FOR_EACH(Cbutton, p, iconbar->cbuttons)
    {
        if(p!=cbutton && XGetClassHint(xinfo.display, p->cwin, &ph))
        {
            result=!strcmp(ph.res_class, ch.res_class);
            vXFree(ph.res_class, ph.res_name);
            if(result)
                break;
        }
    }

    vXFree(ch.res_class, ch.res_name);

    return result;
}

void taskbar_update_by_client_state(Window cwin)
{
    Net_wm_state state=get_net_wm_state(cwin);
    
    if(iconbar_find_cbutton(taskbar->iconbar, cwin))
    {
        if(!state.hidden)
            iconbar_del_cbutton(taskbar->iconbar, cwin);
    }
    else if(state.hidden)
    {
        iconbar_add_cbutton(taskbar->iconbar, cwin);
    }
}

void taskbar_update_by_icon_name(const Window cwin, const char *icon_name)
{
    Cbutton *cbutton=iconbar_find_cbutton(taskbar->iconbar, cwin);
    if(cbutton == NULL)
        return;

    button_set_label(cbutton->button, icon_name);
    iconbar_update(taskbar->iconbar);
}

void taskbar_update_by_icon_image(const Window cwin, Imlib_Image image)
{
    Cbutton *cbutton=iconbar_find_cbutton(taskbar->iconbar, cwin);
    if(cbutton == NULL)
        return;

    button_change_icon(cbutton->button, image, NULL, NULL);
    button_update_fg(WIDGET(cbutton->button));
}

static void iconbar_update_bg(const Widget *widget)
{
    const Iconbar *iconbar=(const Iconbar *)widget;

    widget_update_bg(WIDGET(iconbar));
    /* Xlib手冊說窗口收到Expose事件時會更新背景，但事實上不知道爲何，上邊的語句
     * 雖然給iconbar->win發送了Expose事件，但實際上沒更新背景。也許當窗口沒有內容
     * 時，收到Expose事件並不會更新背景。故只好調用本函數強制更新背景。 */
    XClearWindow(xinfo.display, WIDGET_WIN(taskbar->iconbar));

    LIST_FOR_EACH(Cbutton, cb, iconbar->cbuttons)
        widget_update_bg(WIDGET(cb->button));
}

static Statusbar *statusbar_new(Widget *parent, int x, int y, int w, int h, const char *label)
{
    Statusbar *statusbar=Malloc(sizeof(Statusbar));
    statusbar_ctor(statusbar, parent, x, y, w, h, label);
    return statusbar;
}

static void statusbar_ctor(Statusbar *statusbar, Widget *parent, int x, int y, int w, int h, const char *label)
{
    widget_ctor(WIDGET(statusbar), parent, WIDGET_TYPE_STATUSBAR, STATUSBAR, x, y, w, h);
    statusbar_set_method(WIDGET(statusbar));
    XSelectInput(xinfo.display, WIDGET_WIN(statusbar), ExposureMask);
    statusbar->label=copy_string(label);
}

static void statusbar_set_method(Widget *widget)
{
    widget->update_fg=statusbar_update_fg;
}

static void statusbar_del(Statusbar *statusbar)
{
    statusbar_dtor(statusbar);
    widget_del(WIDGET(statusbar));
}

static void statusbar_dtor(Statusbar *statusbar)
{
    Free(statusbar->label);
}

static void statusbar_update_fg(const Widget *widget)
{
    const Statusbar *statusbar=(const Statusbar *)widget;
    XftColor fg=get_text_color(widget);
    if(statusbar->label)
    {
        Str_fmt fmt={0, 0, WIDGET_W(statusbar),
            WIDGET_H(statusbar), CENTER, true, false, 0, fg};
        draw_string(WIDGET_WIN(statusbar), statusbar->label, &fmt);
    }
}

void taskbar_change_statusbar_label(const char *label)
{
    Statusbar *s=taskbar->statusbar;
    int x=WIDGET_X(s), y=WIDGET_Y(s), w=WIDGET_W(s), h=WIDGET_H(s), nw=0;

    Free(s->label);
    s->label=copy_string(label);
    get_string_size(label, &nw, NULL);
    nw += 2*get_font_pad();
    if(nw > cfg->statusbar_width_max)
        nw=cfg->statusbar_width_max;
    if(nw != w)
        widget_move_resize(WIDGET(s), x+w-nw, y, nw, h);
    statusbar_update_fg(WIDGET(s));
}

static Menu *act_center_new(void)
{
    int i=WIDGET_INDEX(ACT_CENTER_ITEM, TASKBAR_BUTTON);
    Menu *menu=menu_new(WIDGET(taskbar->buttons[i]), ACT_CENTER,
        cfg->widget_icon_names+ACT_CENTER_ITEM_BEGIN,
        cfg->widget_symbols+ACT_CENTER_ITEM_BEGIN,
        cfg->widget_labels+ACT_CENTER_ITEM_BEGIN,
        ACT_CENTER_ITEM_N, cfg->act_center_col);
    widget_set_poppable(WIDGET(menu), true);

    return menu;
}

void taskbar_show_act_center(void)
{
    menu_show(WIDGET(taskbar->act_center));
}
