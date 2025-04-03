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

#include "button.h"
#include "menu.h"
#include "config.h"
#include "gwm.h"
#include "image.h"
#include "list.h"
#include "prop.h"
#include "ewmh.h"
#include "tooltip.h"
#include "iconbar.h"
#include "statusbar.h"
#include "taskbar.h"

struct _taskbar_tag // 任務欄
{
    Widget base;
    int urgency_n[DESKTOP_N], attent_n[DESKTOP_N]; // 各桌面緊急、注意提示窗口數
    Button *buttons[TASKBAR_BUTTON_N];
    Iconbar *iconbar;
    Statusbar *statusbar;
};

static void taskbar_ctor(Taskbar *taskbar, Widget *parent, int x, int y, int w, int h);
static Rect taskbar_compute_iconbar_rect(Taskbar *taskbar);
static Rect taskbar_compute_statusbar_rect(Taskbar *taskbar, const char *label);
static char *get_statusbar_label(void);
static void taskbar_dtor(Taskbar *taskbar);
static void taskbar_buttons_new(Taskbar *taskbar);
static void taskbar_buttons_del(Taskbar *taskbar);
static bool taskbar_button_is_chosen(Widget_id id);
static Menu *act_center_new(const Taskbar *taskbar);
static void taskbar_set_method(Widget *widget);

Taskbar *taskbar_new(Widget *parent, int x, int y, int w, int h)
{
    Taskbar *taskbar=Malloc(sizeof(Taskbar));
    taskbar_ctor(taskbar, parent, x, y, w, h);
    return taskbar;
}

static void taskbar_ctor(Taskbar *taskbar, Widget *parent, int x, int y, int w, int h)
{
    Rect r;

    widget_ctor(WIDGET(taskbar), parent, WIDGET_TYPE_TASKBAR, TASKBAR, x, y, w, h);
    taskbar_set_method(WIDGET(taskbar));
    taskbar_buttons_new(taskbar);

    r=taskbar_compute_iconbar_rect(taskbar);
    taskbar->iconbar=iconbar_new(WIDGET(taskbar), r.x, r.y, r.w, r.h);

    char *label=get_statusbar_label();
    r=taskbar_compute_statusbar_rect(taskbar, label);
    taskbar->statusbar=statusbar_new(WIDGET(taskbar), r.x, r.y, r.w, r.h, label);
    free(label);

    act_center=act_center_new(taskbar);
    XSelectInput(xinfo.display, WIDGET_WIN(taskbar), CROSSING_MASK);
}

static Rect taskbar_compute_iconbar_rect(Taskbar *taskbar)
{
    int bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N;
    return (Rect){bw, 0, WIDGET_W(taskbar)-bw, WIDGET_H(taskbar)};
}

static Rect taskbar_compute_statusbar_rect(Taskbar *taskbar, const char *label)
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

void taskbar_del(Taskbar *taskbar)
{
    taskbar_dtor(taskbar);
    widget_del(WIDGET(taskbar));
}

static void taskbar_dtor(Taskbar *taskbar)
{
    taskbar_buttons_del(taskbar);
    iconbar_del(taskbar->iconbar);
    statusbar_del(taskbar->statusbar);
}

static void taskbar_buttons_new(Taskbar *taskbar)
{
    int w=cfg->taskbar_button_width, h=WIDGET_H(taskbar);

    for(int i=0; i<TASKBAR_BUTTON_N; i++)
    {
        Widget_id id=TASKBAR_BUTTON_BEGIN+i;
        Widget_state state={.chosen=taskbar_button_is_chosen(id), .unfocused=0};
        taskbar->buttons[i]=button_new(WIDGET(taskbar), id,
            w*i, 0, w, h, cfg->taskbar_button_text[i]);
        widget_set_state(WIDGET(taskbar->buttons[i]), state);
        WIDGET_TOOLTIP(taskbar->buttons[i])=(Widget *)tooltip_new(WIDGET(taskbar->buttons[i]), cfg->tooltip[id]);
    }
}

static void taskbar_buttons_del(Taskbar *taskbar)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        button_del(WIDGET(taskbar->buttons[i])), taskbar->buttons[i]=NULL;
}

static bool taskbar_button_is_chosen(Widget_id id)
{
    unsigned int n=get_net_current_desktop(), lay=get_gwm_layout();

    return (id==DESKTOP_BUTTON_BEGIN+n || id==LAYOUT_BUTTON_BEGIN+lay);
}

static Menu *act_center_new(const Taskbar *taskbar)
{
    int i=WIDGET_INDEX(ACT_CENTER_ITEM, TASKBAR_BUTTON);
    Menu *menu=menu_new(WIDGET(taskbar->buttons[i]),
        ACT_CENTER, cfg->act_center_item_icon, cfg->act_center_item_symbol,
        cfg->act_center_item_label, ACT_CENTER_ITEM_N, cfg->act_center_col);
    widget_set_poppable(WIDGET(menu), true);

    return menu;
}

void taskbar_buttons_update_bg(Taskbar *taskbar)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        widget_update_bg(WIDGET(taskbar->buttons[i]));
}

static void taskbar_set_method(Widget *widget)
{
    widget->update_bg=taskbar_update_bg;
}

void taskbar_update_bg(const Widget *widget)
{
    Taskbar *taskbar=(Taskbar *)widget;
    taskbar_buttons_update_bg(taskbar);
    iconbar_update_bg(WIDGET(taskbar->iconbar));
    /* Xlib手冊說窗口收到Expose事件時會更新背景，但事實上不知道爲何，上邊的語句
     * 雖然給iconbar->win發送了Expose事件，但實際上沒更新背景。也許當窗口沒有內容
     * 時，收到Expose事件並不會更新背景。故只好調用本函數強制更新背景。 */
    XClearWindow(xinfo.display, WIDGET_WIN(taskbar->iconbar));
    widget_update_bg(WIDGET(taskbar->statusbar));
}

void taskbar_set_urgency(Taskbar *taskbar, unsigned int desktop_mask)
{
    unsigned int cur_desktop=get_net_current_desktop();
    int incr = is_on_cur_desktop(desktop_mask) ? -1 : 1;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( i!=cur_desktop && is_on_desktop_n(i, desktop_mask)
            && taskbar->urgency_n[i]+incr >= 0)
            taskbar->urgency_n[i] += incr;
    taskbar_buttons_update_bg(taskbar);
}

void taskbar_set_attention(Taskbar *taskbar, unsigned int desktop_mask)
{
    unsigned int cur_desktop=get_net_current_desktop();
    int incr = is_on_cur_desktop(desktop_mask) ? -1 : 1;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( i!=cur_desktop && is_on_desktop_n(i, desktop_mask)
            && taskbar->attent_n[i]+incr >= 0)
            taskbar->attent_n[i] += incr;
    taskbar_buttons_update_bg(taskbar);
}

void taskbar_buttons_update_bg_by_chosen(Taskbar *taskbar)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        WIDGET_STATE(taskbar->buttons[i]).chosen=
            taskbar_button_is_chosen(TASKBAR_BUTTON_BEGIN+i);
    taskbar_buttons_update_bg(taskbar);
}

Iconbar *taskbar_get_iconbar(const Taskbar *taskbar)
{
    return taskbar->iconbar;
}

Statusbar *taskbar_get_statusbar(const Taskbar *taskbar)
{
    return taskbar->statusbar;
}

void taskbar_add_client(Taskbar *taskbar, Window cwin)
{
    iconbar_add_cbutton(taskbar->iconbar, cwin);
}

void taskbar_remove_client(Taskbar *taskbar, Window cwin)
{
    iconbar_del_cbutton(taskbar->iconbar, cwin);
}
