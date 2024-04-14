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

#include "gwm.h"

struct _button_tag
{
    Widget base;
    Imlib_Image image;
    char *icon_name;
    char *symbol;
    char *label;
    Align_type align; // 標籤的對齊方式
};

Button *create_button(Widget_id id, Widget_state state, Window parent, int x, int y, int w, int h, const char *label)
{
    Button *button=malloc_s(sizeof(Button));

    init_widget(WIDGET(button), id, BUTTON_TYPE, state, parent, x, y, w, h);

    button->image=button->icon_name=button->symbol=NULL;
    button->label=copy_string(label);
    button->align=CENTER;

    XSelectInput(xinfo.display, WIDGET_WIN(button), BUTTON_EVENT_MASK);

    return button;
}

void set_button_icon(Button *button, Imlib_Image image, const char *icon_name, const char *symbol)
{
    if(!image && !icon_name && !symbol)
        return;

    if(image)
        button->image=image;
    else if(icon_name)
        button->icon_name=copy_string(icon_name);
    else if(symbol)
        button->symbol=copy_string(symbol);
    else
        return;
}

char *get_button_label(Button *button)
{
    return button->label;
}

void set_button_label(Button *button, const char *label)
{
    if(button->label)
        free_s(button->label);
    button->label=copy_string(label);
}

void set_button_align(Button *button, Align_type align)
{
    button->align=align;
}

void destroy_button(Button *button)
{
    vfree(button->icon_name, button->symbol, button->label, NULL);
    destroy_widget(WIDGET(button));
}

void show_button(const Button *button)
{
    Widget *widget=WIDGET(button);
    show_widget(widget);
    if(widget->state.hot || widget->state.warn)
        update_hint_win_for_info(widget, widget->tooltip);
}

void update_button_fg(const Button *button)
{
    XftColor fg=get_widget_fg(get_widget_fg_id(WIDGET(button)));
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
