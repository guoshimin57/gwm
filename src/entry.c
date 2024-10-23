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

#include <wchar.h>
#include "gwm.h"
#include "config.h"
#include "listview.h"
#include "file.h"
#include "font.h"
#include "icccm.h"
#include "prop.h"
#include "entry.h"

#define ENTRY_EVENT_MASK (ButtonPressMask|KeyPressMask|ExposureMask)

struct _entry_tag // 輸入構件
{
    Widget base;
    wchar_t text[BUFSIZ]; // 構件上的文字
    const char *hint; // 構件的提示文字
    size_t cursor_offset; // 光標偏移字符數
    XIC xic; // 輸入法句柄
    Strings *(*complete)(Entry *entry); // 補全函數
    Listview *listview; // 用於展示補全結果
};

static void entry_ctor(Entry *entry, Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *));
static void entry_clear(Entry *entry);
static void entry_set_method(Widget *widget);
static void entry_dtor(Entry *entry);
static int entry_get_cursor_x(Entry *entry);
static char *entry_get_part_match_regex(Entry *entry);
static void entry_complete(Entry *entry, bool show);
static Strings *entry_get_cmd_completion(Entry *entry);

Entry *cmd_entry=NULL; // 輸入命令並執行的構件

Entry *entry_new(Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *))
{
    Entry *entry=Malloc(sizeof(Entry));
    entry_ctor(entry, parent, id, x, y, w, h, hint, complete);
    return entry;
}

static void entry_ctor(Entry *entry, Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *))
{
    widget_ctor(WIDGET(entry), parent, id, WIDGET_STATE_1(current), x, y, w, h);
    entry_set_method(WIDGET(entry));
    entry->hint=hint;
    entry->complete=complete;
    entry->listview=listview_new(parent, UNUSED_WIDGET_ID, x, y+h, w, h, NULL);
    entry_clear(entry);
    XSelectInput(xinfo.display, WIDGET_WIN(entry), ENTRY_EVENT_MASK);
    set_xic(WIDGET_WIN(entry), &entry->xic);
}

static void entry_clear(Entry *entry)
{
    entry->text[0]=L'\0';
    entry->cursor_offset=0;
}

static void entry_set_method(Widget *widget)
{
    widget->show=entry_show;
    widget->hide=entry_hide;
    widget->update_bg=entry_update_bg;
    widget->update_fg=entry_update_fg;
}

void entry_del(Entry *entry)
{
    entry_dtor(entry);
    widget_del(WIDGET(entry));
}

static void entry_dtor(Entry *entry)
{
    if(entry->xic)
        XDestroyIC(entry->xic);
    widget_del(WIDGET(entry->listview));
}

void entry_show(Widget *widget)
{
    Entry *entry=ENTRY(widget);

    XRaiseWindow(xinfo.display, WIDGET_WIN(entry));
    widget_show(widget);
    XGrabKeyboard(xinfo.display, WIDGET_WIN(entry), False,
        GrabModeAsync, GrabModeAsync, CurrentTime);
}

void entry_hide(const Widget *widget)
{
    XUngrabKeyboard(xinfo.display, CurrentTime);
    widget_hide(WIDGET(ENTRY(widget)->listview));
    widget_hide(widget);
}

void entry_update_bg(const Widget *widget)
{
    widget_update_bg(widget);
    widget_set_border_color(widget, get_widget_color(WIDGET_STATE(widget)));
}

void entry_update_fg(const Widget *widget)
{
    Entry *entry=ENTRY(widget);
    int x=entry_get_cursor_x(entry);
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

wchar_t *entry_get_text(Entry *entry)
{
    return entry->text;
}

static int entry_get_cursor_x(Entry *entry)
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

bool entry_input(Entry *entry, XKeyEvent *ke)
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
        Widget *widget=WIDGET(entry);
        switch(ks)
        {
            case XK_Escape:    entry_hide(widget); entry_clear(entry); return false;
            case XK_Return:
            case XK_KP_Enter:  entry_complete(entry, false); entry_hide(widget); entry_clear(entry); return true;
            case XK_BackSpace: if(n1) wmemmove(s+*i-1, s+*i, no+1), --*i; break;
            case XK_Delete:
            case XK_KP_Delete: if(n1 < n) wmemmove(s+*i, s+*i+1, no+1); break;
            case XK_Left:
            case XK_KP_Left:   *i = *i ? *i-1 : *i; break;
            case XK_Right:
            case XK_KP_Right:  *i = *i<n1 ? *i+1 : *i; break;
            case XK_Home:      (*i)=0; break;
            case XK_End:       (*i)=n1; break;
            case XK_Tab:       entry_complete(entry, false); break;
            default:           wcsncpy(s+n1, keyname, n-n1-1), (*i)+=n2;
        }
    }
    else if(is_equal_modifier_mask(ShiftMask, ke->state))
        wcsncpy(s+n1, keyname, n-n1-1), (*i)+=n2;

    entry_complete(entry, true);
    entry_update_fg(WIDGET(entry));
    return false;
}

static char *entry_get_part_match_regex(Entry *entry)
{
    char text[FILENAME_MAX]={0};
    wcstombs(text, entry->text, FILENAME_MAX);
    return copy_strings(text, ".*", NULL);
}

static void entry_complete(Entry *entry, bool show)
{
    Strings *strs=NULL;
    if( !entry->text[0] || !entry->complete
        || !(strs=entry->complete(entry)) || list_is_empty(&strs->list))
    {
        widget_hide(WIDGET(entry->listview));
        return;
    }

    if(show)
        listview_update(entry->listview, strs);
    else
    {
        Strings *first=list_first_entry(&strs->list, Strings, list);
        mbstowcs(entry->text, first->str, FILENAME_MAX);
        entry->cursor_offset=wcslen(entry->text);
    }
    vfree_strings(strs);
}

void entry_paste(Entry *entry)
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
    entry_update_fg(WIDGET(entry));
}

Entry *cmd_entry_new(Widget_id id)
{
    int sw=xinfo.screen_width, sh=xinfo.screen_height, bw=cfg->border_width,
        x, y, w, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(cfg->cmd_entry_hint, &w, NULL);
    w += 2*pad, w = (w>=sw/4 && w<=sw-2*bw) ? w : sw/4;
    x=(sw-w)/2-bw, y=(sh-h)/2-bw;

    Entry *entry=entry_new(NULL, id, x, y, w, h, cfg->cmd_entry_hint,
        entry_get_cmd_completion);
    listview_set_nmax(entry->listview, (sh-y-h-h)/h);

    return entry;
}

static Strings *entry_get_cmd_completion(Entry *entry)
{
    char *regex=entry_get_part_match_regex(entry);
    char *paths=getenv("PATH");
    Strings *cmds=get_files_in_paths(paths, regex, false);
    Free(regex);
    return cmds;
}
