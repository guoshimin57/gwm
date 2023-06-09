/* *************************************************************************
 *     font.h：與font.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef FONT_H
#define FONT_H

struct string_format_tag // 字符串格式
{
    Rect r; // 坐標和尺寸信息
    Align_type align; // 對齊方式
    bool change_bg; // 是否改變背景色的標志
    unsigned long bg; // r區域的背景色
    XftColor fg; // 字符串的前景色
    Font_type font_type; // 字體類型
};

void load_font(WM *wm);
void draw_wcs(WM *wm, Drawable d, const wchar_t *wcs, const String_format *f);
void draw_string(WM *wm, Drawable d, const char *str, const String_format *f);
void get_string_size(WM *wm, XftFont *font, const char *str, int *w, int *h);
void close_fonts(WM *wm);
int get_min_font_size(WM *wm);
int get_scale_font_size(WM *wm, double scale);
int get_font_pad(WM *wm, Font_type type);
int get_font_height_by_pad(WM *wm, Font_type type);

#endif
