/* *************************************************************************
 *     drawable.c：實現與X可畫物相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <Imlib2.h>
#include <X11/Xatom.h>
#include "gwm.h"
#include "config.h"
#include "prop.h"
#include "ewmh.h"
#include "icccm.h"
#include "misc.h"
#include "drawable.h"

static Pixmap create_pixmap_with_color(Drawable d, unsigned long color);
static void change_prop_for_root_bg(Pixmap pixmap);

bool is_pointer_on_win(Window win)
{
    Window r, c;
    int rx, ry, x, y, w, h;
    unsigned int mask;
    
    return get_geometry(win, NULL, NULL, &w, &h, NULL, NULL)
        && XQueryPointer(xinfo.display, win, &r, &c, &rx, &ry, &x, &y, &mask)
        && x>=0 && x<w && y>=0 && y<h;
}

/* 通過求窗口與屏幕是否有交集來判斷窗口是否已經在屏幕外。
 * 若滿足以下條件，則有交集：窗口與屏幕中心距≤窗口半邊長+屏幕半邊長。
 * 即：|x+w/2-0-sw/2|＜|w/2+sw/2| 且 |y+h/2-0-sh/2|＜|h/2+sh/2|。
 * 兩邊同乘以2，得：|2*x+w-sw|＜|w+sw| 且 |2*y+h-sh|＜|h+sh|。
 */
bool is_on_screen(int x, int y, int w, int h)
{
    long sw=xinfo.screen_width, sh=xinfo.screen_height, wl=w, hl=h;
    return labs(2*x+wl-sw)<wl+sw && labs(2*y+hl-sh)<hl+sh;
}

void print_area(Drawable d, int x, int y, int w, int h)
{
    imlib_context_set_drawable(d);
    Imlib_Image image=imlib_create_image_from_drawable(None, x, y, w, h, 0);

    if(!image)
        return;

    time_t timer=time(NULL), err=-1;
    char name[FILENAME_MAX];

    if(cfg->screenshot_path[0] == '~')
        sprintf(name, "%s%s/gwm-", getenv("HOME"), cfg->screenshot_path+1);
    else
        sprintf(name, "%s/gwm-", cfg->screenshot_path);
    if(timer != err)
        strftime(name+strlen(name), FILENAME_MAX, "%Y_%m_%d_%H_%M_%S", localtime(&timer));
    set_visual_for_imlib(d);
    imlib_context_set_image(image);
    imlib_image_set_format(cfg->screenshot_format);
    sprintf(name+strlen(name), ".%s", cfg->screenshot_format);
    imlib_save_image(name);
    imlib_free_image();
}

/* 當存在合成器時，合成器會在根窗口上放置特效，即使用XSetWindowBackground*設置
 * 了背景，也會被合成器的特效擋着，目前還沒有標準的方法來設置背景。要給根窗口
 * 設置顏色，就得借助pixmap。這種情況下，若要真正地設置背景，得用E方法。這種方
 * 法是事實上的標準，它通過設置非標準的窗口特性_XROOTPMAP_ID和ESETROOT_PMAP_ID
 * 來達到此目的。通常設置其中之一便可，爲了保險起見，通常同時設置。這種方法的一
 * 個弊端是，不能在設置完背景後馬上釋放pixmap，否則背景設置失效，這會佔用一定的
 * 內存空間。修改背景時，應通過XKillClient來釋放舊的pixmap。通常_XROOTPMAP_ID和
 * ESETROOT_PMAP_ID特性指向相同的pixmap，有非標準化的文檔說兩者指向相同的pixmap
 * 時才應釋放它。詳見：
 *     https://metacpan.org/pod/X11::Protocol::XSetRoot
 *     https://lists.gnome.org/archives/wm-spec-list/2002-January/msg00003.html
 *     https://mail.gnome.org/archives/wm-spec-list/2002-January/msg00011.html
 */
void update_win_bg(Window win, unsigned long color, Pixmap pixmap)
{
    XEvent event={.xexpose={.type=Expose, .window=win}};
    bool compos_root = (win==xinfo.root_win && have_compositor());

    if(compos_root && !pixmap)
        pixmap=create_pixmap_with_color(win, color);

    if(pixmap)
        XSetWindowBackgroundPixmap(xinfo.display, win, pixmap);
    else
        XSetWindowBackground(xinfo.display, win, color);

    /* XSetWindowBackgroundPixmap或XSetWindowBackground不改變窗口當前內容，
       應通過發送顯露事件或調用XClearWindow來立即改變背景。*/
    if(pixmap || win==xinfo.root_win)
        XClearWindow(xinfo.display, win);
    else
        XSendEvent(xinfo.display, win, False, ExposureMask, &event);

    if(compos_root)
        change_prop_for_root_bg(pixmap);
}

static Pixmap create_pixmap_with_color(Drawable d, unsigned long color)
{
    int w, h, red=(color & 0x00ff0000UL)>>16, green=(color & 0x0000ff00UL)>>8,
        blue=(color & 0x000000ffUL), alpha=(color & 0xff000000UL)>>24;
    unsigned int depth;
    Imlib_Image image=NULL;

    if( !get_geometry(d, NULL, NULL, &w, &h, NULL, &depth)
        || !(image=imlib_create_image(w, h)))
        return None;

    Pixmap pixmap=XCreatePixmap(xinfo.display, d, w, h, depth);
    if(!pixmap)
        return None;

    set_visual_for_imlib(d);
    imlib_context_set_image(image);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color(red, green, blue , alpha);
    imlib_image_fill_rectangle(0, 0, w, h);
    imlib_render_image_on_drawable(0, 0);
    imlib_free_image();
    return pixmap;
}

static void change_prop_for_root_bg(Pixmap pixmap)
{
    Window win=xinfo.root_win;
    Atom prop_root=XInternAtom(xinfo.display, "_XROOTPMAP_ID", True);
    Atom prop_esetroot=XInternAtom(xinfo.display, "ESETROOT_PMAP_ID", True);

    if(prop_root && prop_esetroot)
    {
        Pixmap rid=get_pixmap_prop(win, prop_root),
               eid=get_pixmap_prop(win, prop_esetroot);

        if(rid && eid && eid!=rid)
            XKillClient(xinfo.display, rid);
    }

    prop_root=XInternAtom(xinfo.display, "_XROOTPMAP_ID", False);
    prop_esetroot=XInternAtom(xinfo.display, "ESETROOT_PMAP_ID", False);
    XChangeProperty(xinfo.display, win, prop_root, XA_PIXMAP, 32,
        PropModeReplace, (unsigned char *)&pixmap, 1);
    XChangeProperty(xinfo.display, win, prop_esetroot, XA_PIXMAP, 32,
        PropModeReplace, (unsigned char *)&pixmap, 1);
}

void set_override_redirect(Window win)
{
    XSetWindowAttributes attr={.override_redirect=True};
    XChangeWindowAttributes(xinfo.display, win, CWOverrideRedirect, &attr);
}

bool get_geometry(Drawable drw, int *x, int *y, int *w, int *h, int *bw, unsigned int *depth)
{
    Window r;
    int xt, yt;
    unsigned int wt, ht, bwt, dt;

    return XGetGeometry(xinfo.display, drw, &r, x ? x : &xt, y ? y : &yt,
        w ? (unsigned int *)w : &wt, h ? (unsigned int *)h : &ht,
        bw ? (unsigned int *)bw : &bwt, depth ? depth : &dt);
}

void set_visual_for_imlib(Drawable d)
{
    if(d == xinfo.root_win)
        imlib_context_set_visual(DefaultVisual(xinfo.display, xinfo.screen));
    else
        imlib_context_set_visual(xinfo.visual);
}

Window *query_win_list(unsigned int *n)
{
    Window root, parent, *child=NULL;
    unsigned int m;

    if(!XQueryTree(xinfo.display, xinfo.root_win, &root, &parent, &child, &m))
        return NULL;

    if(n)
        *n=m;

    return child;
}

char *get_title_text(Window win, const char *fallback)
{
    char *s=NULL;

    if((s=get_net_wm_name(win)))
    {
        if(strlen(s))
            return s;
        free(s);
    }

    if((s=get_wm_name(win)))
    {
        if(strlen(s))
            return s;
        free(s);
    }

    return copy_string(fallback);
}

char *get_icon_title_text(Window win, const char *fallback)
{
    char *s=NULL;

    if((s=get_net_wm_icon_name(win)))
    {
        if(strlen(s))
            return s;
        free(s);
    }
    if((s=get_wm_icon_name(win)))
    {
        if(strlen(s))
            return s;
        free(s);
    }

    return get_title_text(win, fallback);
}
