/* *************************************************************************
 *     init.c：實現初始化gwm的功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void set_visual_info(WM *wm);
static void set_locale(WM *wm);
static void create_refer_wins(WM *wm);
static void set_workarea(WM *wm);
static void set_atoms(WM *wm);
static void create_cursors(WM *wm);
static void create_run_cmd_entry(WM *wm);
static void create_hint_win(WM *wm);
static void create_client_menu(WM *wm);
static void create_clients(WM *wm);
static void init_imlib(WM *wm);
static void init_wallpaper_files(WM *wm);
static void init_root_win_background(WM *wm);
static void exec_autostart(WM *wm);

void init_wm(WM *wm)
{
    memset(wm, 0, sizeof(WM));
    if(!(wm->display=XOpenDisplay(NULL)))
        exit_with_msg("error: cannot open display");

    XSetErrorHandler(x_fatal_handler);
    set_locale(wm);
    wm->screen=DefaultScreen(wm->display);
    wm->screen_width=DisplayWidth(wm->display, wm->screen);
    wm->screen_height=DisplayHeight(wm->display, wm->screen);
    wm->mod_map=XGetModifierMapping(wm->display);
    wm->root_win=RootWindow(wm->display, wm->screen);
    XSelectInput(wm->display, wm->root_win, ROOT_EVENT_MASK);
    set_visual_info(wm);
    create_refer_wins(wm);
    wm->gc=XCreateGC(wm->display, wm->wm_check_win, 0, NULL);
    config(wm);
    init_imlib(wm);
    if(wm->cfg->wallpaper_paths)
        init_wallpaper_files(wm);
    init_desktop(wm);
    reg_event_handlers(wm);
    set_atoms(wm);
    load_font(wm);
    alloc_color(wm);
    init_root_win_background(wm);
    create_cursors(wm);
    XDefineCursor(wm->display, wm->root_win, wm->cursors[NO_OP]);
    create_taskbar(wm);
    set_workarea(wm);
    create_run_cmd_entry(wm);
    create_hint_win(wm);
    create_client_menu(wm);
    create_clients(wm);
    grab_keys(wm);
    exec_autostart(wm);
}

static void create_refer_wins(WM *wm)
{
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        wm->top_wins[i]=create_widget_win(wm, wm->root_win, -1, -1, 1, 1, 0, 0, 0);
    wm->wm_check_win=create_widget_win(wm, wm->root_win, -1, -1, 1, 1, 0, 0, 0);
}

static void set_visual_info(WM *wm)
{
    XVisualInfo v;
    XMatchVisualInfo(wm->display, wm->screen, 32, TrueColor, &v);
    wm->depth=v.depth;
    wm->visual=v.visual;
    wm->colormap=XCreateColormap(wm->display, wm->root_win, v.visual, AllocNone);
}

static void set_workarea(WM *wm)
{
    long sw=wm->screen_width, sh=wm->screen_height, th=wm->taskbar->h;

    wm->workarea=(Rect){0, 0, sw, sh};
    if(wm->cfg->show_taskbar)
    {
        wm->workarea.h-=th+wm->cfg->win_gap;
        if(wm->cfg->taskbar_on_top)
            wm->workarea.y=th+wm->cfg->win_gap;
    }
}

static void exec_autostart(WM *wm)
{
    char cmd[BUFSIZ];
    sprintf(cmd, "[ -x '%s' ] && '%s'", wm->cfg->autostart, wm->cfg->autostart);
    const char *sh_cmd[]={"/bin/sh", "-c", wm->cfg->autostart, NULL};
    exec_cmd(wm, (char *const *)sh_cmd);
}

static void set_locale(WM *wm)
{
	if(!setlocale(LC_ALL, "") || !XSupportsLocale())
		fprintf(stderr, "warning: no locale support\n");
    else
    {
        bindtextdomain("gwm", "/usr/share/locale/");
        textdomain("gwm");

        char *m=XSetLocaleModifiers("");
        wm->xim=XOpenIM(wm->display, NULL, NULL, NULL);
        if(!m || !wm->xim)
            fprintf(stderr, _("錯誤: 不能設置輸入法"));
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
        wm->cursors[i]=XCreateFontCursor(wm->display, wm->cfg->cursor_shape[i]);
}

static void create_run_cmd_entry(WM *wm)
{
    int sw=wm->screen_width, sh=wm->screen_height, bw=wm->cfg->border_width,
        ew, eh=ENTRY_HEIGHT(wm), pad=get_font_pad(wm, ENTRY_FONT);
    get_string_size(wm, wm->font[ENTRY_FONT], wm->cfg->run_cmd_entry_hint, &ew, NULL);
    ew += 2*pad, ew = (ew>=sw/4 && ew<=sw-2*bw) ? ew : sw/4;
    Rect r={(sw-ew)/2-bw, (sh-eh)/2-bw, ew, eh};
    wm->run_cmd=create_entry(wm, &r, wm->cfg->run_cmd_entry_hint);
}

static void create_hint_win(WM *wm)
{
    wm->hint_win=create_widget_win(wm, wm->root_win, 0, 0, 1, 1, 0, 0,
        WIDGET_COLOR(wm, HINT_WIN));
    XSelectInput(wm->display, wm->hint_win, ExposureMask);
}

static void create_client_menu(WM *wm)
{
    wm->client_menu=create_menu(wm, wm->cfg->client_menu_item_text, CLIENT_MENU_ITEM_N, 1);
}

/* 生成帶表頭結點的雙向循環鏈表 */
static void create_clients(WM *wm)
{
    Window root, parent, *child=NULL;
    unsigned int n;
    Desktop **d=wm->desktop;

    wm->clients=malloc_s(sizeof(Client));
    memset(wm->clients, 0, sizeof(Client));
    for(size_t i=0; i<DESKTOP_N; i++)
        d[i]->cur_focus_client=d[i]->prev_focus_client=wm->clients;
    wm->clients->win=wm->root_win;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(!XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &n))
        exit_with_msg(_("錯誤：查詢窗口清單失敗！"));
    for(size_t i=0; i<n; i++)
    {
        if(is_wm_win(wm, child[i], true))
            add_client(wm, child[i]);
        restack_win(wm, child[i]);
    }
    XFree(child);
}

static void init_imlib(WM *wm)
{
    imlib_context_set_dither(1);
    imlib_context_set_display(wm->display);
    imlib_context_set_visual(wm->visual);
    imlib_context_set_colormap(wm->colormap);
}

static void init_wallpaper_files(WM *wm)
{
    const char *paths=wm->cfg->wallpaper_paths, *reg="*.png|*.jpg|*.svg|*.webp";
    wm->wallpapers=get_files_in_paths(paths, reg, NOSORT, true, NULL);
    wm->cur_wallpaper=wm->wallpapers->next;
}

static void init_root_win_background(WM *wm)
{
    const char *name=wm->cfg->wallpaper_filename;

    Pixmap pixmap=create_pixmap_from_file(wm, wm->root_win, name ? name : "");
    update_win_bg(wm, wm->root_win, WIDGET_COLOR(wm, ROOT_WIN), pixmap);
    if(pixmap && !have_compositor(wm))
        XFreePixmap(wm->display, pixmap);
}
