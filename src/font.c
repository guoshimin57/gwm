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

#include "gwm.h"
#include "font.h"
#include "misc.h"

static char *copy_string(const char *s);

void create_font_set(WM *wm)
{
    char **list, *str;
    int n;
    wm->font_set=XCreateFontSet(wm->display, FONT_SET, &list, &n, &str);
    XFreeStringList(list);
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
        else if(XmbTextPropertyToTextList(wm->display, &name, &list, &n) == Success)
            result=copy_string(*list), XFreeStringList(list);
        XFree(name.value);
    }
    else
        result=copy_string(win==wm->taskbar.win ? "gwm" : "");
    return result;
}

static char *copy_string(const char *s)
{
    return strcpy(malloc_s(strlen(s)+1), s);
}

void draw_string(WM *wm, Drawable d, const char *str, const String_format *f)
{
    if(str && str[0]!='\0')
    {
        unsigned int w=f->r.w, h=f->r.h, lw, lh, n=strlen(str);
        get_string_size(wm, str, &lw, &lh);
        int x=f->r.x, y=f->r.y, sx, sy, cx=x+w/2-lw/2, cy=y+h/2+lh/2,
            left=x, right=x+w-lw, top=y+lh, bottom=y+h;
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
        if(f->bg != f->fg)
        {
            XSetForeground(wm->display, wm->gc, f->bg);
            XFillRectangle(wm->display, d, wm->gc, x, y, w, h);
        }
        XSetForeground(wm->display, wm->gc, f->fg);
        XmbDrawString(wm->display, d, wm->font_set, wm->gc, sx, sy, str, n);
    }
}

void get_string_size(WM *wm, const char *str, unsigned int *w, unsigned int *h)
{
    XRectangle ink, logical;
    XmbTextExtents(wm->font_set, str, strlen(str), &ink, &logical);
    if(w)
        *w=logical.width;
    if(h)
        *h=logical.height;
}
