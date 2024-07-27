/* *************************************************************************
 *     button.c：實現按鈕相關功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <Imlib2.h>
#include "font.h"
#include "image.h"
#include "widget.h"
#include "button.h"

#define BUTTON_EVENT_MASK (BUTTON_MASK|ExposureMask|CROSSING_MASK)

struct _button_tag
{
    Widget base;
    Imlib_Image image;
    char *icon_name;
    char *symbol;
    char *label;
    Align_type align; // 標籤的對齊方式
};

static void set_button_method(Widget *widget);

Button *create_button(Widget *parent, Widget_id id, Widget_state state, int x, int y, int w, int h, const char *label)
{
    Button *button=Malloc(sizeof(Button));
    init_widget(WIDGET(button), parent, id, state, x, y, w, h);
    set_button_method(WIDGET(button));

    button->image=button->icon_name=button->symbol=NULL;
    button->label=copy_string(label);
    button->align=CENTER;
    XSelectInput(xinfo.display, WIDGET_WIN(button), BUTTON_EVENT_MASK);

    return button;
}

static void set_button_method(Widget *widget)
{
    widget->update_fg=update_button_fg;
}

void destroy_button(Button *button)
{
    destroy_widget(WIDGET(button));
    vfree(button->icon_name, button->symbol, button->label);
    vset_null(button->icon_name, button->symbol, button->label);
}

void update_button_fg(const Widget *widget)
{
    Button *button=BUTTON(widget);
    XftColor fg=get_widget_fg(WIDGET_STATE(button));
    int xi=0, y=0, h=WIDGET_H(button), wi=h, xl=wi, wl=WIDGET_H(button)-wi;

    if(button->image)
        draw_image(button->image, WIDGET_WIN(button), xi, y, wi, h);
    else if(button->symbol)
    {
        Str_fmt fmt={xi, y, wi, h, CENTER, false, false, 0, fg};
        draw_string(WIDGET_WIN(button), button->symbol, &fmt);
    }
    else if(button->icon_name)
    {
        char s[2]={button->icon_name[0], '\0'};
        Str_fmt fmt={xi, y, wi, h, CENTER, false, false, 0, fg};
        draw_string(WIDGET_WIN(button), s, &fmt);
    }
    else
        xl=0, wl=WIDGET_W(button);

    if(button->label)
    {
        Str_fmt fmt={xl, y, wl, h, button->align, true, false, 0, fg};
        draw_string(WIDGET_WIN(button), button->label, &fmt);
    }
}

void set_button_icon(Button *button, Imlib_Image image, const char *icon_name, const char *symbol)
{
    if(image)
        button->image=image;
    else if(icon_name)
        button->icon_name=copy_string(icon_name);
    else if(symbol)
        button->symbol=copy_string(symbol);
    else
        return;
}

void change_button_icon(Button *button, Imlib_Image image, const char *icon_name, const char *symbol)
{
    if(icon_name && icon_name!=button->icon_name)
        Free(button->icon_name);
    else if(symbol && symbol!=button->symbol)
        Free(button->symbol);
    else
        return;
    set_button_icon(button, image, icon_name, symbol);
}

char *get_button_label(Button *button)
{
    return button->label;
}

void set_button_label(Button *button, const char *label)
{
    if(button->label)
        Free(button->label);
    button->label=copy_string(label);
}

void set_button_align(Button *button, Align_type align)
{
    button->align=align;
}
