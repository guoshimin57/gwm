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

void create_entry(WM *wm, Entry *e, Rect *r, char *hint)
{
    XSetWindowAttributes attr={.override_redirect=True};
    e->x=r->x, e->y=r->y, e->w=r->w, e->h=r->h;
    e->win=XCreateSimpleWindow(wm->display, wm->root_win,
        e->x, e->y, e->w, e->h,
        0, 0, wm->widget_color[ENTRY_COLOR].pixel);
    XChangeWindowAttributes(wm->display, e->win, CWOverrideRedirect, &attr);
    XSelectInput(wm->display, e->win, ButtonPressMask|KeyPressMask|ExposureMask);
    e->hint=hint;
}

void show_entry(WM *wm, Entry *e)
{
    e->text[0]='\0';
    XMapWindow(wm->display, e->win);
    String_format f={{0, 0, e->w, e->h},
        CENTER_LEFT, false, 0, wm->text_color[HINT_TEXT_COLOR]};
    draw_string(wm, e->win, e->hint, &f);
    XGrabKeyboard(wm->display, e->win, True, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void update_entry_text(WM *wm, Entry *e)
{
    String_format f={{0, 0, e->w, e->h}, CENTER_LEFT, false, 0,
        e->text[0]=='\0' ? wm->text_color[HINT_TEXT_COLOR]
        : wm->text_color[ENTRY_TEXT_COLOR]};
    XClearArea(wm->display, e->win, 0, 0, e->w, e->h, False); 
    draw_string(wm, e->win, e->text[0]=='\0' ? e->hint : e->text, &f);
}

void handle_key_press_for_entry(WM *wm, XKeyEvent *e)
{
    char buf[BUFSIZ]={0};
    KeySym ks;
    Entry *r=&wm->run_cmd;

    XLookupString(e, buf, sizeof(buf), &ks, 0);
    if(IsCursorKey(ks) || IsFunctionKey(ks) || IsModifierKey(ks) || IsMiscFunctionKey(ks) || IsPFKey(ks))
        return;
    if(is_equal_modifier_mask(wm, ControlMask, e->state))
        r->text[0] = ks==XK_u ? '\0' : r->text[0];
    else if(is_equal_modifier_mask(wm, None, e->state))
    {
        size_t n=strlen(r->text);
        if(ks == XK_BackSpace)
            r->text[n-1]='\0';
        else if(ks == XK_Escape || ks == XK_Return)
        {
            XUngrabKeyboard(wm->display, CurrentTime);
            XUnmapWindow(wm->display, r->win);
            return;
        }
        else
        {
            if(n+strlen(buf) < sizeof(r->text))
                strcpy(r->text+n, buf);
        }
    }
    update_entry_text(wm, r);
}
 
