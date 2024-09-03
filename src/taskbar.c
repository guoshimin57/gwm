/* *************************************************************************
 *     taskbar->c：實現任務欄相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "button.h"
#include "config.h"
#include "gwm.h"
#include "image.h"
#include "list.h"
#include "prop.h"
#include "tooltip.h"
#include "taskbar.h"

typedef struct // 狀態欄
{
    Widget base;
    char *label;
} Statusbar;

typedef struct cbutton_tag
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

struct _taskbar_tag // 任務欄
{
    Widget base;
    int urgency_n[DESKTOP_N], attent_n[DESKTOP_N]; // 各桌面緊急、注意提示窗口數
    Button *buttons[TASKBAR_BUTTON_N];
    Iconbar *iconbar;
    Statusbar *statusbar;
};

static void set_taskbar_method(Widget *widget);
static void create_taskbar_buttons(void);
static bool is_chosen_taskbar_button(Widget_id id);
static void create_iconbar(void);
static void create_statusbar(void);
static void create_act_center(void);
static Cbutton *create_cbutton(Widget *parent, int x, int y, int w, int h, Window cwin);
static void set_cbutton_icon(Cbutton *cbutton);
static void destroy_cbutton(Cbutton *cbutton);
static bool have_similar_cbutton(const Cbutton *cbutton);
static Cbutton *win_to_cbutton(Window cwin);

Taskbar *taskbar=NULL;

void create_taskbar(void)
{
    int w=xinfo.screen_width, h=get_font_height_by_pad(),
        y=(cfg->taskbar_on_top ? 0 : xinfo.screen_height-h);

    taskbar=Malloc(sizeof(Taskbar));
    init_widget(WIDGET(taskbar), NULL, TASKBAR, WIDGET_STATE_1(current),
        0, y, w, h);
    set_taskbar_method(WIDGET(taskbar));
    XSelectInput(xinfo.display, WIDGET_WIN(taskbar), CROSSING_MASK);

    create_taskbar_buttons();
    create_iconbar();
    create_statusbar();
    create_act_center();
}

static void set_taskbar_method(Widget *widget)
{
    widget->update_bg=update_taskbar_bg;
}

static void create_taskbar_buttons(void)
{
    int w=cfg->taskbar_button_width, h=WIDGET_H(taskbar);

    for(int i=0; i<TASKBAR_BUTTON_N; i++)
    {
        Widget_id id=TASKBAR_BUTTON_BEGIN+i;
        Widget_state state={.chosen=is_chosen_taskbar_button(id), .current=1};
        taskbar->buttons[i]=create_button(WIDGET(taskbar), id, state,
            w*i, 0, w, h, cfg->taskbar_button_text[i]);
        create_tooltip(WIDGET(taskbar->buttons[i]), cfg->tooltip[id]);
    }
}

static bool is_chosen_taskbar_button(Widget_id id)
{
    unsigned int n=get_net_current_desktop(), lay=get_gwm_current_layout();

    return (id==DESKTOP_BUTTON_BEGIN+n || id==LAYOUT_BUTTON_BEGIN+lay);
}

static void create_iconbar(void)
{
    int bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N, x=bw,
        w=WIDGET_W(taskbar)-bw, h=WIDGET_H(taskbar);

    taskbar->iconbar=Malloc(sizeof(Iconbar));
    init_widget(WIDGET(taskbar->iconbar), WIDGET(taskbar),
        ICONBAR, WIDGET_STATE(taskbar), x, 0, w, h);
    taskbar->iconbar->cbuttons=Malloc(sizeof(Cbutton));
    list_init(&taskbar->iconbar->cbuttons->list);
}

static void create_statusbar(void)
{
    int w=0;
    char *p=get_text_prop(xinfo.root_win, XA_WM_NAME);

    taskbar->statusbar=Malloc(sizeof(Statusbar));
        p=copy_string("gwm");
    get_string_size(p, &w, NULL);
    w += 2*get_font_pad();
    if(w > cfg->status_area_width_max)
        w=cfg->status_area_width_max;
    else if(w == 0)
        w=1;
    init_widget(WIDGET(taskbar->statusbar), WIDGET(taskbar), STATUSBAR,
        WIDGET_STATE(taskbar), WIDGET_W(taskbar)-w, 0, w, WIDGET_H(taskbar));
    XSelectInput(xinfo.display, WIDGET_WIN(taskbar->statusbar), ExposureMask);
    taskbar->statusbar->label=copy_string(p);
}

static void create_act_center(void)
{
    int i=WIDGET_INDEX(ACT_CENTER_ITEM, TASKBAR_BUTTON);
    act_center=create_menu(WIDGET(taskbar->buttons[i]),
        ACT_CENTER, cfg->act_center_item_icon, cfg->act_center_item_symbol,
        cfg->act_center_item_label, ACT_CENTER_ITEM_N, cfg->act_center_col);
}

void taskbar_add_cbutton(Window cwin)
{
    int h=WIDGET_H(taskbar->iconbar), w=h;
    Cbutton *c=create_cbutton(WIDGET(taskbar->iconbar), 0, 0, w, h, cwin);
    list_add(&c->list, &taskbar->iconbar->cbuttons->list);
    update_iconbar();
    WIDGET(c->button)->show(WIDGET(c->button));
}

static Cbutton *create_cbutton(Widget *parent, int x, int y, int w, int h, Window cwin)
{
    Cbutton *cbutton=Malloc(sizeof(Cbutton));
    char *icon_title=get_icon_title_text(cwin, "");

    cbutton->button=create_button(parent, CLIENT_ICON, WIDGET_STATE_1(current),
        x, y, w, h, icon_title);
    set_button_align(cbutton->button, CENTER_LEFT);

    cbutton->cwin=cwin;

    set_cbutton_icon(cbutton);
    create_tooltip(WIDGET(cbutton->button), icon_title);
    Free(icon_title);

    return cbutton;
}

static void set_cbutton_icon(Cbutton *cbutton)
{
    XClassHint class_hint={NULL, NULL};

    XGetClassHint(xinfo.display, cbutton->cwin, &class_hint);
    Imlib_Image image=get_icon_image(cbutton->cwin, class_hint.res_name,
        cfg->icon_image_size, cfg->cur_icon_theme);

    set_button_icon(cbutton->button, image, class_hint.res_name, NULL);
    XFree(class_hint.res_name), XFree(class_hint.res_class);
}

void taskbar_del_cbutton(Window cwin)
{
    list_for_each_entry_safe(Cbutton, c, &taskbar->iconbar->cbuttons->list, list)
    {
        if(c->cwin == cwin)
        {
            list_del(&c->list);
            destroy_cbutton(c);
            update_iconbar();
            break;
        }
    }
}

static void destroy_cbutton(Cbutton *cbutton)
{
    destroy_button(cbutton->button), cbutton->button=NULL;
    free(cbutton);
}

Window get_iconic_win(Window button_win)
{
    for(unsigned int i=0; i<DESKTOP_N; i++)
        list_for_each_entry(Cbutton, c, &taskbar->iconbar->cbuttons->list, list)
            if(WIDGET_WIN(c->button) == button_win)
                return c->cwin;
    return None;
}

void update_iconbar(void)
{
    int x=0, w=0, h=WIDGET_H(taskbar->iconbar), wi=h, wl=0;

    list_for_each_entry(Cbutton, c, &taskbar->iconbar->cbuttons->list, list)
    {
        Button *b=c->button;
        if(have_similar_cbutton(c))
        {
            get_string_size(get_button_label(b), &wl, NULL);
            w=MIN(wi+wl, cfg->icon_win_width_max);
        }
        else
            w=wi;
        move_resize_widget(WIDGET(b), x, WIDGET_Y(b), w, WIDGET_H(b)); 
        x+=w+cfg->icon_gap;
    }
}

static bool have_similar_cbutton(const Cbutton *cbutton)
{
    XClassHint ch={NULL, NULL}, ph;
    if(!XGetClassHint(xinfo.display, cbutton->cwin, &ch))
        return false;

    bool same=false;
    list_for_each_entry(Cbutton, p, &taskbar->iconbar->cbuttons->list, list)
    {
        if(p!=cbutton && XGetClassHint(xinfo.display, p->cwin, &ph))
        {
            same = (strcmp(ph.res_class, ch.res_class) == 0
                && strcmp(ph.res_name, ch.res_name) == 0);
            XFree(ph.res_class), XFree(ph.res_name);
            if(same)
                break;
        }
    }
    
    XFree(ch.res_class), XFree(ch.res_name);

    return same;
}

void update_iconbar_by_state(Window cwin)
{
    Net_wm_state state=get_net_wm_state(cwin);
    
    if(win_to_cbutton(cwin))
    {
        if(!state.hidden)
            taskbar_del_cbutton(cwin);
    }
    else if(state.hidden)
        taskbar_add_cbutton(cwin);
}

static Cbutton *win_to_cbutton(Window cwin)
{
    list_for_each_entry(Cbutton, p, &taskbar->iconbar->cbuttons->list, list)
        if(p->cwin == cwin)
            return p;
    return NULL;
}

void update_taskbar_buttons_bg(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        update_widget_bg(WIDGET(taskbar->buttons[i]));
}

void update_statusbar_fg(void)
{
    Statusbar *b=taskbar->statusbar;
    XftColor fg=get_widget_fg(WIDGET_STATE(b));
    if(b->label)
    {
        Str_fmt fmt={0, 0, WIDGET_W(b), WIDGET_H(b), CENTER, true, false, 0, fg};
        draw_string(WIDGET_WIN(b), b->label, &fmt);
    }
}

void set_statusbar_label(const char *label)
{
    int w=0, h=WIDGET_H(taskbar->statusbar);

    Free(taskbar->statusbar->label);
    taskbar->statusbar->label=copy_string(label);
    get_string_size(label, &w, NULL);
    w += 2*get_font_pad();
    if(w > cfg->status_area_width_max)
        w=cfg->status_area_width_max;
    if(w != taskbar->statusbar->base.w)
        move_resize_widget(WIDGET(taskbar->statusbar), taskbar->base.w-w, 0, w, h);
    update_statusbar_fg();
}

void update_taskbar_bg(const Widget *widget)
{
    Taskbar *taskbar=(Taskbar *)widget;
    update_taskbar_buttons_bg();
    update_widget_bg(WIDGET(taskbar->iconbar));
    /* Xlib手冊說窗口收到Expose事件時會更新背景，但事實上不知道爲何，上邊的語句
     * 雖然給iconbar->win發送了Expose事件，但實際上沒更新背景。也許當窗口沒有內容
     * 時，收到Expose事件並不會更新背景。故只好調用本函數強制更新背景。 */
    XClearWindow(xinfo.display, taskbar->iconbar->base.win);
    update_widget_bg(WIDGET(taskbar->statusbar));
}

void destroy_taskbar(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        destroy_button(taskbar->buttons[i]), taskbar->buttons[i]=NULL;
    list_for_each_entry_safe(Cbutton, c, &taskbar->iconbar->cbuttons->list, list)
        destroy_cbutton(c);
    Free(taskbar->iconbar->cbuttons);
    destroy_widget(WIDGET(taskbar->iconbar)), taskbar->iconbar=NULL;
    Free(taskbar->statusbar->label);
    destroy_widget(WIDGET(taskbar->statusbar)), taskbar->statusbar=NULL;
    destroy_widget(WIDGET(taskbar)), taskbar=NULL;
}

void set_taskbar_urgency(Window cwin, bool urg)
{
    unsigned int cur_desktop=get_net_current_desktop();
    int incr = (urg ? 1 : -1);
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( i!=cur_desktop && is_on_desktop_n(cwin, i+1)
            && taskbar->urgency_n[i]+incr >= 0)
            taskbar->urgency_n[i] += incr;
    update_taskbar_buttons_bg();
}

void set_taskbar_attention(Window cwin, bool attent)
{
    unsigned int cur_desktop=get_net_current_desktop();
    int incr = attent ? 1 : -1;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( i!=cur_desktop && is_on_desktop_n(cwin, (i+1))
            && taskbar->attent_n[i]+incr >= 0)
            taskbar->attent_n[i] += incr;
    update_taskbar_buttons_bg();
}

void update_taskbar_buttons_bg_by_chosen(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        WIDGET_STATE(taskbar->buttons[i]).chosen=
            is_chosen_taskbar_button(TASKBAR_BUTTON_BEGIN+i);
    update_taskbar_buttons_bg();
}
