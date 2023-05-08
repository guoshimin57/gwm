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
static void create_cmd_center(WM *wm);

void create_taskbar(WM *wm)
{
    Taskbar *b=wm->taskbar=malloc_s(sizeof(Taskbar));
    b->x=0, b->y=wm->screen_height-wm->cfg->taskbar_height;
    b->w=wm->screen_width, b->h=wm->cfg->taskbar_height;
    b->win=XCreateSimpleWindow(wm->display, wm->root_win, b->x, b->y,
        b->w, b->h, 0, 0, 0);
    set_override_redirect(wm, b->win);
    XSelectInput(wm->display, b->win, CROSSING_MASK);
    create_taskbar_buttons(wm);
    create_status_area(wm);
    create_icon_area(wm);
    create_cmd_center(wm);
    XMapRaised(wm->display, b->win);
    XMapWindow(wm->display, b->win);
    XMapSubwindows(wm->display, b->win);
}

static void create_taskbar_buttons(WM *wm)
{
    Taskbar *b=wm->taskbar;
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        unsigned long color = is_chosen_button(wm, TASKBAR_BUTTON_BEGIN+i) ?
            wm->widget_color[wm->cfg->color_theme][CHOSEN_TASKBAR_BUTTON_COLOR].pixel :
            wm->widget_color[wm->cfg->color_theme][NORMAL_TASKBAR_BUTTON_COLOR].pixel ;
        b->buttons[i]=XCreateSimpleWindow(wm->display, b->win,
            wm->cfg->taskbar_button_width*i, 0,
            wm->cfg->taskbar_button_width, wm->cfg->taskbar_button_height, 0, 0, color);
        XSelectInput(wm->display, b->buttons[i], BUTTON_EVENT_MASK);
    }
}

static void create_icon_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    unsigned int bw=wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N,
        w=b->w-bw-b->status_area_w;
    b->icon_area=XCreateSimpleWindow(wm->display, b->win,
        bw, 0, w, b->h, 0, 0, wm->widget_color[wm->cfg->color_theme][ICON_AREA_COLOR].pixel);
}

static void create_status_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    b->status_text=get_text_prop(wm, wm->root_win, XA_WM_NAME);
    get_string_size(wm, wm->font[STATUS_AREA_FONT], b->status_text, &b->status_area_w, NULL);
    if(b->status_area_w > wm->cfg->status_area_width_max)
        b->status_area_w=wm->cfg->status_area_width_max;
    else if(b->status_area_w == 0)
        b->status_area_w=1;
    wm->taskbar->status_area=XCreateSimpleWindow(wm->display, b->win,
        b->w-b->status_area_w, 0, b->status_area_w, b->h,
        0, 0, wm->widget_color[wm->cfg->color_theme][STATUS_AREA_COLOR].pixel);
    XSelectInput(wm->display, b->status_area, ExposureMask);
}

static void create_cmd_center(WM *wm)
{
    unsigned int n=CMD_CENTER_ITEM_N, col=wm->cfg->cmd_center_col,
        w=wm->cfg->cmd_center_item_width, h=wm->cfg->cmd_center_item_height;
    unsigned long color=wm->widget_color[wm->cfg->color_theme][CMD_CENTER_COLOR].pixel;

    wm->cmd_center=create_menu(wm, n, col, w, h, color);
}

void update_taskbar_button(WM *wm, Widget_type type, bool change_bg)
{
    size_t i=TASKBAR_BUTTON_INDEX(type);
    String_format f={{0, 0, wm->cfg->taskbar_button_width, wm->cfg->taskbar_button_height},
            CENTER, change_bg, is_chosen_button(wm, type) ?
            wm->widget_color[wm->cfg->color_theme][CHOSEN_TASKBAR_BUTTON_COLOR].pixel :
            wm->widget_color[wm->cfg->color_theme][NORMAL_TASKBAR_BUTTON_COLOR].pixel,
            wm->text_color[wm->cfg->color_theme][TASKBAR_BUTTON_TEXT_COLOR], TASKBAR_BUTTON_FONT};
    draw_string(wm, wm->taskbar->buttons[i], wm->cfg->taskbar_button_text[i], &f);
}

void update_status_area_text(WM *wm)
{
    Taskbar *b=wm->taskbar;
    String_format f={{0, 0, b->status_area_w, b->h}, CENTER_RIGHT, false, 0,
        wm->text_color[wm->cfg->color_theme][STATUS_AREA_TEXT_COLOR], STATUS_AREA_FONT};
    draw_string(wm, b->status_area, b->status_text, &f);
}

void hint_leave_taskbar_button(WM *wm, Widget_type type)
{
    unsigned long color = is_chosen_button(wm, type) ?
        wm->widget_color[wm->cfg->color_theme][CHOSEN_TASKBAR_BUTTON_COLOR].pixel :
        wm->widget_color[wm->cfg->color_theme][NORMAL_TASKBAR_BUTTON_COLOR].pixel ;
    Window win=wm->taskbar->buttons[TASKBAR_BUTTON_INDEX(type)];
    update_win_background(wm, win, color, None);
}

void update_status_area(WM *wm)
{
    unsigned int w, bw=wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N;
    Taskbar *b=wm->taskbar;
    get_string_size(wm, wm->font[STATUS_AREA_FONT], b->status_text, &w, NULL);
    if(w > wm->cfg->status_area_width_max)
        w=wm->cfg->status_area_width_max;
    if(w != b->status_area_w)
    {
        XMoveResizeWindow(wm->display, b->status_area, b->w-w, 0, w, b->h);
        XMoveResizeWindow(wm->display, b->icon_area, bw, 0, b->w-bw-w, b->h);
    }
    b->status_area_w=w;
    update_status_area_text(wm);
}

void update_icon_text(WM *wm, Window win)
{
    Client *c=win_to_iconic_state_client(wm, win);
    if(c)
    {
        Icon *i=c->icon;
        unsigned int w=get_icon_draw_width(wm, c);
        draw_icon(wm, c);
        if(!i->is_short_text)
        {
            String_format f={{w, 0, i->w, i->h}, CENTER_LEFT, false, 0,
                wm->text_color[wm->cfg->color_theme][c==CUR_FOC_CLI(wm) ? CURRENT_TITLE_TEXT_COLOR
                    : NORMAL_TITLE_TEXT_COLOR], TITLE_FONT};
            draw_string(wm, i->win, i->title_text, &f);
        }
    }
}

void update_cmd_center_button_text(WM *wm, size_t index)
{
    String_format f={{0, 0, wm->cfg->cmd_center_item_width, wm->cfg->cmd_center_item_height},
        CENTER_LEFT, false, 0, wm->text_color[wm->cfg->color_theme][CMD_CENTER_ITEM_TEXT_COLOR],
        CMD_CENTER_FONT};
    draw_string(wm, wm->cmd_center->items[index], wm->cfg->cmd_center_item_text[index], &f);
}

void handle_wm_icon_name_notify(WM *wm, Client *c, Window win)
{
    if(c && c->win==win && c->area_type==ICONIFY_AREA)
    {
        free(c->icon->title_text);
        c->icon->title_text=get_text_prop(wm, c->win, XA_WM_ICON_NAME);
        update_icon_area(wm);
    }
}