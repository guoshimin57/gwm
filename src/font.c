/* *************************************************************************
 *     font.c：實現字體相關功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static int get_str_rect_by_fmt(WM *wm, const String_format *f, const char *str, int *x, int *y, int *w, int *h);

void load_font(WM *wm)
{
    for(size_t i=0; i<FONT_N; i++)
    {
        XftFont *p=NULL;
        for(size_t j=0; j<i && !p; j++)
            if(strcmp(wm->cfg->font_name[j], wm->cfg->font_name[i]) == 0)
                p=wm->font[j];
        if(!p)
            p=XftFontOpenName(wm->display, wm->screen, wm->cfg->font_name[i]);
        if(p)
            wm->font[i]=p;
        else
            exit_with_msg(_("錯誤：不能加載必要的字體。"));
    }
}

void draw_wcs(WM *wm, Drawable d, const wchar_t *wcs, const String_format *f)
{
    size_t n=wcslen(wcs)*MB_CUR_MAX+1;
    char mbs[n];
    wcstombs(mbs, wcs, n);
    draw_string(wm, d, mbs, f);
}

void draw_string(WM *wm, Drawable d, const char *str, const String_format *f)
{
    if(!str)
        return;

    XftFont *font=wm->font[f->font_type];
    int x=f->r.x, y=f->r.y, w=f->r.w, h=f->r.h, sx, sy, sw, sh, pad, n;

    pad=get_str_rect_by_fmt(wm, f, str, &sx, &sy, &sw, &sh);
    n=strlen(str);
    XClearArea(wm->display, d, x, y, w, h, False); 
    if(f->change_bg)
    {
        XSetForeground(wm->display, wm->gc, f->bg);
        XFillRectangle(wm->display, d, wm->gc, x, y, w, h);
    }

    XftDraw *draw=XftDrawCreate(wm->display, d, wm->visual, wm->colormap);
    const char *es="︙";
    if(f->trunc && sw+2*pad>w)
    {
        int ew, en=strlen(es);
        get_string_size(wm, font, es, &ew, NULL);
        XftDrawStringUtf8(draw, &f->fg, font, sx, sy, (const FcChar8 *)str, n);
        XClearArea(wm->display, d, x+w-ew, 0, ew, h, False); 
        XftDrawStringUtf8(draw, &TEXT_COLOR(wm, HINT), font, x+w-ew, sy, (const FcChar8 *)es, en);
    }
    else
        XftDrawStringUtf8(draw, &f->fg, font, sx, sy, (const FcChar8 *)str, n);
    XftDrawDestroy(draw);
}

static int get_str_rect_by_fmt(WM *wm, const String_format *f, const char *str, int *x, int *y, int *w, int *h)
{
    XftFont *font=wm->font[f->font_type];
    int cx, cy, pad, left, right, top, bottom;

    pad = f->pad ? get_font_pad(wm, f->font_type) : 0;
    get_string_size(wm, font, str, w, h);
    cx=f->r.x+f->r.w/2-*w/2, cy=f->r.y+f->r.h/2-*h/2+font->ascent;
    left=f->r.x+pad, right=f->r.x+f->r.w-*w-pad;
    top=f->r.y+*h, bottom=f->r.y+f->r.h;

    switch(f->align)
    {
        case TOP_LEFT: *x=left, *y=top; break;
        case TOP_CENTER: *x=cx, *y=top; break;
        case TOP_RIGHT: *x=right, *y=top; break;
        case CENTER_LEFT: *x=left, *y=cy; break;
        case CENTER: *x=cx, *y=cy; break;
        case CENTER_RIGHT: *x=right, *y=cy; break;
        case BOTTOM_LEFT: *x=left, *y=bottom; break;
        case BOTTOM_CENTER: *x=cx, *y=bottom; break;
        case BOTTOM_RIGHT: *x=right, *y=bottom; break;
    }
    if(f->trunc && *w+2*pad>f->r.w)
        *x=left;
    return pad;
}

void get_string_size(WM *wm, XftFont *font, const char *str, int *w, int *h)
{
    /* libXrender文檔沒有解釋XGlyphInfo結構體成員的含義。
       猜測xOff指字符串原點到字符串限定框最右邊的偏移量。*/
    XGlyphInfo e;
    XftTextExtentsUtf8(wm->display, font, (const FcChar8 *)str, strlen(str), &e);
    if(w)
        *w=e.xOff;
    /* Xft文檔沒有解析font->height的含義，但font->ascent+font->descent的確比
     * font->height大1，且前者看上去似乎才是實際的字體高度 */
    if(h)
        *h=font->ascent+font->descent;
}

void close_fonts(WM *wm)
{
    for(size_t i=0, j=0; i<FONT_N; i++)
    {
        for(j=0; j<i; j++)
            if(wm->font[i] == wm->font[j])
                break;
        if(j == i)
            XftFontClose(wm->display, wm->font[i]);
    }
}

/*
 * 正常坐姿下，人眼到屏幕的距離是0.8米，正常視力的人可以勉強看清距離5米、
 * 高度爲5.78毫米的E字。根據相似三角形的性質可知：
 *                                 *
 *                         *       *
 *                 *               *
 *         *                       *
 * *       +       *       *       + 5m, 5.78mm
 *         *                       *
 *       0.8m      *               *
 *                         *       *
 *                                 *
 * 人眼可以看得清的E字最小高度hE=0.8*5.78/5=0.9248mm。
 * 因1英寸等於25.4毫米，故DPM=DPI/25.4。
 * 若以像素計量，人眼可以看得清的E字最小高度HE=hE*DPM=hE*DPI/25.4。
 * 一般字體會爲字留空白，最大字高佔外框高度的90%左右，故與人眼可以看得
 * 清的E字相應的最小字體高度HEf=HE/0.9=hE*DPM/0.9=hE*DPI/(25.4*0.9)。
 * 漢字或其他一些文字的筆畫遠比E字多，若以筆畫數最多的常用字“矗”爲準，
 * 則筆畫數是E的6倍，考慮到同一字體，非字母文字尺寸是字母文字的2倍，故
 * 實際筆畫密度是E的3倍。因此，人眼可以看得清的最小字體尺寸
 * Hf=3*HEf=3*hE*DPM/0.9=3*hE*DPI/(25.4*0.9)。
 * 簡化可得：Hf=3*HEf=hE*DPM/0.3=hE*DPI/7.62。
 * 考慮到舒適性，應乘以一個放大系數a。因此，人眼可以輕鬆看得清的最小字體尺寸
 * Hfb=a*Hf=a*3*HEf=a*hE*DPM/0.3=a*hE*DPI/7.62。
 * 對於近視的人，字體尺寸還應調大一點。
 */
int get_min_font_size(WM *wm)
{
    int w=DisplayWidthMM(wm->display, wm->screen),
        h=DisplayHeightMM(wm->display, wm->screen),
        W=DisplayWidth(wm->display, wm->screen),
        H=DisplayHeight(wm->display, wm->screen);
    double dpm=sqrt(W*W+H*H)/sqrt(w*w+h*h), dpi=25.4*dpm;
    return ceil(0.9248*dpi/7.62);
}

int get_scale_font_size(WM *wm, double scale)
{
    return scale*get_min_font_size(wm);
}

int get_font_pad(WM *wm, Font_type type)
{
    return wm->cfg->font_size[type]*wm->cfg->font_pad_ratio+0.5;
}

int get_font_height_by_pad(WM *wm, Font_type type)
{
    return wm->cfg->font_size[type]*(1+wm->cfg->font_pad_ratio*2)+0.5;
}
