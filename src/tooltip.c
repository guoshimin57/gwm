/* *************************************************************************
 *     tooltip.c：實現構件功能提示的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "memory.h"

struct _tooltip_tag // 提示工具
{
    Widget base;
    Widget *owner;
    char *tip;
};

static void set_tooltip_method(Widget *widget);

Tooltip *create_tooltip(Widget *owner, const char *tip)
{
    if(tip == NULL)
        return NULL;

    Tooltip *tooltip=Malloc(sizeof(Tooltip));
    int x=0, y=0, w=0, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(tip, &w, NULL);
    w+=pad*2;
    init_widget(WIDGET(tooltip), NULL, UNUSED_WIDGET_ID, WIDGET_STATE_1(current), x, y, w, h);
    set_tooltip_method(WIDGET(tooltip));

    tooltip->tip=copy_string(tip);
    tooltip->owner=owner;
    owner->tooltip=WIDGET(tooltip);

    return tooltip;
}

static void set_tooltip_method(Widget *widget)
{
    widget->show=show_tooltip;
    widget->update_fg=update_tooltip_fg;
}

void change_tooltip_tip(Tooltip *tooltip, const char *tip)
{
    Free(tooltip->tip);
    tooltip->tip=copy_string(tip);

    int *pw=&WIDGET_W(tooltip), h=WIDGET_H(tooltip), pad=get_font_pad();
    get_string_size(tip, pw, NULL);
    *pw+=pad*2;
    XResizeWindow(xinfo.display, WIDGET_WIN(tooltip), *pw, h);
}

void destroy_tooltip(Tooltip *tooltip)
{
    tooltip->owner->tooltip=NULL;
    Free(tooltip->tip);
    destroy_widget(WIDGET(tooltip));
}

void show_tooltip(Widget *widget)
{
    Tooltip *t=TOOLTIP(widget);
    int *px=&WIDGET_X(t), *py=&WIDGET_Y(t), w=WIDGET_W(t), h=WIDGET_H(t);
    set_pos_for_click(WIDGET_WIN(t->owner), px, py, w, h);
    XMoveWindow(xinfo.display, WIDGET_WIN(t), *px, *py);
    XRaiseWindow(xinfo.display, WIDGET_WIN(t));
    show_widget(widget);
}

void update_tooltip_fg(const Widget *widget)
{
    Str_fmt f={0, 0, WIDGET_W(widget), WIDGET_H(widget), CENTER, true, false, 0,
        get_widget_fg(WIDGET_STATE_NORMAL)};
    draw_string(WIDGET_WIN(widget), TOOLTIP(widget)->tip, &f);
}
