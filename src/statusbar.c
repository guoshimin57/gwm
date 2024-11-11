/* *************************************************************************
 *     statusbar.c：實現狀態欄相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "font.h"
#include "widget.h"
#include "statusbar.h"

struct statusbar_tag // 狀態欄
{
    Widget base;
    char *label;
};

static void statusbar_ctor(Statusbar *statusbar, Widget *parent, int x, int y, int w, int h, const char *label);
static void statusbar_set_method(Widget *widget);
static void statusbar_dtor(Statusbar *statusbar);
static void statusbar_update_fg(const Widget *widget);

Statusbar *statusbar_new(Widget *parent, int x, int y, int w, int h, const char *label)
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

void statusbar_del(Statusbar *statusbar)
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
    XftColor fg=get_widget_fg(WIDGET_STATE(statusbar));
    if(statusbar->label)
    {
        Str_fmt fmt={0, 0, WIDGET_W(statusbar),
            WIDGET_H(statusbar), CENTER, true, false, 0, fg};
        draw_string(WIDGET_WIN(statusbar), statusbar->label, &fmt);
    }
}

void statusbar_change_label(Statusbar *statusbar, const char *label)
{
    int x=WIDGET_X(statusbar), y=WIDGET_Y(statusbar),
        w=WIDGET_W(statusbar), h=WIDGET_H(statusbar), nw=0;

    Free(statusbar->label);
    statusbar->label=copy_string(label);
    get_string_size(label, &nw, NULL);
    nw += 2*get_font_pad();
    if(nw > cfg->statusbar_width_max)
        nw=cfg->statusbar_width_max;
    if(nw != w)
        widget_move_resize(WIDGET(statusbar), x+w-nw, y, nw, h);
    statusbar_update_fg(WIDGET(statusbar));
}
