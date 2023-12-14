/* *************************************************************************
 *     taskbar->c：實現任務欄相關的功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"


static void create_taskbar_buttons(void);
static bool is_chosen_taskbar_button(Widget_type type);
static void create_icon_area(void);
static void create_status_area(void);
static void create_act_center(void);

Taskbar *taskbar=NULL;

Taskbar *create_taskbar(void)
{
    taskbar=malloc_s(sizeof(Taskbar));
    memset(taskbar, 0, sizeof(Taskbar));

    taskbar->w=xinfo.screen_width, taskbar->h=get_font_height_by_pad();
    taskbar->y=(cfg->taskbar_on_top ? 0 : xinfo.screen_height-taskbar->h);
    taskbar->win=create_widget_win(xinfo.root_win, taskbar->x, taskbar->y,
        taskbar->w, taskbar->h, 0, 0, get_widget_color(TASKBAR_COLOR));
    XSelectInput(xinfo.display, taskbar->win, CROSSING_MASK);
    create_taskbar_buttons();
    create_status_area();
    create_icon_area();
    create_act_center();
    XMapSubwindows(xinfo.display, taskbar->win);
    if(cfg->show_taskbar)
        XMapWindow(xinfo.display, taskbar->win);

    return taskbar;
}

static void create_taskbar_buttons(void)
{
    int w=cfg->taskbar_button_width, h=taskbar->h;

    for(int i=0; i<TASKBAR_BUTTON_N; i++)
    {
        bool chosen=is_chosen_taskbar_button(TASKBAR_BUTTON_BEGIN+i);
        unsigned long color=get_widget_color(chosen ? CHOSEN_BUTTON_COLOR : TASKBAR_COLOR);
        taskbar->buttons[i]=create_widget_win(taskbar->win, w*i, 0, w, h, 0, 0, color);
        XSelectInput(xinfo.display, taskbar->buttons[i], BUTTON_EVENT_MASK);
    }
}

static bool is_chosen_taskbar_button(Widget_type type)
{
    unsigned int desk;
    Layout lay;

    return ((get_net_current_desktop(&desk) && type==DESKTOP_BUTTON_BEGIN+desk)
        || (get_gwm_current_layout(&lay) && type==LAYOUT_BUTTON_BEGIN+lay));
}

static void create_icon_area(void)
{
    int bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N,
        w=taskbar->w-bw-taskbar->status_area_w;
    taskbar->icon_area=create_widget_win(taskbar->win,
        bw, 0, w, taskbar->h, 0, 0, get_widget_color(TASKBAR_COLOR));
}

static void create_status_area(void)
{
    taskbar->status_text=get_text_prop(xinfo.root_win, XA_WM_NAME);
    if(!taskbar->status_text)
        taskbar->status_text=copy_string("gwm");
    get_string_size(taskbar->status_text, &taskbar->status_area_w, NULL);
    if(taskbar->status_area_w > cfg->status_area_width_max)
        taskbar->status_area_w=cfg->status_area_width_max;
    else if(taskbar->status_area_w == 0)
        taskbar->status_area_w=1;
    taskbar->status_area=create_widget_win(taskbar->win,
        taskbar->w-taskbar->status_area_w, 0, taskbar->status_area_w,
        taskbar->h, 0, 0, get_widget_color(TASKBAR_COLOR));
    XSelectInput(xinfo.display, taskbar->status_area, ExposureMask);
}

static void create_act_center(void)
{
    act_center=create_menu(cfg->act_center_item_text,
        ACT_CENTER_ITEM_N, cfg->act_center_col);
}

void update_taskbar_buttons_bg(WM *wm)
{
    for(Widget_type t=TASKBAR_BUTTON_BEGIN; t<=TASKBAR_BUTTON_END; t++)
        update_taskbar_button_bg(wm, t);
}

void update_taskbar_button_bg(WM *wm, Widget_type type)
{
    Window win=taskbar->buttons[WIDGET_INDEX(type, TASKBAR_BUTTON)];
    bool chosen=is_chosen_taskbar_button(type);
    unsigned long color=get_widget_color(chosen ? CHOSEN_BUTTON_COLOR : TASKBAR_COLOR);

    if(IS_WIDGET_CLASS(type, DESKTOP_BUTTON))
    {
        unsigned int desktop_n=WIDGET_INDEX(type, DESKTOP_BUTTON)+1;
        if(desktop_n != wm->cur_desktop)
        {
            if(taskbar->urgency_n[desktop_n-1])
                color=get_widget_color(URGENCY_WIDGET_COLOR);
            else if(taskbar->attent_n[desktop_n-1])
                color=get_widget_color(ATTENTION_WIDGET_COLOR);
        }
    }
    update_win_bg(win, color, None);
}

void update_taskbar_button_fg(Widget_type type)
{
    size_t i=WIDGET_INDEX(type, TASKBAR_BUTTON);
    Str_fmt fmt={0, 0, cfg->taskbar_button_width, taskbar->h, CENTER,
        false, false, 0, get_text_color(TASKBAR_TEXT_COLOR)};
    draw_string(taskbar->buttons[i], cfg->taskbar_button_text[i], &fmt);
}

void update_status_area_fg(void)
{
    Str_fmt fmt={0, 0, taskbar->status_area_w, taskbar->h, CENTER_RIGHT,
        false, false, 0, get_text_color(TASKBAR_TEXT_COLOR)};
    draw_string(taskbar->status_area, taskbar->status_text, &fmt);
}

void update_icon_status_area(void)
{
    int w, bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N;

    get_string_size(taskbar->status_text, &w, NULL);
    if(w > cfg->status_area_width_max)
        w=cfg->status_area_width_max;
    if(w != taskbar->status_area_w)
    {
        XMoveResizeWindow(xinfo.display, taskbar->status_area,
            taskbar->w-w, 0, w, taskbar->h);
        XMoveResizeWindow(xinfo.display, taskbar->icon_area,
            bw, 0, taskbar->w-bw-w, taskbar->h);
    }
    taskbar->status_area_w=w;
    update_status_area_fg();
}

void update_client_icon_fg(WM *wm, Window win)
{
    Client *c=win_to_iconic_state_client(wm, win);
    if(!c)
        return;

    Icon *i=c->icon;
    draw_icon(c->icon->win, c->image, c->class_name, taskbar->h);
    if(i->show_text)
    {
        Str_fmt fmt={taskbar->h, 0, i->w-taskbar->h, i->h, CENTER,
            false, false, 0, get_text_color(TASKBAR_TEXT_COLOR)};
        draw_string(i->win, i->title_text, &fmt);
    }
}

void destroy_taskbar(Taskbar *taskbar)
{
    XDestroyWindow(xinfo.display, taskbar->win);
    vfree(taskbar->status_text, taskbar, NULL);
}
