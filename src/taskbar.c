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

static void create_taskbar_buttons(WM *wm);
static void create_icon_area(WM *wm);
static void create_status_area(WM *wm);
static void create_act_center(void);

void create_taskbar(WM *wm)
{
    Taskbar *b=wm->taskbar=malloc_s(sizeof(Taskbar));
    b->w=xinfo.screen_width, b->h=get_font_height_by_pad();
    b->x=0, b->y=(cfg->taskbar_on_top ? 0 : xinfo.screen_height-b->h);
    b->win=create_widget_win(xinfo.root_win, b->x, b->y, b->w, b->h,
        0, 0, get_widget_color(TASKBAR_COLOR));
    XSelectInput(xinfo.display, b->win, CROSSING_MASK);
    create_taskbar_buttons(wm);
    create_status_area(wm);
    create_icon_area(wm);
    create_act_center();
    XMapSubwindows(xinfo.display, b->win);
    if(cfg->show_taskbar)
        XMapWindow(xinfo.display, b->win);
}

static void create_taskbar_buttons(WM *wm)
{
    Taskbar *b=wm->taskbar;
    int w=cfg->taskbar_button_width, h=b->h;

    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        unsigned long color=get_widget_color(is_chosen_button(wm,
            TASKBAR_BUTTON_BEGIN+i) ? CHOSEN_BUTTON_COLOR : TASKBAR_COLOR);
        b->buttons[i]=create_widget_win(b->win, w*i, 0, w, h, 0, 0, color);
        XSelectInput(xinfo.display, b->buttons[i], BUTTON_EVENT_MASK);
    }
}

static void create_icon_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    int bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N,
        w=b->w-bw-b->status_area_w;
    b->icon_area=create_widget_win(b->win,
        bw, 0, w, b->h, 0, 0, get_widget_color(TASKBAR_COLOR));
}

static void create_status_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    b->status_text=get_text_prop(xinfo.root_win, XA_WM_NAME);
    if(!b->status_text)
        b->status_text=copy_string("gwm");
    get_string_size(b->status_text, &b->status_area_w, NULL);
    if(b->status_area_w > cfg->status_area_width_max)
        b->status_area_w=cfg->status_area_width_max;
    else if(b->status_area_w == 0)
        b->status_area_w=1;
    wm->taskbar->status_area=create_widget_win(b->win,
        b->w-b->status_area_w, 0, b->status_area_w, b->h,
        0, 0, get_widget_color(TASKBAR_COLOR));
    XSelectInput(xinfo.display, b->status_area, ExposureMask);
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
    Window win=wm->taskbar->buttons[WIDGET_INDEX(type, TASKBAR_BUTTON)];
    unsigned long color=get_widget_color(is_chosen_button(wm, type) ?
        CHOSEN_BUTTON_COLOR : TASKBAR_COLOR);

    if(IS_WIDGET_CLASS(type, DESKTOP_BUTTON))
    {
        unsigned int desktop_n=WIDGET_INDEX(type, DESKTOP_BUTTON)+1;
        if(desktop_n != wm->cur_desktop)
        {
            if(have_urgency(wm, desktop_n))
                color=get_widget_color(URGENCY_WIDGET_COLOR);
            else if(have_attention(wm, desktop_n))
                color=get_widget_color(ATTENTION_WIDGET_COLOR);
        }
    }
    update_win_bg(win, color, None);
}

void update_taskbar_button_fg(WM *wm, Widget_type type)
{
    size_t i=WIDGET_INDEX(type, TASKBAR_BUTTON);
    Str_fmt f={0, 0, cfg->taskbar_button_width, wm->taskbar->h, CENTER,
        false, false, 0, get_text_color(TASKBAR_TEXT_COLOR)};
    draw_string(wm->taskbar->buttons[i], cfg->taskbar_button_text[i], &f);
}

void update_status_area_fg(WM *wm)
{
    Taskbar *b=wm->taskbar;
    Str_fmt f={0, 0, b->status_area_w, b->h, CENTER_RIGHT, false, false, 0,
        get_text_color(TASKBAR_TEXT_COLOR)};
    draw_string(b->status_area, b->status_text, &f);
}

void update_icon_status_area(WM *wm)
{
    int w, bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N;
    Taskbar *b=wm->taskbar;

    get_string_size(b->status_text, &w, NULL);
    if(w > cfg->status_area_width_max)
        w=cfg->status_area_width_max;
    if(w != b->status_area_w)
    {
        XMoveResizeWindow(xinfo.display, b->status_area, b->w-w, 0, w, b->h);
        XMoveResizeWindow(xinfo.display, b->icon_area, bw, 0, b->w-bw-w, b->h);
    }
    b->status_area_w=w;
    update_status_area_fg(wm);
}

void update_client_icon_fg(WM *wm, Window win)
{
    Client *c=win_to_iconic_state_client(wm, win);
    if(!c)
        return;

    Icon *i=c->icon;
    draw_icon(c->icon->win, c->image, c->class_name, wm->taskbar->h);
    if(i->show_text)
    {
        Str_fmt f={wm->taskbar->h, 0, i->w-wm->taskbar->h, i->h, CENTER,
            false, false, 0, get_text_color(TASKBAR_TEXT_COLOR)};
        draw_string(i->win, i->title_text, &f);
    }
}
