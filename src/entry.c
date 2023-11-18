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

static int get_entry_cursor_x(Entry *e);
static void hint_for_run_cmd_entry(WM *wm, const char *regex);
static char *get_match_cmd(const char *regex);
static char *get_part_match_regex(Entry *e);
static void complete_cmd_for_entry(Entry *e);
static bool close_entry(WM *wm, Entry *e, bool result);

Entry *create_entry(WM *wm, Rect *r, const char *hint)
{
    Entry *e=wm->run_cmd=malloc_s(sizeof(Entry));
    e->x=r->x, e->y=r->y, e->w=r->w, e->h=r->h, e->hint=hint;
    e->win=create_widget_win(xinfo.root_win, e->x, e->y, e->w, e->h,
        cfg->border_width, WIDGET_COLOR(wm, CURRENT_BORDER), 
        WIDGET_COLOR(wm, ENTRY));
    XSelectInput(xinfo.display, e->win, ENTRY_EVENT_MASK);
    set_xic(e->win, &e->xic);
    return e;
}

void show_entry(WM *wm, Entry *e)
{
    e->text[0]=L'\0', e->cursor_offset=0;
    XRaiseWindow(xinfo.display, e->win);
    XMapWindow(xinfo.display, e->win);
    update_entry_text(wm, e);
    XGrabKeyboard(xinfo.display, e->win, False, GrabModeAsync, GrabModeAsync, CurrentTime);
}

void update_entry_text(WM *wm, Entry *e)
{
    int x=get_entry_cursor_x(e);
    bool empty = e->text[0]==L'\0';
    XftColor color = empty ? TEXT_COLOR(wm, HINT) : TEXT_COLOR(wm, ENTRY);
    String_format f={{0, 0, e->w, e->h}, CENTER_LEFT, false, true, false, 0,
        color};

    if(empty)
        draw_string(wm, e->win, e->hint, &f);
    else
        draw_wcs(wm, e->win, e->text, &f);
    XDrawLine(xinfo.display, e->win, wm->gc, x, 0, x, e->h);
}

static int get_entry_cursor_x(Entry *e)
{
    size_t n=e->cursor_offset*MB_CUR_MAX+1;
    int w=0;
    wchar_t wc=e->text[e->cursor_offset]; 
    char mbs[n]; 

    e->text[e->cursor_offset]=L'\0'; 
    if(wcstombs(mbs, e->text, n) != (size_t)-1)
        get_string_size(mbs, &w, NULL);
    e->text[e->cursor_offset]=wc; 
    return get_font_pad()+w;
}

static void hint_for_run_cmd_entry(WM *wm, const char *regex)
{
    Window win=wm->hint_win;
    char *paths=getenv("PATH");

    if(paths && regex && strcmp(regex, "*"))
    {
        int bw=cfg->border_width, w=wm->run_cmd->w, h=wm->run_cmd->h,
            x=wm->run_cmd->x+bw, y=wm->run_cmd->y+wm->run_cmd->h+2*bw,
            i, n, max=(wm->workarea.h-y)/h;
        String_format fmt={{0, 0, w, h}, CENTER_LEFT, false, true, false, 0,
            TEXT_COLOR(wm, HINT)};

        File *f, *files=get_files_in_paths(paths, regex, RISE, false, &n);
        if(n)
        {
            XMoveResizeWindow(xinfo.display, win, x, y, w, MIN(n, max)*h);
            XMapWindow(xinfo.display, win);
            XClearWindow(xinfo.display, win);
            for(i=0, f=files->next; f && i<max; f=f->next, i++)
                draw_string(wm, win, i<max-1 ? f->name : "...", &fmt), fmt.r.y+=h;
            free_files(files);
            return;
        }
    }
    XUnmapWindow(xinfo.display, win);
}

static char *get_match_cmd(const char *regex)
{
    char *cmd=NULL, *paths=getenv("PATH");
    if(!paths)
        return NULL;

    File *files=get_files_in_paths(paths, regex, RISE, false, NULL);
    if(files->next)
        cmd=copy_string(files->next->name), free_files(files);
    return cmd;
}

bool input_for_entry(WM *wm, Entry *e, XKeyEvent *ke)
{
    wchar_t *s=e->text, keyname[FILENAME_MAX]={0};
    size_t *i=&e->cursor_offset, no=wcslen(s+*i);
    KeySym ks=look_up_key(e->xic, ke, keyname, FILENAME_MAX);
    size_t n1=wcslen(s), n2=wcslen(keyname), n=ARRAY_NUM(e->text);

    if(is_equal_modifier_mask(ControlMask, ke->state))
    {
        if(ks == XK_u)
            wmemmove(s, s+*i, no+1), *i=0;
        else if(ks == XK_v)
            XConvertSelection(xinfo.display, XA_PRIMARY, get_utf8_string_atom(),
                None, e->win, ke->time);
    }
    else if(is_equal_modifier_mask(None, ke->state))
    {
        switch(ks)
        {
            case XK_Escape:    return close_entry(wm, e, false);
            case XK_Return:
            case XK_KP_Enter:  complete_cmd_for_entry(e); return close_entry(wm, e, true);
            case XK_BackSpace: if(n1) wmemmove(s+*i-1, s+*i, no+1), --*i; break;
            case XK_Delete:
            case XK_KP_Delete: if(n1 < n) wmemmove(s+*i, s+*i+1, no+1); break;
            case XK_Left:
            case XK_KP_Left:   *i = *i ? *i-1 : *i; break;
            case XK_Right:
            case XK_KP_Right:  *i = *i<n1 ? *i+1 : *i; break;
            case XK_Home:      (*i)=0; break;
            case XK_End:       (*i)=n1; break;
            case XK_Tab:       complete_cmd_for_entry(e); break;
            default:           wcsncpy(s+n1, keyname, n-n1-1), (*i)+=n2;
        }
    }
    else if(is_equal_modifier_mask(ShiftMask, ke->state))
        wcsncpy(s+n1, keyname, n-n1-1), (*i)+=n2;

    char *regex=get_part_match_regex(e);
    hint_for_run_cmd_entry(wm, regex), free(regex);
    update_entry_text(wm, e);
    return false;
}

static void complete_cmd_for_entry(Entry *e)
{
    if(!e->text[0])
       return;

    char *regex=get_part_match_regex(e), *cmd=get_match_cmd(regex);
    if(cmd)
        mbstowcs(e->text, cmd, FILENAME_MAX), e->cursor_offset=wcslen(e->text);
    vfree(regex, cmd, NULL);
}

static char *get_part_match_regex(Entry *e)
{
    char text[FILENAME_MAX]={0};
    wcstombs(text, e->text, FILENAME_MAX);
    return copy_strings(text, "*", NULL);
}

static bool close_entry(WM *wm, Entry *e, bool result)
{
    XUngrabKeyboard(xinfo.display, CurrentTime);
    XUnmapWindow(xinfo.display, e->win);
    XUnmapWindow(xinfo.display, wm->hint_win);
    return result;
}

void paste_for_entry(WM *wm, Entry *e)
{
    char *p=(char *)get_prop(e->win, get_utf8_string_atom(), NULL);
    wchar_t text[BUFSIZ];
    int n=mbstowcs(text, p, BUFSIZ);
    XFree(p);
    if(n <= 0)
        return;

    wchar_t *src=e->text+e->cursor_offset, *dest=src+n;
    wmemmove(dest, src, wcslen(e->text)-e->cursor_offset);
    wcsncpy(src, text, n);
    e->cursor_offset += n;
    update_entry_text(wm, e);
}
