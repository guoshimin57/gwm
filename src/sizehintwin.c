/* *************************************************************************
 *     sizehintwin.c：實現尺寸提示窗口相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "misc.h"
#include "icccm.h"
#include "font.h"
#include "config.h"
#include "sizehintwin.h"

#define SIZE_HINT_INFO_MAX 32

struct _size_hint_win_tag
{
    Widget base;
    Widget *hint_for;
    char info[SIZE_HINT_INFO_MAX];
};

static void size_hint_win_ctor(Size_hint_win *size_hint_win, Widget *hint_for);
static void size_hint_win_set_method(Widget *widget);
static void size_hint_win_set_info(Size_hint_win *size_hint_win);

Size_hint_win *size_hint_win_new(Widget *hint_for)
{
    Size_hint_win *size_hint_win=Malloc(sizeof(Size_hint_win));
    size_hint_win_ctor(size_hint_win, hint_for);
    return size_hint_win;
}

static void size_hint_win_ctor(Size_hint_win *size_hint_win, Widget *hint_for)
{
    int x, y, w, h=get_font_height_by_pad();

    get_string_size(" (xxxx, yyyy) wwww, hhhh ", &w, NULL);
    x=(WIDGET_W(hint_for)-w)/2, y=(WIDGET_H(hint_for)-h)/2;
    widget_ctor(WIDGET(size_hint_win), NULL, WIDGET_TYPE_SIZE_HINT_WIN, UNUSED_WIDGET_ID, x, y, w, h);
    size_hint_win_set_method(WIDGET(size_hint_win));
    size_hint_win->hint_for=hint_for;
    size_hint_win_set_info(size_hint_win);
}

static void size_hint_win_set_method(Widget *widget)
{
    widget->update_fg=size_hint_win_update_fg;
}

void size_hint_win_update_fg(const Widget *widget)
{
    Size_hint_win *size_hint_win=SIZE_HINT_WIN(widget);
    Str_fmt f={0, 0, WIDGET_W(widget), WIDGET_H(widget), CENTER,
        true, false, 0, get_text_color(widget)};

    draw_string(WIDGET_WIN(widget), size_hint_win->info, &f);
}

void size_hint_win_update(Size_hint_win *size_hint_win)
{
    widget_show(WIDGET(size_hint_win));
    size_hint_win_set_info(size_hint_win);
    size_hint_win_update_fg(WIDGET(size_hint_win));
}

static void size_hint_win_set_info(Size_hint_win *size_hint_win)
{
    XSizeHints hint=get_size_hint(WIDGET_WIN(size_hint_win->hint_for));
    long col=get_win_col(WIDGET_W(size_hint_win->hint_for), &hint),
         row=get_win_row(WIDGET_H(size_hint_win->hint_for), &hint);

    sprintf(size_hint_win->info, "(%d, %d) %ldx%ld", WIDGET_X(size_hint_win->hint_for), WIDGET_Y(size_hint_win->hint_for), col, row);
}
