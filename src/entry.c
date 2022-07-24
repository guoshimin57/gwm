/* *************************************************************************
 *     entry.c：實現單行文本輸入框功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "entry.h"
#include "font.h"
#include "grab.h"
#include "misc.h"

void create_entry(WM *wm, Entry *e, Rect *r, wchar_t *hint)
{
    XSetWindowAttributes attr={.override_redirect=True};
    e->x=r->x, e->y=r->y, e->w=r->w, e->h=r->h;
    e->win=XCreateSimpleWindow(wm->display, wm->root_win,
        e->x, e->y, e->w, e->h,
        0, 0, wm->widget_color[ENTRY_COLOR].pixel);
    XChangeWindowAttributes(wm->display, e->win, CWOverrideRedirect, &attr);
    XSelectInput(wm->display, e->win, ENTRY_EVENT_MASK);
    e->hint=hint;
    set_xic(wm, e->win, &e->xic);
}

void show_entry(WM *wm, Entry *e)
{
    e->text[0]=L'\0', e->cursor_offset=0;
    XMapWindow(wm->display, e->win);
    update_entry_text(wm, e);
    XGrabKeyboard(wm->display, e->win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void update_entry_text(WM *wm, Entry *e)
{
    String_format f={{ENTRY_TEXT_INDENT, 0, e->w, e->h}, CENTER_LEFT, false, 0,
        e->text[0]==L'\0' ? wm->text_color[HINT_TEXT_COLOR] :
        wm->text_color[ENTRY_TEXT_COLOR]};
    int x=get_entry_cursor_x(wm, e);
    XClearArea(wm->display, e->win, 0, 0, e->w, e->h, False); 
    draw_wcs(wm, e->win, e->text[0]==L'\0' ? e->hint : e->text, &f);
    XDrawLine(wm->display, e->win, wm->gc, x, 0, x, e->h);
}

int get_entry_cursor_x(WM *wm, Entry *e)
{
    unsigned int n=e->cursor_offset*MB_CUR_MAX+1, w=0;
    wchar_t wc=e->text[e->cursor_offset]; 
    char mbs[n]; 
    e->text[e->cursor_offset]=L'\0'; 
    wcstombs(mbs, e->text, n);
    get_string_size(wm, wm->font[ENTRY_FONT], mbs, &w, NULL);
    e->text[e->cursor_offset]=wc; 
    return ENTRY_TEXT_INDENT+w;
}

void handle_key_press_for_entry(WM *wm, Entry *e, KeySym ks, wchar_t *keyname, unsigned int state)
{
    if(is_equal_modifier_mask(wm, ControlMask, state))
    {
        if(ks == XK_u)
            e->text[e->cursor_offset=0]=L'\0';
    }
    else if(is_equal_modifier_mask(wm, None, state))
    {
        size_t n1=wcslen(e->text), n2=wcslen(keyname), n=ARRAY_NUM(e->text);
        switch(ks)
        {
            case XK_Escape: case XK_Return: case XK_KP_Enter:
                XUngrabKeyboard(wm->display, CurrentTime);
                XUnmapWindow(wm->display, e->win);
                return;
            case XK_BackSpace:
                if(n1)
                    for(wchar_t *p=e->text+e->cursor_offset---1; *p!=L'\0'; p++)
                        *p=*(p+1);
                break;
            case XK_Delete: case XK_KP_Delete:
                if(n1 < n)
                    for(wchar_t *p=e->text+e->cursor_offset; *p!=L'\0'; p++)
                        *p=*(p+1);
                break;
            case XK_Left: case XK_KP_Left:
                if(e->cursor_offset)
                    e->cursor_offset--;
                break;
            case XK_Right: case XK_KP_Right:
                if(e->cursor_offset < n1)
                    e->cursor_offset++;
                break;
            case XK_Tab: break;
            default:
                if(n1+n2 < n)
                {
                    wcscpy(e->text+n1, keyname);
                    e->cursor_offset+=n2;
                }
        }
    }
    update_entry_text(wm, e);
}
