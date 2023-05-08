/* *************************************************************************
 *     entry.c：實現單行文本輸入框功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static int get_entry_cursor_x(WM *wm, Entry *e);
static void hint_for_run_cmd_entry(WM *wm, const char *pattern);
static char *get_match_cmd(const char *pattern);
static void complete_cmd_for_entry(Entry *e);
static bool close_entry(WM *wm, Entry *e, bool result);

Entry *create_entry(WM *wm, Rect *r, const char *hint)
{
    Entry *e=wm->run_cmd=malloc_s(sizeof(Entry));
    e->x=r->x, e->y=r->y, e->w=r->w, e->h=r->h;
    e->win=XCreateSimpleWindow(wm->display, wm->root_win, e->x, e->y,
        e->w, e->h, wm->cfg->border_width,
        wm->widget_color[wm->cfg->color_theme][CURRENT_BORDER_COLOR].pixel,
        wm->widget_color[wm->cfg->color_theme][ENTRY_COLOR].pixel);
    set_override_redirect(wm, e->win);
    XSelectInput(wm->display, e->win, ENTRY_EVENT_MASK);
    e->hint=hint;
    set_xic(wm, e->win, &e->xic);
    return e;
}

void show_entry(WM *wm, Entry *e)
{
    e->text[0]=L'\0', e->cursor_offset=0;
    XRaiseWindow(wm->display, e->win);
    XMapWindow(wm->display, e->win);
    update_entry_text(wm, e);
    XGrabKeyboard(wm->display, e->win, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void update_entry_text(WM *wm, Entry *e)
{
    String_format f={{wm->cfg->entry_text_indent, 0, e->w-2*wm->cfg->entry_text_indent, e->h},
        CENTER_LEFT, false, 0,
        e->text[0]==L'\0' ? wm->text_color[wm->cfg->color_theme][HINT_TEXT_COLOR] :
        wm->text_color[wm->cfg->color_theme][ENTRY_TEXT_COLOR], ENTRY_FONT};
    int x=get_entry_cursor_x(wm, e);
    XClearArea(wm->display, e->win, 0, 0, e->w, e->h, False); 
    if(e->text[0] == L'\0')
        draw_string(wm, e->win, e->hint, &f);
    else
        draw_wcs(wm, e->win, e->text, &f);
    XDrawLine(wm->display, e->win, wm->gc, x, 0, x, e->h);
}

static int get_entry_cursor_x(WM *wm, Entry *e)
{
    unsigned int n=e->cursor_offset*MB_CUR_MAX+1, w=0;
    wchar_t wc=e->text[e->cursor_offset]; 
    char mbs[n]; 
    e->text[e->cursor_offset]=L'\0'; 
    if(wcstombs(mbs, e->text, n) != (size_t)-1)
        get_string_size(wm, wm->font[ENTRY_FONT], mbs, &w, NULL);
    e->text[e->cursor_offset]=wc; 
    return wm->cfg->entry_text_indent+w;
}

static void hint_for_run_cmd_entry(WM *wm, const char *pattern)
{
    Window win=wm->hint_win;
    char *paths=getenv("PATH");
    if(paths && pattern && *pattern)
    {
        unsigned int w=wm->cfg->run_cmd_entry_width, h=wm->cfg->hint_win_line_height;
        String_format fmt={{0, 0, w, h}, CENTER_LEFT,
            false, 0, wm->text_color[wm->cfg->color_theme][HINT_TEXT_COLOR], HINT_FONT};
        int x=wm->run_cmd->x+wm->cfg->border_width, y=wm->run_cmd->y+wm->run_cmd->h+wm->cfg->border_width, *py=&fmt.r.y;
        size_t i, n, max=(wm->screen_height-wm->taskbar->h-y)/h;
        const char *reg=copy_strings(pattern, "*", NULL);
        File *f, *files=get_files_in_paths(paths, reg, RISE, false, &n);
        if(n)
        {
            XMoveResizeWindow(wm->display, win, x, y, w, MIN(n, max)*h);
            XMapWindow(wm->display, win);
            XClearWindow(wm->display, win);
            for(i=0, f=files->next; f && i<max; f=f->next, i++)
                draw_string(wm, win, i<max-1 ? f->name : "...", &fmt), *py+=h;
            free_files(files);
            return;
        }
    }
    XUnmapWindow(wm->display, win);
}

static char *get_match_cmd(const char *pattern)
{
    char *cmd=NULL, *paths=getenv("PATH");
    if(paths)
    {
        char *reg=copy_strings(pattern, "*", NULL);
        File *files=get_files_in_paths(paths, reg, RISE, false, NULL);
        if(files->next)
            cmd=copy_string(files->next->name), free_files(files);
    }
    return cmd;
}

bool input_for_entry(WM *wm, Entry *e, XKeyEvent *ke)
{
    size_t *i=&e->cursor_offset, no=wcslen(e->text+*i);
    wchar_t keyname[FILENAME_MAX]={0};
    KeySym ks=look_up_key(e->xic, ke, keyname, FILENAME_MAX);
    size_t n1=wcslen(e->text), n2=wcslen(keyname), n=ARRAY_NUM(e->text);

    if(is_equal_modifier_mask(wm, ControlMask, ke->state))
    {
        if(ks == XK_u)
            wmemmove(e->text, e->text+*i, no+1), *i=0;
        else if(ks == XK_v)
            XConvertSelection(wm->display, XA_PRIMARY, wm->utf8,
                None, e->win, ke->time);
    }
    else if(is_equal_modifier_mask(wm, None, ke->state))
    {
        switch(ks)
        {
            case XK_Escape:     return close_entry(wm, e, false);
            case XK_Return:
            case XK_KP_Enter:   complete_cmd_for_entry(e); return close_entry(wm, e, true);
            case XK_BackSpace:
                if(n1)
                    wmemmove(e->text+*i-1, e->text+*i, no+1), (*i)--;
                break;
            case XK_Delete:
            case XK_KP_Delete:
                if(n1 < n)
                    wmemmove(e->text+*i, e->text+*i+1, no+1);
                break;
            case XK_Left:
            case XK_KP_Left:    *i = *i ? *i-1 : *i; break;
            case XK_Right:
            case XK_KP_Right:   *i = *i<n1 ? *i+1 : *i; break;
            case XK_Home:       (*i)=0; break;
            case XK_End:        (*i)=n1; break;
            case XK_Tab:        complete_cmd_for_entry(e); break;
            default:            wcsncpy(e->text+n1, keyname, n-n1-1), (*i)+=n2;
        }
    }
    else if(is_equal_modifier_mask(wm, ShiftMask, ke->state))
        wcsncpy(e->text+n1, keyname, n-n1-1), (*i)+=n2;
    char pattern[FILENAME_MAX]={0};
    wcstombs(pattern, e->text, FILENAME_MAX);
    hint_for_run_cmd_entry(wm, pattern);
    update_entry_text(wm, e);
    return false;
}

static void complete_cmd_for_entry(Entry *e)
{
    char *cmd, pattern[FILENAME_MAX]={0};
    wcstombs(pattern, e->text, FILENAME_MAX);
    if((cmd=get_match_cmd(pattern)))
        mbstowcs(e->text, cmd, FILENAME_MAX), e->cursor_offset=wcslen(e->text);
}

static bool close_entry(WM *wm, Entry *e, bool result)
{
    XUngrabKeyboard(wm->display, CurrentTime);
    XUnmapWindow(wm->display, e->win);
    XUnmapWindow(wm->display, wm->hint_win);
    return result;
}

void paste_for_entry(WM *wm, Entry *e)
{
    char *p;
    Atom da;
    int di;
    unsigned long dl;
    if(XGetWindowProperty(wm->display, e->win, wm->utf8, 0, BUFSIZ/4, True,
       wm->utf8, &da, &di, &dl, &dl, (unsigned char **)&p) == Success && p)
    {
        wchar_t text[BUFSIZ];
        int n=mbstowcs(text, p, BUFSIZ);
        XFree(p);
        if(n > 0)
        {
            wchar_t *src=e->text+e->cursor_offset, *dest=src+n;
            wmemmove(dest, src, wcslen(e->text)-e->cursor_offset);
            wcsncpy(src, text, n);
            e->cursor_offset += n;
            update_entry_text(wm, e);
        }
    }
}
