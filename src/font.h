/* *************************************************************************
 *     font.h：與font.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef FONT_H
#define FONT_H

typedef enum align_type_tag // 文字對齊方式
{
    TOP_LEFT, TOP_CENTER, TOP_RIGHT,
    CENTER_LEFT, CENTER, CENTER_RIGHT,
    BOTTOM_LEFT, BOTTOM_CENTER, BOTTOM_RIGHT,
} Align_type;

typedef struct // 字符串格式
{
    int x, y, w, h; // 字符串區域的坐標和尺寸信息
    Align_type align; // 對齊方式
    bool pad, change_bg; // 兩端是否留白, 是否改變背景色的標志
    unsigned long bg; // 字符串區域的背景色
    XftColor fg; // 字符串的前景色
} Str_fmt;

void load_fonts(void);
void close_fonts(void);
void draw_wcs(Drawable d, const wchar_t *wcs, const Str_fmt *f);
void draw_string(Drawable d, const char *str, const Str_fmt *f);
void get_string_size(const char *str, int *w, int *h);
int get_min_font_size(void);
int get_scale_font_size(double scale);
int get_font_pad(void);
int get_font_height_by_pad(void);

#endif
