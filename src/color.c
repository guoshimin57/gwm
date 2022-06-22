/* *************************************************************************
 *     color.c：實現分配顏色的功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void alloc_widget_color(WM *wm, const char *color_name, XColor *color);
static void alloc_text_color(WM *wm, const char *color_name, XftColor *color);

static void alloc_widget_color(WM *wm, const char *color_name, XColor *color)
{
    XParseColor(wm->display, wm->colormap, color_name, color); 
    XAllocColor(wm->display, wm->colormap, color);
}

static void alloc_text_color(WM *wm, const char *color_name, XftColor *color)
{
    XftColorAllocName(wm->display, wm->visual, wm->colormap, color_name, color);
}

void alloc_color(WM *wm)
{
    for(Widget_color i=0; i<WIDGET_COLOR_N; i++)
        alloc_widget_color(wm, WIDGET_COLOR_NAME[i], wm->widget_color+i);
    for(Text_color i=0; i<TEXT_COLOR_N; i++)
        alloc_text_color(wm, TEXT_COLOR_NAME[i], wm->text_color+i);
}
