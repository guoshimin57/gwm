/* *************************************************************************
 *     init.c：實現初始化gwm的功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <locale.h>
#include <sys/types.h>
#include "gwm.h"
#include "init.h"
#include "client.h"
#include "color.h"
#include "desktop.h"
#include "entry.h"
#include "font.h"
#include "func.h"
#include "grab.h"
#include "layout.h"
#include "menu.h"
#include "misc.h"

static void set_locale(WM *wm);
static void set_atoms(WM *wm);
static void create_cursors(WM *wm);
static void create_taskbar(WM *wm);
static void create_taskbar_buttons(WM *wm);
static void create_icon_area(WM *wm);
static void create_status_area(WM *wm);
static void create_cmd_center(WM *wm);
static void create_run_cmd_entry(WM *wm);
static void create_hint_win(WM *wm);
static void create_clients(WM *wm);
static void init_wallpaper_files(WM *wm);

void init_wm(WM *wm)
{
    memset(wm, 0, sizeof(WM));
    if(!(wm->display=XOpenDisplay(NULL)))
        exit_with_msg("error: cannot open display");
    set_locale(wm);

    wm->screen=DefaultScreen(wm->display);
    wm->screen_width=DisplayWidth(wm->display, wm->screen);
    wm->screen_height=DisplayHeight(wm->display, wm->screen);
    wm->mod_map=XGetModifierMapping(wm->display);
    wm->root_win=RootWindow(wm->display, wm->screen);
    wm->gc=XCreateGC(wm->display, wm->root_win, 0, NULL);
    wm->visual=DefaultVisual(wm->display, wm->screen);
    wm->colormap=DefaultColormap(wm->display, wm->screen);
    wm->focus_mode=DEFAULT_FOCUS_MODE;

#ifdef WALLPAPER_PATHS
    init_wallpaper_files(wm);
#endif
    init_desktop(wm);
    XSetErrorHandler(x_fatal_handler);
    XSelectInput(wm->display, wm->root_win, ROOT_EVENT_MASK);
    set_atoms(wm);
    load_font(wm);
    alloc_color(wm);
    create_cursors(wm);
    XDefineCursor(wm->display, wm->root_win, wm->cursors[NO_OP]);
    create_taskbar(wm);
    create_cmd_center(wm);
    create_run_cmd_entry(wm);
    create_hint_win(wm);
    create_clients(wm);
    update_layout(wm);
    grab_keys(wm);
    exec(wm, NULL, (Func_arg)SH_CMD("[ -x "AUTOSTART" ] && "AUTOSTART));
}

static void set_locale(WM *wm)
{
	if(!setlocale(LC_CTYPE, "") || !XSupportsLocale())
		fprintf(stderr, "warning: no locale support\n");
    else
    {
        char *m=XSetLocaleModifiers("");
        wm->xim=XOpenIM(wm->display, NULL, NULL, NULL);
        if(!m || !wm->xim)
            fprintf(stderr, "錯誤: 不能設置輸入法");
    }
}

static void set_atoms(WM *wm)
{
    for(size_t i=0; i<ICCCM_ATOMS_N; i++)
        wm->icccm_atoms[i]=XInternAtom(wm->display, ICCCM_NAMES[i], False);
    for(size_t i=0; i<EWMH_ATOM_N; i++)
        wm->ewmh_atom[i]=XInternAtom(wm->display, EWMH_NAME[i], False);
    wm->utf8=XInternAtom(wm->display, "UTF8_STRING", False);
}

static void create_cursors(WM *wm)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        wm->cursors[i]=XCreateFontCursor(wm->display, CURSOR_SHAPE[i]);
}

static void create_taskbar(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    b->x=0, b->y=wm->screen_height-TASKBAR_HEIGHT;
    b->w=wm->screen_width, b->h=TASKBAR_HEIGHT;
    b->win=XCreateSimpleWindow(wm->display, wm->root_win, b->x, b->y,
        b->w, b->h, 0, 0, 0);
    set_override_redirect(wm, b->win);
    XSelectInput(wm->display, b->win, CROSSING_MASK);
    create_taskbar_buttons(wm);
    create_status_area(wm);
    create_icon_area(wm);
    XMapRaised(wm->display, b->win);
    XMapWindow(wm->display, b->win);
    XMapSubwindows(wm->display, b->win);
}

static void create_taskbar_buttons(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    for(size_t i=0; i<TASKBAR_BUTTON_N; i++)
    {
        unsigned long color = is_chosen_button(wm, TASKBAR_BUTTON_BEGIN+i) ?
            wm->widget_color[CHOSEN_TASKBAR_BUTTON_COLOR].pixel :
            wm->widget_color[NORMAL_TASKBAR_BUTTON_COLOR].pixel ;
        b->buttons[i]=XCreateSimpleWindow(wm->display, b->win,
            TASKBAR_BUTTON_WIDTH*i, 0,
            TASKBAR_BUTTON_WIDTH, TASKBAR_BUTTON_HEIGHT, 0, 0, color);
        XSelectInput(wm->display, b->buttons[i], BUTTON_EVENT_MASK);
    }
}

static void create_icon_area(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    unsigned int bw=TASKBAR_BUTTON_WIDTH*TASKBAR_BUTTON_N,
        w=b->w-bw-b->status_area_w;
    b->icon_area=XCreateSimpleWindow(wm->display, b->win,
        bw, 0, w, b->h, 0, 0, wm->widget_color[ICON_AREA_COLOR].pixel);
}
static void create_status_area(WM *wm)
{
    Taskbar *b=&wm->taskbar;
    b->status_text=get_text_prop(wm, wm->root_win, XA_WM_NAME);
    get_string_size(wm, wm->font[STATUS_AREA_FONT], b->status_text, &b->status_area_w, NULL);
    if(b->status_area_w > STATUS_AREA_WIDTH_MAX)
        b->status_area_w=STATUS_AREA_WIDTH_MAX;
    else if(b->status_area_w == 0)
        b->status_area_w=1;
    wm->taskbar.status_area=XCreateSimpleWindow(wm->display, b->win,
        b->w-b->status_area_w, 0, b->status_area_w, b->h,
        0, 0, wm->widget_color[STATUS_AREA_COLOR].pixel);
    XSelectInput(wm->display, b->status_area, ExposureMask);
}

static void create_cmd_center(WM *wm)
{
    unsigned int n=CMD_CENTER_ITEM_N, col=CMD_CENTER_COL,
        w=CMD_CENTER_ITEM_WIDTH, h=CMD_CENTER_ITEM_HEIGHT;
    unsigned long color=wm->widget_color[CMD_CENTER_COLOR].pixel;

    create_menu(wm, &wm->cmd_center, n, col, w, h, color);
}

static void create_run_cmd_entry(WM *wm)
{
    Rect r={(wm->screen_width-RUN_CMD_ENTRY_WIDTH)/2,
    (wm->screen_height-RUN_CMD_ENTRY_HEIGHT)/2,
    RUN_CMD_ENTRY_WIDTH, RUN_CMD_ENTRY_HEIGHT};
    create_entry(wm, &wm->run_cmd, &r, RUN_CMD_ENTRY_HINT);
}

static void create_hint_win(WM *wm)
{
    wm->hint_win=XCreateSimpleWindow(wm->display, wm->root_win, 0, 0, 1,
        1, 0, 0, wm->widget_color[HINT_WIN_COLOR].pixel);
    set_override_redirect(wm, wm->hint_win);
    XSelectInput(wm->display, wm->hint_win, ExposureMask);
}

/* 生成帶表頭結點的雙向循環鏈表 */
static void create_clients(WM *wm)
{
    Window root, parent, *child=NULL;
    unsigned int n;
    Desktop *d=wm->desktop;

    wm->clients=malloc_s(sizeof(Client));
    memset(wm->clients, 0, sizeof(Client));
    for(size_t i=0; i<DESKTOP_N; i++)
        d[i].cur_focus_client=d[i].prev_focus_client=wm->clients;
    wm->clients->area_type=ROOT_AREA;
    wm->clients->win=wm->root_win;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(!XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &n))
        exit_with_msg("錯誤：查詢窗口清單失敗！");
    for(size_t i=0; i<n; i++)
        if(is_wm_win(wm, child[i]))
            add_client(wm, child[i]);
    XFree(child);
}

void init_imlib(WM *wm)
{
    imlib_context_set_dither(1);
    imlib_context_set_display(wm->display);
    imlib_context_set_visual(wm->visual);
}

static void init_wallpaper_files(WM *wm)
{
    const char *exts[]={"png", "jpg"};
    size_t n=ARRAY_NUM(WALLPAPER_PATHS);
    wm->wallpapers=get_files_in_dirs(WALLPAPER_PATHS, n, exts, 2, NOSORT, true);
    wm->cur_wallpaper=wm->wallpapers->next;
}

void init_root_win_background(WM *wm)
{
    unsigned long color=wm->widget_color[ROOT_WIN_COLOR].pixel;
    Pixmap pixmap=None;
#ifdef WALLPAPER_FILENAME
    pixmap=create_pixmap_from_file(wm, wm->root_win, WALLPAPER_FILENAME);
#endif
    update_win_background(wm, wm->root_win, color, pixmap);
#ifdef WALLPAPER_FILENAME
    if(pixmap)
        XFreePixmap(wm->display, pixmap);
#endif
}
