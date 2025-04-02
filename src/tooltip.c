/* *************************************************************************
 *     tooltip.c：實現構件功能提示的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "drawable.h"
#include "font.h"
#include "gwm.h"
#include "misc.h"
#include "tooltip.h"

struct _tooltip_tag // 提示工具
{
    Widget base;
    const Widget *owner;
    char *tip;
};

static void tooltip_ctor(Tooltip *tooltip, const Widget *owner, const char *tip);
static void tooltip_set_method(Widget *widget);
static void tooltip_dtor(Tooltip *tooltip);

Tooltip *tooltip_new(const Widget *owner, const char *tip)
{
    if(tip == NULL)
        return NULL;

    Tooltip *tooltip=Malloc(sizeof(Tooltip));
    tooltip_ctor(tooltip, owner, tip);

    return tooltip;
}

static void tooltip_ctor(Tooltip *tooltip, const Widget *owner, const char *tip)
{
    int x=0, y=0, w=0, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(tip, &w, NULL);
    w+=pad*2;
    widget_ctor(WIDGET(tooltip), NULL, WIDGET_TYPE_TOOLTIP, UNUSED_WIDGET_ID, x, y, w, h);
    tooltip_set_method(WIDGET(tooltip));
    tooltip->owner=owner;
    tooltip->tip=copy_string(tip);
}

static void tooltip_set_method(Widget *widget)
{
    widget->del=tooltip_del;
    widget->show=tooltip_show;
    widget->update_fg=tooltip_update_fg;
}

void tooltip_change_tip(Tooltip *tooltip, const char *tip)
{
    Free(tooltip->tip);
    tooltip->tip=copy_string(tip);

    int *pw=&WIDGET_W(tooltip), h=WIDGET_H(tooltip), pad=get_font_pad();
    get_string_size(tip, pw, NULL);
    *pw+=pad*2;
    XResizeWindow(xinfo.display, WIDGET_WIN(tooltip), *pw, h);
}

void tooltip_del(Widget *widget)
{
    Tooltip *tooltip=TOOLTIP(widget);
    tooltip_dtor(tooltip);
    widget_del(widget);
}

static void tooltip_dtor(Tooltip *tooltip)
{
    Free(tooltip->tip);
}

void tooltip_show(Widget *widget)
{
    Tooltip *t=TOOLTIP(widget);
    int *px=&WIDGET_X(t), *py=&WIDGET_Y(t), w=WIDGET_W(t), h=WIDGET_H(t);
    set_popup_pos(t->owner, true, px, py, w, h);
    XMoveWindow(xinfo.display, WIDGET_WIN(t), *px, *py);
    XRaiseWindow(xinfo.display, WIDGET_WIN(t));
    widget_show(widget);
}

void tooltip_update_fg(const Widget *widget)
{
    Str_fmt f={0, 0, WIDGET_W(widget), WIDGET_H(widget), CENTER, true, false, 0,
        get_text_color(widget)};
    draw_string(WIDGET_WIN(widget), TOOLTIP(widget)->tip, &f);
}
