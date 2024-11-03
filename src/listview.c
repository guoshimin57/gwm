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

struct _listview_tag // 清單顯示構件
{
    Widget base;
    const Strings *texts;
    int nmax;
};

static void listview_ctor(Listview *listview, Widget *parent, Widget_id id, int x, int y, int w, int h, const Strings *texts);
static int get_strings_width(const Strings *texts);
static int get_strings_height(const Strings *texts);
static void listview_set_method(Widget *widget);

Listview *listview_new(Widget *parent, Widget_id id, int x, int y, int w, int h, const Strings *texts)
{
    if((!texts || list_is_empty(&texts->list)) && (w<=0 || h<=0))
        return NULL;

    Listview *listview=Malloc(sizeof(Listview));
    listview_ctor(listview, parent, id, x, y, w, h, texts);

    return listview;
}

static void listview_ctor(Listview *listview, Widget *parent, Widget_id id, int x, int y, int w, int h, const Strings *texts)
{
    w = w>0 ? w : get_strings_width(texts);
    h = h>0 ? h : get_strings_height(texts);
    widget_ctor(WIDGET(listview), parent, id, WIDGET_STATE_NORMAL, x, y, w, h);
    listview->texts=texts;
    listview->nmax=INT_MAX;
    listview_set_method(WIDGET(listview));
    XSelectInput(xinfo.display, WIDGET_WIN(listview), None);
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

static void listview_set_method(Widget *widget)
{
    widget->show=listview_show;
    widget->update_fg=listview_update_fg;
}

void listview_show(Widget *widget)
{
    XRaiseWindow(xinfo.display, WIDGET_WIN(widget));
    widget_show(widget);
}

void listview_update_fg(const Widget *widget)
{
    Listview *listview=LIST_VIEW(widget);
    if(!listview->texts)
        return;
    
    Window win=WIDGET_WIN(listview);
    int w=WIDGET_W(listview), h=get_font_height_by_pad(),
        i=0, nmax=listview->nmax;
    Str_fmt fmt={0, 0, w, h, CENTER_LEFT, true, false, 0,
        get_widget_fg(WIDGET_STATE_NORMAL)};

    list_for_each_entry(Strings, s, &listview->texts->list, list)
    {
        if(i < nmax)
            draw_string(win, i<nmax-1 ? s->str : "...", &fmt), fmt.y+=h;
        else
            break;
        i++;
    }
}

void listview_update(Listview *listview, const Strings *texts)
{
    int w=WIDGET_W(listview), hl=get_font_height_by_pad(),
        n=list_count_nodes(&texts->list), h=MIN(n, listview->nmax)*hl;

    listview->texts=texts;
    widget_resize(WIDGET(listview), w, h);
    listview_show(WIDGET(listview));
    listview_update_fg(WIDGET(listview));
}

void listview_set_nmax(Listview *listview, int nmax)
{
    listview->nmax=nmax;
}
