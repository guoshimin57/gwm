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
static void create_act_center(WM *wm);
static void draw_client_icon(WM *wm, Client *c);

void create_taskbar(WM *wm)
{
    Taskbar *b=wm->taskbar=malloc_s(sizeof(Taskbar));
    b->w=wm->screen_width, b->h=TASKBAR_HEIGHT(wm);
    b->x=0, b->y=(wm->cfg->taskbar_on_top ? 0 : wm->screen_height-b->h);
    b->win=create_widget_win(wm, wm->root_win, b->x, b->y, b->w, b->h,
        0, 0, WIDGET_COLOR(wm, TASKBAR));
    XSelectInput(wm->display, b->win, CROSSING_MASK);
    set_net_wm_win_type(wm, b->win, NET_WM_WINDOW_TYPE_DOCK);

    Window wins[]={wm->top_wins[DOCK_TOP], b->win};
    XRestackWindows(wm->display, wins, 2);
    create_taskbar_buttons(wm);
    create_status_area(wm);
    create_icon_area(wm);
    create_act_center(wm);
    XMapSubwindows(wm->display, b->win);
    if(wm->cfg->show_taskbar)
        XMapWindow(wm->display, b->win);
}

static void create_taskbar_buttons(WM *wm)
{
    Taskbar *b=wm->taskbar;
    int w=wm->cfg->taskbar_button_width, h=b->h;

    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        b->buttons[i]=create_widget_win(wm, b->win, w*i, 0, w, h, 0, 0,
            NCHOSEN_BUTTON_COLOR(wm, TASKBAR_BUTTON_BEGIN+i, TASKBAR_COLOR));
        XSelectInput(wm->display, b->buttons[i], BUTTON_EVENT_MASK);
    }
}

static void create_icon_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    int bw=wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N,
        w=b->w-bw-b->status_area_w;
    b->icon_area=create_widget_win(wm, b->win,
        bw, 0, w, b->h, 0, 0, WIDGET_COLOR(wm, TASKBAR));
}

static void create_status_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    b->status_text=get_text_prop(wm, wm->root_win, XA_WM_NAME);
    if(!b->status_text)
        b->status_text=copy_string("gwm");
    get_string_size(wm, wm->font[TASKBAR_FONT], b->status_text, &b->status_area_w, NULL);
    if(b->status_area_w > wm->cfg->status_area_width_max)
        b->status_area_w=wm->cfg->status_area_width_max;
    else if(b->status_area_w == 0)
        b->status_area_w=1;
    wm->taskbar->status_area=create_widget_win(wm, b->win,
        b->w-b->status_area_w, 0, b->status_area_w, b->h,
        0, 0, WIDGET_COLOR(wm, TASKBAR));
    XSelectInput(wm->display, b->status_area, ExposureMask);
}

static void create_act_center(WM *wm)
{
    wm->act_center=create_menu(wm, wm->cfg->act_center_item_text,
        ACT_CENTER_ITEM_N, wm->cfg->act_center_col);
    set_net_wm_win_type(wm, wm->act_center->win, NET_WM_WINDOW_TYPE_DROPDOWN_MENU);
}

void update_taskbar_buttons_bg(WM *wm)
{
    for(Widget_type t=TASKBAR_BUTTON_BEGIN; t<=TASKBAR_BUTTON_END; t++)
        update_taskbar_button_bg(wm, t);
}

void update_taskbar_button_bg(WM *wm, Widget_type type)
{
    Window win=wm->taskbar->buttons[WIDGET_INDEX(type, TASKBAR_BUTTON)];
    update_win_bg(wm, win, NCHOSEN_BUTTON_COLOR(wm, type, TASKBAR_COLOR), None);
}

void update_taskbar_button_fg(WM *wm, Widget_type type)
{
    size_t i=WIDGET_INDEX(type, TASKBAR_BUTTON);
    String_format f={{0, 0, wm->cfg->taskbar_button_width, wm->taskbar->h},
        CENTER, true, false, false, NCHOSEN_BUTTON_COLOR(wm, type, TASKBAR_COLOR),
        TEXT_COLOR(wm, TASKBAR), TASKBAR_FONT};
    draw_string(wm, wm->taskbar->buttons[i], wm->cfg->taskbar_button_text[i], &f);
}

void update_status_area_fg(WM *wm)
{
    Taskbar *b=wm->taskbar;
    String_format f={{0, 0, b->status_area_w, b->h}, CENTER_RIGHT, true,
        false, false, 0, TEXT_COLOR(wm, TASKBAR), TASKBAR_FONT};
    draw_string(wm, b->status_area, b->status_text, &f);
}

void update_icon_status_area(WM *wm)
{
    int w, bw=wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N;
    Taskbar *b=wm->taskbar;

    get_string_size(wm, wm->font[TASKBAR_FONT], b->status_text, &w, NULL);
    if(w > wm->cfg->status_area_width_max)
        w=wm->cfg->status_area_width_max;
    if(w != b->status_area_w)
    {
        XMoveResizeWindow(wm->display, b->status_area, b->w-w, 0, w, b->h);
        XMoveResizeWindow(wm->display, b->icon_area, bw, 0, b->w-bw-w, b->h);
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
    draw_client_icon(wm, c);
    if(i->show_text)
    {
        String_format f={{wm->taskbar->h, 0, i->w-wm->taskbar->h, i->h},
            CENTER, true, false, false, 0, 
            TEXT_COLOR(wm, TASKBAR), TASKBAR_FONT};
        draw_string(wm, i->win, i->title_text, &f);
    }
}

static void draw_client_icon(WM *wm, Client *c)
{
    int size=wm->taskbar->h;
    if(c->image)
        draw_image(wm, c->image, c->icon->win, 0, 0, size, size);
    else
    {
        String_format f={{0, 0, size, size}, CENTER, true, false, false, 0,
            TEXT_COLOR(wm, CLASS), CLASS_FONT};
        draw_string(wm, c->icon->win, c->class_name, &f);
    }
}
