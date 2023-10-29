/* *************************************************************************
 *     color.c：實現分配顏色的功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void alloc_widget_color(WM *wm, const char *color_name, XColor *color);
static void alloc_text_color(WM *wm, const char *color_name, XftColor *color);
static void update_taskbar_bg(WM *wm);
static void update_act_center_bg(WM *wm);
static void update_run_cmd_bg(WM *wm);
static void update_client_menu_bg(WM *wm);
static void update_clients_bg(WM *wm);

static void alloc_widget_color(WM *wm, const char *color_name, XColor *color)
{
    XParseColor(wm->display, wm->colormap, color_name, color); 
    XAllocColor(wm->display, wm->colormap, color);
}

static void alloc_text_color(WM *wm, const char *color_name, XftColor *color)
{
    XftColorAllocName(wm->display, wm->visual, wm->colormap, color_name, color);
}

void alloc_color(WM *wm)
{
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Widget_color j=0; j<WIDGET_COLOR_N; j++)
            alloc_widget_color(wm, wm->cfg->widget_color_name[i][j],
                &wm->widget_color[i][j]);
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Text_color j=0; j<TEXT_COLOR_N; j++)
            alloc_text_color(wm, wm->cfg->text_color_name[i][j],
                &wm->text_color[i][j]);
}

void update_widget_bg(WM *wm)
{
    update_taskbar_bg(wm);
    update_act_center_bg(wm);
    update_run_cmd_bg(wm);
    update_win_bg(wm, wm->hint_win, WIDGET_COLOR(wm, HINT_WIN), None);
    update_client_menu_bg(wm);
    update_clients_bg(wm);
}

static void update_taskbar_bg(WM *wm)
{
    unsigned long bg=WIDGET_COLOR(wm, TASKBAR);
    update_taskbar_buttons_bg(wm);
    update_win_bg(wm, wm->taskbar->icon_area, bg, None);
    /* Xlib手冊說窗口收到Expose事件時會更新背景，但事實上不知道爲何，上邊的語句
     * 雖然給icon_area發送了Expose事件，但實際上沒更新背景。也許當窗口沒有內容
     * 時，收到Expose事件並不會更新背景。故只好調用本函數強制更新背景。 */
    XClearWindow(wm->display, wm->taskbar->icon_area);
    update_win_bg(wm, wm->taskbar->status_area, bg, None);
}

static void update_act_center_bg(WM *wm)
{
    unsigned long bg=WIDGET_COLOR(wm, MENU);
    update_win_bg(wm, wm->act_center->win, bg, None);
    for(size_t i=0; i<ACT_CENTER_ITEM_N; i++)
        update_win_bg(wm, wm->act_center->items[i], bg,None);
}

static void update_run_cmd_bg(WM *wm)
{
    Window win=wm->run_cmd->win;
    update_win_bg(wm, win, WIDGET_COLOR(wm, ENTRY), None);
    XSetWindowBorder(wm->display, win, WIDGET_COLOR(wm, CURRENT_BORDER));
}

static void update_client_menu_bg(WM *wm)
{
    unsigned long bg=WIDGET_COLOR(wm, MENU);
    update_win_bg(wm, wm->client_menu->win, bg, None);
    for(size_t i=0; i<CLIENT_MENU_ITEM_N; i++)
        update_win_bg(wm, wm->client_menu->items[i], bg, None);
}

static void update_clients_bg(WM *wm)
{
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        update_client_bg(wm, wm->cur_desktop, c);
}

void update_client_bg(WM *wm, unsigned int desktop_n, Client *c)
{
    if(!c || c==wm->clients)
        return;

    Desktop *d=wm->desktop[desktop_n-1];
    if(c->icon && d->cur_layout!=PREVIEW)
        update_win_bg(wm, c->icon->win, c==d->cur_focus_client ?
            WIDGET_COLOR(wm, ENTERED_NORMAL_BUTTON) :
            WIDGET_COLOR(wm, TASKBAR), None);
    else
        update_frame_bg(wm, desktop_n, c);
}

void update_frame_bg(WM *wm, unsigned int desktop_n, Client *c)
{
    bool cur=(c==wm->desktop[desktop_n-1]->cur_focus_client);
    unsigned long color=NCUR_WIDGET_COLOR(wm, cur, BORDER);

    if(c->border_w)
        XSetWindowBorder(wm->display, c->frame, color);
    if(c->titlebar_h)
    {
        cur=(cur && is_focusable(wm, c));
        color=NCUR_WIDGET_COLOR(wm, cur, TITLEBAR);
        update_win_bg(wm, c->logo, color, 0);
        update_win_bg(wm, c->title_area, color, 0);
        for(size_t i=0; i<TITLE_BUTTON_N; i++)
            update_win_bg(wm, c->buttons[i], color, None);
    }
}

unsigned long get_widget_color(WM *wm, Widget_color wc)
{
    float wo=wm->cfg->widget_opacity[wm->cfg->color_theme][wc];
    unsigned long rgb=wm->widget_color[wm->cfg->color_theme][wc].pixel;

    return ((rgb & 0x00ffffff) | ((unsigned long)(0xff*wo))<<24);
}
