/* *************************************************************************
 *     listview.c：實現清單顯示功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "font.h"
#include "gwm.h"
#include "listview.h"

struct _list_view_tag // 清單顯示構件
{
    Widget base;
    const Strings *texts;
    int nmax;
};

static int get_strings_width(const Strings *texts);
static int get_strings_height(const Strings *texts);
static void set_list_view_method(Widget *widget);

List_view *create_list_view(Widget *parent, Widget_id id, int x, int y, int w, int h, const Strings *texts)
{
    if((!texts || list_is_empty(&texts->list)) && (w<=0 || h<=0))
        return NULL;

    List_view *list_view=Malloc(sizeof(List_view));
    w = w>0 ? w : get_strings_width(texts);
    h = h>0 ? h : get_strings_height(texts);
    init_widget(WIDGET(list_view), parent, id, WIDGET_STATE_NORMAL, x, y, w, h);
    XSelectInput(xinfo.display, WIDGET_WIN(list_view), None);
    set_list_view_method(WIDGET(list_view));
    list_view->texts=texts;
    list_view->nmax=INT_MAX;

    return list_view;
}

static int get_strings_width(const Strings *texts)
{
    int wmax=0, w=0;

    list_for_each_entry(Strings, s, &texts->list, list)
    {
        get_string_size(s->str, &w, NULL);
        if(w > wmax)
            wmax=w;
    }

    return wmax;
}

static int get_strings_height(const Strings *texts)
{
    int n=0, h=get_font_height_by_pad();
    list_for_each_entry(Strings, s, &texts->list, list)
        n++;
    return h*n;
}

static void set_list_view_method(Widget *widget)
{
    widget->show=show_list_view;
    widget->update_fg=update_list_view_fg;
}

void show_list_view(Widget *widget)
{
    XRaiseWindow(xinfo.display, WIDGET_WIN(widget));
    show_widget(widget);
}

void update_list_view_fg(const Widget *widget)
{
    List_view *list_view=LIST_VIEW(widget);
    if(!list_view->texts)
        return;
    
    Window win=WIDGET_WIN(list_view);
    int w=WIDGET_W(list_view), h=get_font_height_by_pad(),
        i=0, nmax=list_view->nmax;
    Str_fmt fmt={0, 0, w, h, CENTER_LEFT, true, false, 0,
        get_widget_fg(WIDGET_STATE_NORMAL)};

    list_for_each_entry(Strings, s, &list_view->texts->list, list)
    {
        if(i < nmax)
            draw_string(win, i<nmax-1 ? s->str : "...", &fmt), fmt.y+=h;
        else
            break;
        i++;
    }
}

void update_list_view(List_view *list_view, Strings *texts)
{
    int w=WIDGET_W(list_view), hl=get_font_height_by_pad(),
        n=list_count_nodes(&texts->list), h=MIN(n, list_view->nmax)*hl;

    list_view->texts=texts;
    resize_widget(WIDGET(list_view), w, h);
    show_list_view(WIDGET(list_view));
    update_list_view_fg(WIDGET(list_view));
}

void set_list_view_nmax(List_view *list_view, int nmax)
{
    list_view->nmax=nmax;
}
