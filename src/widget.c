/* *************************************************************************
 *     widget.c：實現構件相關功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void alloc_widget_color(const char *color_name, XColor *color);
static void alloc_text_color(const char *color_name, XftColor *color);

static XColor widget_color[COLOR_THEME_N][WIDGET_COLOR_N]; // 構件顏色
static XftColor text_color[COLOR_THEME_N][TEXT_COLOR_N]; // 文本顏色

static void alloc_widget_color(const char *color_name, XColor *color)
{
    XParseColor(xinfo.display, xinfo.colormap, color_name, color); 
    XAllocColor(xinfo.display, xinfo.colormap, color);
}

static void alloc_text_color(const char *color_name, XftColor *color)
{
    XftColorAllocName(xinfo.display, xinfo.visual, xinfo.colormap, color_name, color);
}

void alloc_color(void)
{
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Widget_color j=0; j<WIDGET_COLOR_N; j++)
            alloc_widget_color(cfg->widget_color_name[i][j], &widget_color[i][j]);
    for(Color_theme i=0; i<COLOR_THEME_N; i++)
        for(Text_color j=0; j<TEXT_COLOR_N; j++)
            alloc_text_color(cfg->text_color_name[i][j], &text_color[i][j]);
}

unsigned long get_widget_color(Widget_color wc)
{
    float wo=cfg->widget_opacity[cfg->color_theme][wc];
    unsigned long rgb=widget_color[cfg->color_theme][wc].pixel;

    return ((rgb & 0x00ffffff) | ((unsigned long)(0xff*wo))<<24);
}

XftColor get_text_color(Text_color color_id)
{
    return text_color[cfg->color_theme][color_id];
}

Window create_widget_win(Widget_type type, Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel)
{
    XSetWindowAttributes attr;
    attr.colormap=xinfo.colormap;
    attr.border_pixel=border_pixel;
    attr.background_pixel=bg_pixel;
    attr.override_redirect=True;

    Window win=XCreateWindow(xinfo.display, parent, x, y, w, h, border_w, xinfo.depth,
        InputOutput, xinfo.visual,
        CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect, &attr);

    set_gwm_widget_type(win, type);

    return win;
}

Widget_type get_widget_type(Window win)
{
    CARD32 type;
    return get_gwm_widget_type(win, &type) ? type : NON_WIDGET;
}

void update_hint_win_for_info(Window hover, const char *info)
{
    int x, y, rx, ry, pad=get_font_pad(),
        w=0, h=get_font_height_by_pad();

    get_string_size(info, &w, NULL);
    w+=pad*2;
    if(hover)
    {
        Window r, c;
        unsigned int m;
        if(!XQueryPointer(xinfo.display, hover, &r, &c, &rx, &ry, &x, &y, &m))
            return;
        set_pos_for_click(hover, rx, &x, &y, w, h);
    }
    else
        x=(xinfo.screen_width-w)/2, y=(xinfo.screen_height-h)/2;
    XMoveResizeWindow(xinfo.display, xinfo.hint_win, x, y, w, h);
    XMapRaised(xinfo.display, xinfo.hint_win);
    Str_fmt f={0, 0, w, h, CENTER, true, false, 0,
        get_text_color(HINT_TEXT_COLOR)};
    draw_string(xinfo.hint_win, info, &f);
}

void draw_icon(Drawable d, Imlib_Image image, const char *name, int size)
{
    if(image)
        draw_image(image, d, 0, 0, size, size);
    else
    {
        char s[2]={name[0], '\0'};
        XftColor color=get_text_color(CLASS_TEXT_COLOR);
        Str_fmt fmt={0, 0, size, size, CENTER, false, false, 0, color};
        draw_string(d, s, &fmt);
    }
}
