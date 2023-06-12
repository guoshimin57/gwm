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

void update_widget_color(WM *wm)
{
    update_taskbar_buttons(wm);
    update_win_bg(wm, wm->taskbar->icon_area, WIDGET_COLOR(wm, TASKBAR), None);
    /* Xlib手冊說窗口收到Expose事件時會更新背景，但事實上不知道爲何，上邊的語句
     * 雖然給icon_area發送了Expose事件，但實際上沒更新背景。也許當窗口沒有內容
     * 時，收到Expose事件並不會更新背景。故只好調用本函數強制更新背景。 */
    XClearWindow(wm->display, wm->taskbar->icon_area);
    update_win_bg(wm, wm->taskbar->status_area, WIDGET_COLOR(wm, TASKBAR), None);
    update_win_bg(wm, wm->act_center->win, WIDGET_COLOR(wm, ACT_CENTER), None);
    for(size_t i=0; i<ACT_CENTER_ITEM_N; i++)
        update_win_bg(wm, wm->act_center->items[i],
            WIDGET_COLOR(wm, ACT_CENTER) ,None);
    update_win_bg(wm, wm->hint_win, WIDGET_COLOR(wm, HINT_WIN), None);
    update_win_bg(wm, wm->run_cmd->win, WIDGET_COLOR(wm, ENTRY), None);
    XSetWindowBorder(wm->display, wm->run_cmd->win,
        WIDGET_COLOR(wm, CURRENT_BORDER));
    for(Client *c=wm->clients->next; c!=wm->clients; c=c->next)
        update_client_look(wm, wm->cur_desktop, c);
}
