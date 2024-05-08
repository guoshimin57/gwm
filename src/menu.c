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

#include "gwm.h"

struct _menu_tag // 一級多行多列菜單 
{
    Widget base;
    Button **items; // 菜單項
    int n, col, row; // 菜單項數量、列數、行數
};

static bool is_null_strings(const char *strings[], int n);

Menu *act_center=NULL, *client_menu=NULL; // 操作中心, 客戶窗口菜單

Menu *create_menu(Widget_id id, Window parent, const char *icon_names[], const char *symbols[], const char *labels[], int n, int col)
{
    Menu *menu=malloc_s(sizeof(Menu));

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

    init_widget(WIDGET(menu), id, UNUSED_TYPE, WIDGET_STATE_1(current), parent, 0, 0, w*col, h*row);

    menu->items=malloc_s(sizeof(Button *)*n);
    for(int i=0; i<n; i++)
    {
         menu->items[i]=create_button(id+i+1, WIDGET_STATE_1(current), WIDGET_WIN(menu), w*(i%col),
             h*(i/col), w, h, labels[i]);
         set_button_icon(menu->items[i], NULL, icon_names[i], symbols[i]);
         set_button_align(menu->items[i], CENTER_LEFT);
    }

    menu->n=n, menu->col=col, menu->row=row;

    return menu;
}

static bool is_null_strings(const char *strings[], int n)
{
    for(int i=0; i<n; i++)
        if(strings[i])
            return false;
    return true;
}

void show_menu(XEvent *e, Menu *menu, Window bind)
{
    if(e->type == ButtonPress)
        set_pos_for_click(bind, e->xbutton.x_root-e->xbutton.x,
            &WIDGET_X(menu), &WIDGET_Y(menu), WIDGET_W(menu), WIDGET_H(menu));
    move_resize_widget(WIDGET(menu), WIDGET_X(menu), WIDGET_Y(menu), WIDGET_W(menu), WIDGET_H(menu));
    XRaiseWindow(xinfo.display, WIDGET_WIN(menu));
    show_widget(WIDGET(menu));
}

void update_menu_bg(const Menu *menu)
{
    update_widget_bg(WIDGET(menu));
    for(int i=0; i<menu->n; i++)
        update_widget_bg(WIDGET(menu->items[i]));
}

void destroy_menu(Menu *menu)
{
    for(int i=0; i<menu->n; i++)
        destroy_button(menu->items[i]);
    free_s(menu->items);
    XDestroyWindow(xinfo.display, WIDGET_WIN(menu));
    free_s(menu);
}
