/* *************************************************************************
 *     entry.c：實現單行文本輸入框功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "misc.h"

#define ENTRY_EVENT_MASK (ButtonPressMask|KeyPressMask|ExposureMask)

static int get_entry_cursor_x(Entry *entry);
static char *get_part_match_regex(Entry *entry);
static void complete_for_entry(Entry *entry, bool show);
static void hide_entry(Entry *entry);
static Strings *get_cmd_completion_for_entry(Entry *entry, int *n);

Entry *cmd_entry=NULL; // 輸入命令並執行的構件

Entry *create_entry(Widget_type type, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *, int *))
{
    Entry *entry=malloc_s(sizeof(Entry));
    entry->x=x, entry->y=y, entry->w=w, entry->h=h;
    entry->hint=hint, entry->complete=complete;
    entry->win=create_widget_win(type, xinfo.root_win, x, y, w, h,
        cfg->border_width, get_widget_color(CURRENT_BORDER_COLOR),
        get_widget_color(ENTRY_COLOR));
    XSelectInput(xinfo.display, entry->win, ENTRY_EVENT_MASK);
    set_xic(entry->win, &entry->xic);
    return entry;
}

void show_entry(Entry *entry)
{
    entry->text[0]=L'\0', entry->cursor_offset=0;
    XMapRaised(xinfo.display, entry->win);
    update_entry_text(entry);
    XGrabKeyboard(xinfo.display, entry->win, False,
        GrabModeAsync, GrabModeAsync, CurrentTime);
}

void update_entry_bg(Entry *entry)
{
    update_win_bg(entry->win, get_widget_color(ENTRY_COLOR), None);
    XSetWindowBorder(xinfo.display, entry->win,
        get_widget_color(CURRENT_BORDER_COLOR));
}

void update_entry_text(Entry *entry)
{
    int x=get_entry_cursor_x(entry);
    bool empty = entry->text[0]==L'\0';
    Str_fmt fmt={0, 0, entry->w, entry->h, CENTER_LEFT, true, false, 0,
        get_text_color(empty ? HINT_TEXT_COLOR : ENTRY_TEXT_COLOR)};

    if(empty)
        draw_string(entry->win, entry->hint, &fmt);
    else
        draw_wcs(entry->win, entry->text, &fmt);

    GC gc=XCreateGC(xinfo.display, entry->win, 0, NULL);
    XDrawLine(xinfo.display, entry->win, gc, x, 0, x, entry->h);
}

static int get_entry_cursor_x(Entry *entry)
{
    size_t n=entry->cursor_offset*MB_CUR_MAX+1;
    int w=0;
    wchar_t wc=entry->text[entry->cursor_offset]; 
    char mbs[n]; 

    entry->text[entry->cursor_offset]=L'\0'; 
    if(wcstombs(mbs, entry->text, n) != (size_t)-1)
        get_string_size(mbs, &w, NULL);
    entry->text[entry->cursor_offset]=wc; 
    return get_font_pad()+w;
}

bool input_for_entry(Entry *entry, XKeyEvent *ke)
{
    wchar_t *s=entry->text, keyname[FILENAME_MAX]={0};
    size_t *i=&entry->cursor_offset, no=wcslen(s+*i);
    KeySym ks=look_up_key(entry->xic, ke, keyname, FILENAME_MAX);
    size_t n1=wcslen(s), n2=wcslen(keyname), n=ARRAY_NUM(entry->text);

    if(is_equal_modifier_mask(ControlMask, ke->state))
    {
        if(ks == XK_u)
            wmemmove(s, s+*i, no+1), *i=0;
        else if(ks == XK_v)
            XConvertSelection(xinfo.display, XA_PRIMARY, get_utf8_string_atom(),
                None, entry->win, ke->time);
    }
    else if(is_equal_modifier_mask(None, ke->state))
    {
        switch(ks)
        {
            case XK_Escape:    hide_entry(entry); return false;
            case XK_Return:
            case XK_KP_Enter:  complete_for_entry(entry, false); hide_entry(entry); return true;
            case XK_BackSpace: if(n1) wmemmove(s+*i-1, s+*i, no+1), --*i; break;
            case XK_Delete:
            case XK_KP_Delete: if(n1 < n) wmemmove(s+*i, s+*i+1, no+1); break;
            case XK_Left:
            case XK_KP_Left:   *i = *i ? *i-1 : *i; break;
            case XK_Right:
            case XK_KP_Right:  *i = *i<n1 ? *i+1 : *i; break;
            case XK_Home:      (*i)=0; break;
            case XK_End:       (*i)=n1; break;
            case XK_Tab:       complete_for_entry(entry, false); break;
            default:           wcsncpy(s+n1, keyname, n-n1-1), (*i)+=n2;
        }
    }
    else if(is_equal_modifier_mask(ShiftMask, ke->state))
        wcsncpy(s+n1, keyname, n-n1-1), (*i)+=n2;

    complete_for_entry(entry, true);
    update_entry_text(entry);
    return false;
}

static char *get_part_match_regex(Entry *entry)
{
    char text[FILENAME_MAX]={0};
    wcstombs(text, entry->text, FILENAME_MAX);
    return copy_strings(text, "*", NULL);
}

static void complete_for_entry(Entry *entry, bool show)
{
    if(entry->complete == NULL)
        return;

    int n=0;
    Strings *s=NULL, *strs=entry->complete(entry, &n);
    if(strs->next == NULL)
        return;

    if(show)
    {
        Window win=xinfo.hint_win;
        int i, bw=cfg->border_width, w=entry->w, h=entry->h, x=entry->x+bw,
            y=entry->y+entry->h+2*bw, max=(xinfo.screen_height-y)/h;
        Str_fmt fmt={0, 0, w, h, CENTER_LEFT, true, false, 0,
            get_text_color(HINT_TEXT_COLOR)};

        XMoveResizeWindow(xinfo.display, win, x, y, w, MIN(n, max)*h);
        XMapWindow(xinfo.display, win);
        XClearWindow(xinfo.display, win);
        for(i=0, s=strs->next; s && i<max; s = s->next, i++)
            draw_string(win, i<max-1 ? s->str : "...", &fmt), fmt.y+=h;
    }
    else
    {
        mbstowcs(entry->text, strs->next->str, FILENAME_MAX);
        entry->cursor_offset=wcslen(entry->text);
    }
    free_strings(strs);
}

static void hide_entry(Entry *entry)
{
    XUngrabKeyboard(xinfo.display, CurrentTime);
    XUnmapWindow(xinfo.display, entry->win);
    XUnmapWindow(xinfo.display, xinfo.hint_win);
}

void destroy_entry(Entry *entry)
{
    XDestroyWindow(xinfo.display, entry->win);
    if(entry->xic)
        XDestroyIC(entry->xic);
    free(entry);
}

void paste_for_entry(Entry *entry)
{
    char *p=(char *)get_prop(entry->win, get_utf8_string_atom(), NULL);
    wchar_t text[BUFSIZ];
    int n=mbstowcs(text, p, BUFSIZ);
    XFree(p);
    if(n <= 0)
        return;

    wchar_t *src=entry->text+entry->cursor_offset, *dest=src+n;
    wmemmove(dest, src, wcslen(entry->text)-entry->cursor_offset);
    wcsncpy(src, text, n);
    entry->cursor_offset += n;
    update_entry_text(entry);
}

Entry *create_cmd_entry(Widget_type type)
{
    int sw=xinfo.screen_width, sh=xinfo.screen_height, bw=cfg->border_width,
        x, y, w, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(cfg->cmd_entry_hint, &w, NULL);
    w += 2*pad, w = (w>=sw/4 && w<=sw-2*bw) ? w : sw/4;
    x=(sw-w)/2-bw, y=(sh-h)/2-bw;

    return create_entry(type, x, y, w, h, cfg->cmd_entry_hint,
        get_cmd_completion_for_entry);
}

static Strings *get_cmd_completion_for_entry(Entry *entry, int *n)
{
    char *regex=get_part_match_regex(entry);
    char *paths=getenv("PATH");
    Strings *cmds=get_files_in_paths(paths, regex, RISE, false, n);
    free(regex);
    return cmds;
}
