/* *************************************************************************
 *     entry.c：實現單行文本輸入框功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <wchar.h>
#include <X11/Xatom.h>
#include "gwm.h"
#include "config.h"
#include "file.h"
#include "font.h"
#include "icccm.h"
#include "prop.h"
#include "entry.h"

#define ENTRY_EVENT_MASK (ButtonPressMask|KeyPressMask|ExposureMask)
#define ENTRY_TEXT_SIZE BUFSIZ

struct _entry_tag // 輸入構件
{
    Widget base;
    wchar_t text[ENTRY_TEXT_SIZE]; // 構件上的文字
    const char *hint; // 構件的提示文字
    size_t cursor_offset; // 光標偏移字符數
    XIC xic; // 輸入法句柄
    Strings *(*complete)(Entry *entry); // 補全函數
    Listview *listview; // 用於展示補全結果
};

static void entry_ctor(Entry *entry, Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *));
static void entry_set_method(Widget *widget);
static void entry_dtor(Entry *entry);
static int entry_get_cursor_x(Entry *entry);
static void entry_input_ctrl_seq(Entry *entry, XKeyEvent *ke, KeySym ks);
static void insert_wcs(wchar_t *src, size_t size, size_t *offset, const wchar_t *ins);
static void entry_complete(Entry *entry, bool show);

Entry *entry_new(Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *))
{
    Entry *entry=Malloc(sizeof(Entry));
    entry_ctor(entry, parent, id, x, y, w, h, hint, complete);
    return entry;
}

static void entry_ctor(Entry *entry, Widget *parent, Widget_id id, int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *))
{
    widget_ctor(WIDGET(entry), parent, WIDGET_TYPE_ENTRY, id, x, y, w, h);
    entry_set_method(WIDGET(entry));
    entry->hint=hint;
    entry->complete=complete;
    entry->listview=listview_new(parent, UNUSED_WIDGET_ID, x, y+h, w, h, NULL);
    widget_set_poppable(WIDGET(entry->listview), true);
    entry_clear(entry);
    XSelectInput(xinfo.display, WIDGET_WIN(entry), ENTRY_EVENT_MASK);
    set_xic(WIDGET_WIN(entry), &entry->xic);
}

void entry_clear(Entry *entry)
{
    wmemset(entry->text, L'\0', ENTRY_TEXT_SIZE);
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
    widget_set_border_color(widget, get_widget_color(widget));
}

void entry_update_fg(const Widget *widget)
{
    Entry *entry=ENTRY(widget);
    int x=entry_get_cursor_x(entry);
    Str_fmt fmt={0, 0, WIDGET_W(entry), WIDGET_H(entry), CENTER_LEFT, true, false, 0,
        get_text_color(widget)};

    if(wcslen(entry->text) == 0)
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
    size_t ns=wcslen(s), n=ARRAY_NUM(entry->text);

    if(ns == 0)
    {
        WIDGET(entry)->state.active=1;
        WIDGET(entry)->update_bg(WIDGET(entry));
    }

    if(is_equal_modifier_mask(ControlMask, ke->state))
        entry_input_ctrl_seq(entry, ke, ks);
    else if(is_equal_modifier_mask(None, ke->state))
    {
        Widget *widget=WIDGET(entry);
        switch(ks)
        {
            case XK_Escape:    entry_hide(widget); entry_clear(entry); return false;
            case XK_Return:
            case XK_KP_Enter:  entry_complete(entry, false); entry_hide(widget); return true;
            case XK_BackSpace: if(ns) wmemmove(s+*i-1, s+*i, no+1), --*i; break;
            case XK_Delete:
            case XK_KP_Delete: if(ns < n) wmemmove(s+*i, s+*i+1, no+1); break;
            case XK_Left:
            case XK_KP_Left:   *i = *i ? *i-1 : *i; break;
            case XK_Right:
            case XK_KP_Right:  *i = *i<ns ? *i+1 : *i; break;
            case XK_Home:      (*i)=0; break;
            case XK_End:       (*i)=ns; break;
            case XK_Tab:       entry_complete(entry, false); break;
            default:           insert_wcs(s, n, i, keyname);
        }
    }
    else if(is_equal_modifier_mask(ShiftMask, ke->state))
        insert_wcs(s, n, i, keyname);

    entry_complete(entry, true);
    entry_update_fg(WIDGET(entry));
    return false;
}

static void entry_input_ctrl_seq(Entry *entry, XKeyEvent *ke, KeySym ks)
{
    if(ks == XK_u)
    {
        wchar_t *ins=entry->text+entry->cursor_offset;
        wmemmove(entry->text, ins, wcslen(ins)+1);
        entry->cursor_offset=0;
    }
    else if(ks == XK_v)
        XConvertSelection(xinfo.display, XA_PRIMARY, get_utf8_string_atom(),
            None, WIDGET_WIN(entry), ke->time);
}

static void insert_wcs(wchar_t *src, size_t size, size_t *offset, const wchar_t *ins)
{
    size_t ns=wcslen(src), ni=wcslen(ins), i=*offset;
    wmemmove(src+i+ni, src+i, ns-i);
    wcsncpy(src+i, ins, ni < size-i ? ni : size-i);
    *offset = i + (ni < size-i ? ni : size-i);
}

static void entry_complete(Entry *entry, bool show)
{
    Strings *strs=NULL;
    if( !entry->text[0] || !entry->complete
        || !(strs=entry->complete(entry)) || LIST_IS_EMPTY(strs))
    {
        widget_hide(WIDGET(entry->listview));
        return;
    }

    if(show)
        listview_update(entry->listview, strs);
    else
    {
        Strings *first=LIST_FIRST(Strings, strs);
        mbstowcs(entry->text, first->str, FILENAME_MAX);
        entry->cursor_offset=wcslen(entry->text);
    }
    vfree_strings(strs);
}

void entry_paste(Entry *entry)
{
    char *p=get_utf8_string_prop(WIDGET_WIN(entry), get_utf8_string_atom());
    wchar_t text[ENTRY_TEXT_SIZE];
    int n=mbstowcs(text, p, ENTRY_TEXT_SIZE);
    XFree(p);
    if(n <= 0)
        return;

    wchar_t *src=entry->text+entry->cursor_offset, *dest=src+n;
    wmemmove(dest, src, wcslen(entry->text)-entry->cursor_offset);
    wcsncpy(src, text, n);
    entry->cursor_offset += n;
    entry_update_fg(WIDGET(entry));
}

Listview *entry_get_listview(const Entry *entry)
{
    return entry->listview;
}
