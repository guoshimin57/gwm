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

#define ENTRY_EVENT_MASK (ButtonPressMask|KeyPressMask|ExposureMask)

struct _entry_tag // 輸入構件
{
    Widget base;
    wchar_t text[BUFSIZ]; // 構件上的文字
    const char *hint; // 構件的提示文字
    size_t cursor_offset; // 光標偏移字符數
    XIC xic; // 輸入法句柄
    Strings *(*complete)(Entry *, int *); // 補全函數
};

static int get_entry_cursor_x(Entry *entry);
static char *get_part_match_regex(Entry *entry);
static void complete_for_entry(Entry *entry, bool show);
static void hide_entry(Entry *entry);
static Strings *get_cmd_completion_for_entry(Entry *entry, int *n);

Entry *cmd_entry=NULL; // 輸入命令並執行的構件

Entry *create_entry(Widget_id id, Window parent, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *, int *))
{
    Entry *entry=malloc_s(sizeof(Entry));
    init_widget(WIDGET(entry), id, ENTRY_TYPE, WIDGET_STATE_1(current), parent, x, y, w, h);
    entry->text[0]=L'\0', entry->hint=hint, entry->cursor_offset=0;
    entry->complete=complete;
    XSelectInput(xinfo.display, WIDGET_WIN(entry), ENTRY_EVENT_MASK);
    set_xic(WIDGET_WIN(entry), &entry->xic);
    return entry;
}

wchar_t *get_entry_text(Entry *entry)
{
    return entry->text;
}

void show_entry(Entry *entry)
{
    show_widget(WIDGET(entry));
    entry->text[0]=L'\0', entry->cursor_offset=0;
    update_entry_fg(entry);
    XGrabKeyboard(xinfo.display, WIDGET_WIN(entry), False,
        GrabModeAsync, GrabModeAsync, CurrentTime);
}

void update_entry_bg(Entry *entry)
{
    update_widget_bg(WIDGET(entry));
    XSetWindowBorder(xinfo.display, WIDGET_WIN(entry),
        get_widget_color(WIDGET_STATE(entry)));
}

void update_entry_fg(Entry *entry)
{
    int x=get_entry_cursor_x(entry);
    bool empty = entry->text[0]==L'\0';
    Str_fmt fmt={0, 0, WIDGET_W(entry), WIDGET_H(entry), CENTER_LEFT, true, false, 0,
        get_widget_fg(WIDGET_STATE(entry))};

    if(empty)
        draw_string(WIDGET_WIN(entry), entry->hint, &fmt);
    else
        draw_wcs(WIDGET_WIN(entry), entry->text, &fmt);

    GC gc=XCreateGC(xinfo.display, WIDGET_WIN(entry), 0, NULL);
    XDrawLine(xinfo.display, WIDGET_WIN(entry), gc, x, 0, x, WIDGET_H(entry));
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
                None, WIDGET_WIN(entry), ke->time);
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
    update_entry_fg(entry);
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
        int i, bw=cfg->border_width, w=WIDGET_W(entry), h=WIDGET_H(entry),
            x=WIDGET_X(entry)+bw, y=WIDGET_Y(entry)+h+2*bw,
            max=(xinfo.screen_height-y)/h;
        Str_fmt fmt={0, 0, w, h, CENTER_LEFT, true, false, 0,
            get_widget_fg(WIDGET_STATE(entry))};

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
    hide_widget(WIDGET(entry));
    XUnmapWindow(xinfo.display, xinfo.hint_win);
}

void destroy_entry(Entry *entry)
{
    if(entry->xic)
        XDestroyIC(entry->xic);
    destroy_widget(WIDGET(entry));
}

void paste_for_entry(Entry *entry)
{
    char *p=(char *)get_prop(WIDGET_WIN(entry), get_utf8_string_atom(), NULL);
    wchar_t text[BUFSIZ];
    int n=mbstowcs(text, p, BUFSIZ);
    XFree(p);
    if(n <= 0)
        return;

    wchar_t *src=entry->text+entry->cursor_offset, *dest=src+n;
    wmemmove(dest, src, wcslen(entry->text)-entry->cursor_offset);
    wcsncpy(src, text, n);
    entry->cursor_offset += n;
    update_entry_fg(entry);
}

Entry *create_cmd_entry(Widget_id id)
{
    int sw=xinfo.screen_width, sh=xinfo.screen_height, bw=cfg->border_width,
        x, y, w, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(cfg->cmd_entry_hint, &w, NULL);
    w += 2*pad, w = (w>=sw/4 && w<=sw-2*bw) ? w : sw/4;
    x=(sw-w)/2-bw, y=(sh-h)/2-bw;

    return create_entry(id, xinfo.root_win, x, y, w, h, cfg->cmd_entry_hint,
        get_cmd_completion_for_entry);
}

static Strings *get_cmd_completion_for_entry(Entry *entry, int *n)
{
    char *regex=get_part_match_regex(entry);
    char *paths=getenv("PATH");
    Strings *cmds=get_files_in_paths(paths, regex, RISE, false, n);
    free_s(regex);
    return cmds;
}
