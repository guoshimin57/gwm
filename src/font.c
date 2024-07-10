/* *************************************************************************
 *     font.c：實現字體相關功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <fontconfig/fontconfig.h>
#include "gwm.h"

struct _font_tag
{
    XftFont *xfont;
    struct _font_tag *next;
};
typedef struct _font_tag WMFont;

static WMFont *load_font(const char *fontname);
static void close_font(WMFont *font);
static bool has_exist_font(const XftFont *xfont);
static WMFont *get_last_font(void);
static void init_font_set(void);
static int draw_utf8_char(XftDraw *draw, const XftColor *fg, uint32_t codepoint, int len, const FcChar8 *s, int x, int y);
static WMFont *get_suitable_font(uint32_t codepoint);
static void get_str_rect_by_fmt(const Str_fmt *f, const char *str, int *x, int *y, int *w, int *h);
static int get_utf8_codepoint(const char *str, uint32_t *codepoint);

static WMFont *fonts=NULL;
static FcFontSet *font_set=NULL;

void load_fonts(void)
{
    for(int i=0; cfg->font_names[i]; i++)
        load_font(cfg->font_names[i]);
    init_font_set();
}

void close_fonts(void)
{
    for(WMFont *p=fonts; p; p=p->next)
        close_font(p);
    FcFontSetDestroy(font_set);
    FcFini();
}

static WMFont *load_font(const char *fontname)
{
    char name[BUFSIZ];
    XftFont *fp=NULL;

    sprintf(name, "%s:pixelsize=%u", fontname, cfg->font_size);
    if(!(fp=XftFontOpenName(xinfo.display, xinfo.screen, name)))
        return NULL;

    if(has_exist_font(fp))
    {
        XftFontClose(xinfo.display, fp);
        return NULL;
    }

    WMFont *font=malloc_s(sizeof(WMFont));
    font->xfont=fp, font->next=NULL;
    if(fonts)
        get_last_font()->next=font;
    else
        fonts=font;

    return font;
}

static void close_font(WMFont *font)
{
    for(WMFont *p=fonts, *prev=fonts; p; prev=p, p=p->next)
    {
        if(p == font)
        {
            if(p == fonts)
                fonts=p->next;
            else
                prev->next=p->next;
            XftFontClose(xinfo.display, p->xfont);
            vfree(p);
            break;
        }
    }
} 

static bool has_exist_font(const XftFont *xfont)
{
    for(WMFont *p=fonts; p; p=p->next)
        if(xfont == p->xfont)
            return true;
    return false;
}

static WMFont *get_last_font(void)
{
    WMFont *p;
    for(p=fonts; p && p->next; p=p->next)
        ;
    return p;
}

static void init_font_set(void)
{
    FcPattern *pat=FcNameParse((FcChar8 *)":");
    FcObjectSet *os=FcObjectSetCreate();
    FcObjectSetAdd(os, "family");
    font_set=FcFontList(NULL, pat, os);
    FcObjectSetDestroy(os);
    FcPatternDestroy(pat);
}

void draw_wcs(Drawable d, const wchar_t *wcs, const Str_fmt *f)
{
    size_t n=wcslen(wcs)*MB_CUR_MAX+1;
    char mbs[n];
    wcstombs(mbs, wcs, n);
    draw_string(d, mbs, f);
}

void draw_string(Drawable d, const char *str, const Str_fmt *f)
{
    if(!str)
        return;

    int x=f->x, y=f->y, w=f->w, h=f->h, sx, sy, sw, sh;

    get_str_rect_by_fmt(f, str, &sx, &sy, &sw, &sh);
    XClearArea(xinfo.display, d, x, y, w, h, False); 
    if(f->change_bg)
    {
        GC gc=XCreateGC(xinfo.display, d, 0, NULL);
        XSetForeground(xinfo.display, gc, f->bg);
        XFillRectangle(xinfo.display, d, gc, x, y, w, h);
    }

    int len;
    uint32_t codepoint;
    XftDraw *draw=XftDrawCreate(xinfo.display, d, xinfo.visual, xinfo.colormap);
    while(*str)
    {
        len=get_utf8_codepoint(str, &codepoint);
        sx+=draw_utf8_char(draw, &f->fg, codepoint, len, (const FcChar8 *)str, sx, sy);
        str+=len;
    }
    XftDrawDestroy(draw);
}

/* libXrender文檔沒有解釋XGlyphInfo結構體成員的含義。 猜測xOff指字符串原點到
 * 字符串限定框最右邊的偏移量。 */
static int draw_utf8_char(XftDraw *draw, const XftColor *fg, uint32_t codepoint, int len, const FcChar8 *s, int x, int y)
{
    WMFont *font=get_suitable_font(codepoint);
    if(font == NULL)
        return 0;

    XGlyphInfo info;
    XftDrawStringUtf8(draw, fg, font->xfont, x, y, s, len);
    XftTextExtentsUtf8(xinfo.display, font->xfont, s, len, &info);
    return info.xOff;
}

static WMFont *get_suitable_font(uint32_t codepoint)
{
    WMFont *font;
    const FcChar8 *fmt=(const FcChar8 *)"%{=fclist}";

    for(font=fonts; font; font=font->next)
        if(XftCharExists(xinfo.display, font->xfont, codepoint))
            return font;

    for(int i=0; i<font_set->nfont; i++)
    {
        if((font=load_font((char *)FcPatternFormat(font_set->fonts[i], fmt))))
        {
            if(XftCharExists(xinfo.display, font->xfont, codepoint))
                return font;
            else
                close_font(font);
        }
    }

    return NULL;
}

/* Xft文檔沒有解析XftFont的height成员的含義，但ascent+descent的確比height大1，
 * 且前者看上去似乎才是實際的字體高度 */
static void get_str_rect_by_fmt(const Str_fmt *f, const char *str, int *x, int *y, int *w, int *h)
{
    int cx, cy, pad, left, right, top, bottom;

    pad = f->pad ? get_font_pad() : 0;
    get_string_size(str, w, h);
    cx=f->x+f->w/2-*w/2;
    cy=f->y+(f->h-fonts->xfont->height)/2+fonts->xfont->ascent;
    left=f->x+pad, right=f->x+f->w-*w-pad;
    top=f->y+fonts->xfont->ascent, bottom=f->y+f->h-fonts->xfont->descent;

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
    if(*w+2*pad>f->w)
        *x=left;
}

void get_string_size(const char *str, int *w, int *h)
{
    int width=0, max_asc=0, max_desc=0;

    if(str)
    {
        uint32_t codepoint;
        XGlyphInfo info;
        WMFont *font=NULL;

        for(int len=0; *str; str+=len)
        {
            len=get_utf8_codepoint(str, &codepoint);
            if(len && (font=get_suitable_font(codepoint)))
            {
                XftTextExtentsUtf8(xinfo.display, font->xfont, (const FcChar8 *)str, len, &info);
                width += info.xOff;
                if(font->xfont->ascent > max_asc)
                   max_asc=font->xfont->ascent;
                if(font->xfont->descent > max_desc)
                   max_desc=font->xfont->descent;
            }
        }
    }
    if(w)
        *w=width;
    if(h)
        *h=max_asc+max_desc;
}

static int get_utf8_codepoint(const char *str, uint32_t *codepoint)
{
    const uint8_t *p=(const uint8_t*)str;
    int len=0;
    
    if(*p < 0x80) // 單字節字符
        len=1, *codepoint=*p;
    else if((*p>>5) == 0x06) // 雙字節字符
        len=2, *codepoint=(*p & 0x1F)<<6 | (*(p+1) & 0x3F);
    else if((*p>>4) == 0x0E) // 三字節字符
        len=3, *codepoint=(*p & 0x0F)<<12 | (*(p+1) & 0x3F)<<6 | (*(p+2) & 0x3F);
    else if((*p>>3) == 0x1E) // 四字節字符
        len=4, *codepoint=(*p & 0x07)<<18 | (*(p+1) & 0x3F)<<12 | (*(p+2) & 0x3F)<<6 | (*(p+3) & 0x3F);
    else // 非utf8編碼字符
        len=0;

    return len;
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
int get_min_font_size(void)
{
    int w=DisplayWidthMM(xinfo.display, xinfo.screen),
        h=DisplayHeightMM(xinfo.display, xinfo.screen),
        W=DisplayWidth(xinfo.display, xinfo.screen),
        H=DisplayHeight(xinfo.display, xinfo.screen);
    double dpm=sqrt(W*W+H*H)/sqrt(w*w+h*h), dpi=25.4*dpm;
    return ceil(0.9248*dpi/7.62);
}

int get_scale_font_size(double scale)
{
    return scale*get_min_font_size();
}

int get_font_pad(void)
{
    return cfg->font_size*cfg->font_pad_ratio+0.5;
}

int get_font_height_by_pad(void)
{
    return cfg->font_size*(1+cfg->font_pad_ratio*2)+0.5;
}
