/* *************************************************************************
 *     menu.c：實現菜單功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "button.h"
#include "menu.h"

struct _menu_tag // 一級多行多列菜單 
{
    Widget base;
    Widget *owner;
    Button **items; // 菜單項
    int n, col, row; // 菜單項數量、列數、行數
};

static void menu_ctor(Menu *menu, Widget *owner, Widget_id id, const char *icon_names[], const char *symbols[], const char *labels[], int n, int col);
static bool is_null_strings(const char *strings[], int n);
static void menu_set_method(Widget *widget);
static void menu_dtor(Menu *menu);

Menu *act_center=NULL; // 操作中心

Menu *menu_new(Widget *owner, Widget_id id, const char *icon_names[], const char *symbols[], const char *labels[], int n, int col)
{
    Menu *menu=Malloc(sizeof(Menu));
    menu_ctor(menu, owner, id, icon_names, symbols, labels, n, col);
    return menu;
}

static void menu_ctor(Menu *menu, Widget *owner, Widget_id id, const char *icon_names[], const char *symbols[], const char *labels[], int n, int col)
{
    int w, wi=0, wl=0, h=get_font_height_by_pad(), maxw=0, sw=xinfo.screen_width,
        pad=get_font_pad(), row=(n+col-1)/col;

    if(!is_null_strings(icon_names, n) || !is_null_strings(symbols, n))
        wi=h;

    for(int i=0; i<n; i++, maxw = wl>maxw ? wl : maxw)
        get_string_size(labels[i], &wl, NULL);
    wl=maxw;
    
    w = wi+wl+(wl ? 2*pad : 0);
    if(w > sw)
        w=sw/col;

    widget_ctor(WIDGET(menu), NULL, id, WIDGET_STATE_1(current), 0, 0, w*col, h*row);
    menu_set_method(WIDGET(menu));

    menu->owner=owner;
    menu->items=Malloc(sizeof(Button *)*n);
    for(int i=0; i<n; i++)
    {
         menu->items[i]=button_new(WIDGET(menu), id+i+1, WIDGET_STATE_1(current), w*(i%col),
             h*(i/col), w, h, labels[i]);
         button_set_icon(menu->items[i], NULL, icon_names[i], symbols[i]);
         button_set_align(menu->items[i], CENTER_LEFT);
    }
    menu->n=n, menu->col=col, menu->row=row;
}

static bool is_null_strings(const char *strings[], int n)
{
    for(int i=0; i<n; i++)
        if(strings[i])
            return false;
    return true;
}

static void menu_set_method(Widget *widget)
{
    widget->show=menu_show;
    widget->update_bg=menu_update_bg;
}

void menu_del(Menu *menu)
{
    menu_dtor(menu);
    widget_del(WIDGET(menu));
}

static void menu_dtor(Menu *menu)
{
    for(int i=0; i<menu->n; i++)
        button_del(menu->items[i]), menu->items[i]=NULL;
    Free(menu->items);
}

void menu_show(Widget *widget)
{
    Menu *menu=MENU(widget);

    set_pos_for_click(WIDGET_WIN(menu->owner),
        &WIDGET_X(menu), &WIDGET_Y(menu), WIDGET_W(menu), WIDGET_H(menu));
    widget_move_resize(WIDGET(menu), WIDGET_X(menu), WIDGET_Y(menu), WIDGET_W(menu), WIDGET_H(menu));
    XRaiseWindow(xinfo.display, WIDGET_WIN(menu));
    widget_show(widget);
}

void menu_update_bg(const Widget *widget)
{
    Menu *menu=MENU(widget);

    widget_update_bg(widget);
    for(int i=0; i<menu->n; i++)
        widget_update_bg(WIDGET(menu->items[i]));
}
