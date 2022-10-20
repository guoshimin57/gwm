/* *************************************************************************
 *     font.c：實現字體相關功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <X11/Xft/Xft.h>
#include "gwm.h"
#include "font.h"
#include "misc.h"

void load_font(WM *wm)
{
    for(size_t i=0; i<FONT_N; i++)
    {
        bool flag=true;
        for(size_t j=0; j<i && flag; j++)
            if(strcmp(FONT_NAME[j], FONT_NAME[i]) == 0)
                wm->font[i]=wm->font[j], flag=false;
        if(flag)
            if(!(wm->font[i]=XftFontOpenName(wm->display, wm->screen, FONT_NAME[i])))
                if(!(wm->font[i]=XftFontOpenName(wm->display, wm->screen, DEFAULT_FONT_NAME)))
                    exit_with_msg("錯誤：不能加載必要的字體。");
    }
}

char *get_text_prop(WM *wm, Window win, Atom atom)
{
    int n;
    char **list=NULL, *result=NULL;
    XTextProperty name;
    if(XGetTextProperty(wm->display, win, &name, atom))
    {
        if(name.encoding == XA_STRING)
            result=copy_string((char *)name.value);
        else if(Xutf8TextPropertyToTextList(wm->display, &name, &list, &n) == Success)
            result=copy_string(*list), XFreeStringList(list);
        XFree(name.value);
    }
    else
        result=copy_string(win==wm->taskbar.win ? "gwm" : "");
    return result;
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
    if(str && str[0]!='\0')
    {
        XftFont *font=wm->font[f->font_type];
        unsigned int w=f->r.w, h=f->r.h, lw, lh, n=strlen(str);
        get_string_size(wm, font, str, &lw, &lh);
        int x=f->r.x, y=f->r.y, cx=x+w/2-lw/2, cy=y+h/2-lh/2+font->ascent,
            sx, sy, left=x, right=x+w-lw, top=y+lh, bottom=y+h;
        switch(f->align)
        {
            case TOP_LEFT: sx=left, sy=top; break;
            case TOP_CENTER: sx=cx, sy=top; break;
            case TOP_RIGHT: sx=right, sy=top; break;
            case CENTER_LEFT: sx=left, sy=cy; break;
            case CENTER: sx=cx, sy=cy; break;
            case CENTER_RIGHT: sx=right, sy=cy; break;
            case BOTTOM_LEFT: sx=left, sy=bottom; break;
            case BOTTOM_CENTER: sx=cx, sy=bottom; break;
            case BOTTOM_RIGHT: sx=right, sy=bottom; break;
        }
        XClearArea(wm->display, d, x, y, w, h, False); 
        if(f->change_bg)
        {
            XSetForeground(wm->display, wm->gc, f->bg);
            XFillRectangle(wm->display, d, wm->gc, x, y, w, h);
        }
        XftDraw *draw=XftDrawCreate(wm->display, d, wm->visual, wm->colormap);
        XftDrawStringUtf8(draw, &f->fg, font, sx, sy, (const FcChar8 *)str, n);
        XftDrawDestroy(draw);
    }
}

void get_string_size(WM *wm, XftFont *font, const char *str, unsigned int *w, unsigned int *h)
{
    /* libXrender文檔沒有解釋XGlyphInfo結構體成員的含義。
       猜測xOff指字符串原點到字符串限定框最右邊的偏移量。*/
    XGlyphInfo e;
    XftTextExtentsUtf8(wm->display, font, (const FcChar8 *)str, strlen(str), &e);
    if(w)
        *w=e.xOff;
    if(h)
        *h=font->height;
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
