/* *************************************************************************
 *     iconbar.c：實現縮微窗口欄相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "widget.h"
#include "button.h"
#include "tooltip.h"
#include "image.h"
#include "misc.h"
#include "list.h"
#include "iconbar.h"

typedef struct // 縮微窗口按鈕
{
    Button *button;
    Window cwin; // 縮微客戶窗口
    List list;
} Cbutton;

struct iconbar_tag // 縮微窗口欄
{
    Widget base;
    Cbutton *cbuttons;
};

static Cbutton *cbutton_new(Widget *parent, int x, int y, int w, int h, Window cwin);
static void cbutton_del(Cbutton *cbutton);
static void cbutton_ctor(Cbutton *cbutton, Widget *parent, int x, int y, int w, int h, Window cwin);
static void cbutton_dtor(Cbutton *cbutton);
static void cbutton_set_icon(Cbutton *cbutton);
static Cbutton *iconbar_find_cbutton(const Iconbar *iconbar, Window cwin);
static void iconbar_ctor(Iconbar *iconbar, Widget *parent, Widget_state state, int x, int y, int w, int h);
static void iconbar_dtor(Iconbar *iconbar);
static bool iconbar_has_similar_cbutton(Iconbar *iconbar, const Cbutton *cbutton);

static Cbutton *cbutton_new(Widget *parent, int x, int y, int w, int h, Window cwin)
{
    Cbutton *cbutton=Malloc(sizeof(Cbutton));
    cbutton_ctor(cbutton, parent, x, y, w, h, cwin);
    return cbutton;
}

static void cbutton_del(Cbutton *cbutton)
{
    cbutton_dtor(cbutton);
    free(cbutton);
}

static void cbutton_ctor(Cbutton *cbutton, Widget *parent, int x, int y, int w, int h, Window cwin)
{
    char *icon_title=get_icon_title_text(cwin, "");

    cbutton->button=button_new(parent, CLIENT_ICON, WIDGET_STATE_1(current),
        x, y, w, h, icon_title);
    button_set_align(cbutton->button, CENTER_LEFT);

    cbutton->cwin=cwin;

    cbutton_set_icon(cbutton);
    WIDGET_TOOLTIP(cbutton->button)=(Widget *)tooltip_new(WIDGET(cbutton->button), icon_title);
    free(icon_title);
}

static void cbutton_dtor(Cbutton *cbutton)
{
    button_del(cbutton->button);
    cbutton->button=NULL;
}

static void cbutton_set_icon(Cbutton *cbutton)
{
    XClassHint class_hint={NULL, NULL};

    XGetClassHint(xinfo.display, cbutton->cwin, &class_hint);
    Imlib_Image image=get_icon_image(cbutton->cwin, class_hint.res_name,
        cfg->icon_image_size, cfg->cur_icon_theme);

    button_set_icon(cbutton->button, image, class_hint.res_name, NULL);
    vXFree(class_hint.res_name, class_hint.res_class);
}

Window iconbar_get_client_win(Iconbar *iconbar, Window button_win)
{
    for(unsigned int i=0; i<DESKTOP_N; i++)
        list_for_each_entry(Cbutton, cb, &iconbar->cbuttons->list, list)
            if(WIDGET_WIN(cb->button) == button_win)
                return cb->cwin;
    return None;
}

static Cbutton *iconbar_find_cbutton(const Iconbar *iconbar, Window cwin)
{
    list_for_each_entry(Cbutton, p, &iconbar->cbuttons->list, list)
        if(p->cwin == cwin)
            return p;
    return NULL;
}

Iconbar *iconbar_new(Widget *parent, Widget_state state, int x, int y, int w, int h)
{
    Iconbar *iconbar=Malloc(sizeof(Iconbar));
    iconbar_ctor(iconbar, parent, state, x, y, w, h);
    return iconbar;
}

static void iconbar_ctor(Iconbar *iconbar, Widget *parent, Widget_state state, int x, int y, int w, int h)
{
    widget_ctor(WIDGET(iconbar), parent, ICONBAR, state, x, y, w, h);
    iconbar->cbuttons=Malloc(sizeof(Cbutton));
    list_init(&iconbar->cbuttons->list);
}

void iconbar_del(Iconbar *iconbar)
{
    iconbar_dtor(iconbar);
    widget_del(WIDGET(iconbar));
}

static void iconbar_dtor(Iconbar *iconbar)
{
    list_for_each_entry_safe(Cbutton, c, &iconbar->cbuttons->list, list)
        cbutton_del(c);
    Free(iconbar->cbuttons);
}

void iconbar_add_cbutton(Iconbar *iconbar, Window cwin)
{
    int h=WIDGET_H(iconbar), w=h;
    Cbutton *c=cbutton_new(WIDGET(iconbar), 0, 0, w, h, cwin);
    list_add(&c->list, &iconbar->cbuttons->list);
    iconbar_update(iconbar);
    WIDGET(c->button)->show(WIDGET(c->button));
}

void iconbar_del_cbutton(Iconbar *iconbar, Window cwin)
{
    list_for_each_entry_safe(Cbutton, c, &iconbar->cbuttons->list, list)
    {
        if(c->cwin == cwin)
        {
            list_del(&c->list);
            cbutton_del(c);
            iconbar_update(iconbar);
            break;
        }
    }
}

void iconbar_update(Iconbar *iconbar)
{
    int x=0, w=0, h=WIDGET_H(iconbar), wi=h, wl=0;

    list_for_each_entry(Cbutton, c, &iconbar->cbuttons->list, list)
    {
        Button *b=c->button;
        if(iconbar_has_similar_cbutton(iconbar, c))
        {
            get_string_size(button_get_label(b), &wl, NULL);
            w=MIN(wi+wl, cfg->icon_win_width_max);
        }
        else
            w=wi;
        widget_move_resize(WIDGET(b), x, WIDGET_Y(b), w, WIDGET_H(b)); 
        x+=w+cfg->icon_gap;
    }
}

static bool iconbar_has_similar_cbutton(Iconbar *iconbar, const Cbutton *cbutton)
{
    XClassHint ch={NULL, NULL}, ph;
    if(!XGetClassHint(xinfo.display, cbutton->cwin, &ch))
        return false;

    bool same=false;
    list_for_each_entry(Cbutton, p, &iconbar->cbuttons->list, list)
    {
        if(p!=cbutton && XGetClassHint(xinfo.display, p->cwin, &ph))
        {
            same = (strcmp(ph.res_class, ch.res_class) == 0
                && strcmp(ph.res_name, ch.res_name) == 0);
            vXFree(ph.res_class, ph.res_name);
            if(same)
                break;
        }
    }
    
    vXFree(ch.res_class, ch.res_name);

    return same;
}

void iconbar_update_by_state(Iconbar *iconbar, Window cwin)
{
    Net_wm_state state=get_net_wm_state(cwin);
    
    if(iconbar_find_cbutton(iconbar, cwin))
    {
        if(!state.hidden)
            iconbar_del_cbutton(iconbar, cwin);
    }
    else if(state.hidden)
        iconbar_add_cbutton(iconbar, cwin);
}

void iconbar_update_by_icon_name(Iconbar *iconbar, Window cwin, const char *icon_name)
{
    Cbutton *cbutton=iconbar_find_cbutton(iconbar, cwin);
    if(cbutton == NULL)
        return;

    button_set_label(cbutton->button, icon_name);
    iconbar_update(iconbar);
}

void iconbar_update_by_icon(Iconbar *iconbar, Window cwin, Imlib_Image image)
{
    Cbutton *cbutton=iconbar_find_cbutton(iconbar, cwin);
    if(cbutton == NULL)
        return;

    button_change_icon(cbutton->button, image, NULL, NULL);
    button_update_fg(WIDGET(cbutton->button));
}
