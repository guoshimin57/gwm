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

void create_taskbar(WM *wm)
{
    Taskbar *b=wm->taskbar=malloc_s(sizeof(Taskbar));
    b->w=wm->screen_width, b->h=get_font_height_by_pad(wm, TASKBAR_BUTTON_FONT);
    b->x=0, b->y=(wm->cfg->taskbar_on_top ? 0 : wm->screen_height-b->h);
    b->win=XCreateSimpleWindow(wm->display, wm->root_win, b->x, b->y,
        b->w, b->h, 0, 0, 0);
    set_override_redirect(wm, b->win);
    XSelectInput(wm->display, b->win, CROSSING_MASK);
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
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        unsigned long color = is_chosen_button(wm, TASKBAR_BUTTON_BEGIN+i) ?
            wm->widget_color[wm->cfg->color_theme][CHOSEN_TASKBAR_BUTTON_COLOR].pixel :
            wm->widget_color[wm->cfg->color_theme][NORMAL_TASKBAR_BUTTON_COLOR].pixel ;
        b->buttons[i]=XCreateSimpleWindow(wm->display, b->win,
            wm->cfg->taskbar_button_width*i, 0, wm->cfg->taskbar_button_width,
            get_font_height_by_pad(wm, TASKBAR_BUTTON_FONT), 0, 0, color);
        XSelectInput(wm->display, b->buttons[i], BUTTON_EVENT_MASK);
    }
}

static void create_icon_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    int bw=wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N,
        w=b->w-bw-b->status_area_w;
    b->icon_area=XCreateSimpleWindow(wm->display, b->win,
        bw, 0, w, b->h, 0, 0, wm->widget_color[wm->cfg->color_theme][ICON_AREA_COLOR].pixel);
}

static void create_status_area(WM *wm)
{
    Taskbar *b=wm->taskbar;
    b->status_text=get_text_prop(wm, wm->root_win, XA_WM_NAME);
    if(!b->status_text)
        b->status_text=copy_string("gwm");
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

static void create_act_center(WM *wm)
{
    int n=ACT_CENTER_ITEM_N, col=wm->cfg->act_center_col,
        w=wm->cfg->act_center_item_width, pad=get_font_pad(wm, ACT_CENTER_FONT),
        h=get_font_height_by_pad(wm, ACT_CENTER_FONT);
    unsigned long color=wm->widget_color[wm->cfg->color_theme][ACT_CENTER_COLOR].pixel;

    wm->act_center=create_menu(wm, n, col, w, h, pad, color);
}

void update_taskbar_button(WM *wm, Widget_type type, bool change_bg)
{
    size_t i=TASKBAR_BUTTON_INDEX(type);
    String_format f={{0, 0, wm->cfg->taskbar_button_width,
        get_font_height_by_pad(wm, TASKBAR_BUTTON_FONT)},
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
    int w, bw=wm->cfg->taskbar_button_width*TASKBAR_BUTTON_N;
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
        int w=get_icon_draw_width(wm, c);
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

void update_act_center_button_text(WM *wm, size_t index)
{
    Window win=wm->act_center->items[index];
    int h=get_font_height_by_pad(wm, ACT_CENTER_FONT),
        pad=get_font_pad(wm, ACT_CENTER_FONT);
    String_format f={{pad, 0, wm->cfg->act_center_item_width-pad, h},
        CENTER_LEFT, false, 0,
        wm->text_color[wm->cfg->color_theme][ACT_CENTER_ITEM_TEXT_COLOR],
        ACT_CENTER_FONT};

    XClearArea(wm->display, win, 0, 0, pad, h, False); 
    draw_string(wm, win, wm->cfg->act_center_item_text[index], &f);
}
