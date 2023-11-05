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

static void set_visual_info(void);
static void set_locale(void);
static void create_refer_wins(WM *wm);
static void set_workarea(WM *wm);
static void set_atoms(void);
static void create_cursors(WM *wm);
static void create_run_cmd_entry(WM *wm);
static void create_hint_win(WM *wm);
static void create_client_menu(WM *wm);
static void create_clients(WM *wm);
static void init_imlib(void);
static void init_wallpaper_files(WM *wm);
static void init_root_win_background(WM *wm);
static void exec_autostart(WM *wm);

void init_wm(WM *wm)
{
    memset(wm, 0, sizeof(WM));
    if(!(xinfo.display=XOpenDisplay(NULL)))
        exit_with_msg("error: cannot open display");

    XSetErrorHandler(x_fatal_handler);
    set_locale();
    xinfo.screen=DefaultScreen(xinfo.display);
    xinfo.screen_width=DisplayWidth(xinfo.display, xinfo.screen);
    xinfo.screen_height=DisplayHeight(xinfo.display, xinfo.screen);
    xinfo.mod_map=XGetModifierMapping(xinfo.display);
    xinfo.root_win=RootWindow(xinfo.display, xinfo.screen);
    XSelectInput(xinfo.display, xinfo.root_win, ROOT_EVENT_MASK);
    set_visual_info();
    create_refer_wins(wm);
    wm->gc=XCreateGC(xinfo.display, wm->wm_check_win, 0, NULL);
    config(wm);
    init_imlib();
    if(wm->cfg->wallpaper_paths)
        init_wallpaper_files(wm);
    init_desktop(wm);
    reg_event_handlers(wm);
    set_atoms();
    load_font(wm);
    alloc_color(wm);
    init_root_win_background(wm);
    create_cursors(wm);
    XDefineCursor(xinfo.display, xinfo.root_win, wm->cursors[NO_OP]);
    set_workarea(wm);
    create_taskbar(wm);
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
        wm->top_wins[i]=create_widget_win(xinfo.root_win, -1, -1, 1, 1, 0, 0, 0);
    wm->wm_check_win=create_widget_win(xinfo.root_win, -1, -1, 1, 1, 0, 0, 0);
}

static void set_visual_info(void)
{
    XVisualInfo v;
    XMatchVisualInfo(xinfo.display, xinfo.screen, 32, TrueColor, &v);
    xinfo.depth=v.depth;
    xinfo.visual=v.visual;
    xinfo.colormap=XCreateColormap(xinfo.display, xinfo.root_win, v.visual, AllocNone);
}

static void set_workarea(WM *wm)
{
    long sw=xinfo.screen_width, sh=xinfo.screen_height, th=TASKBAR_HEIGHT(wm);

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
    exec_cmd((char *const *)sh_cmd);
}

static void set_locale(void)
{
	if(!setlocale(LC_ALL, "") || !XSupportsLocale())
		fprintf(stderr, "warning: no locale support\n");
    else
    {
        bindtextdomain("gwm", "/usr/share/locale/");
        textdomain("gwm");

        char *m=XSetLocaleModifiers("");
        xinfo.xim=XOpenIM(xinfo.display, NULL, NULL, NULL);
        if(!m || !xinfo.xim)
            fprintf(stderr, _("錯誤: 不能設置輸入法"));
    }
}

static void set_atoms(void)
{
    set_icccm_atoms();
    set_ewmh_atoms();
}

static void create_cursors(WM *wm)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        wm->cursors[i]=XCreateFontCursor(xinfo.display, wm->cfg->cursor_shape[i]);
}

static void create_run_cmd_entry(WM *wm)
{
    int sw=xinfo.screen_width, sh=xinfo.screen_height, bw=wm->cfg->border_width,
        ew, eh=ENTRY_HEIGHT(wm), pad=get_font_pad(wm, ENTRY_FONT);
    get_string_size(wm->font[ENTRY_FONT], wm->cfg->run_cmd_entry_hint, &ew, NULL);
    ew += 2*pad, ew = (ew>=sw/4 && ew<=sw-2*bw) ? ew : sw/4;
    Rect r={(sw-ew)/2-bw, (sh-eh)/2-bw, ew, eh};
    wm->run_cmd=create_entry(wm, &r, wm->cfg->run_cmd_entry_hint);
}

static void create_hint_win(WM *wm)
{
    wm->hint_win=create_widget_win(xinfo.root_win, 0, 0, 1, 1, 0, 0,
        WIDGET_COLOR(wm, HINT_WIN));
    XSelectInput(xinfo.display, wm->hint_win, ExposureMask);
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
    wm->clients->win=xinfo.root_win;
    wm->clients->prev=wm->clients->next=wm->clients;
    if(!XQueryTree(xinfo.display, xinfo.root_win, &root, &parent, &child, &n))
        exit_with_msg(_("錯誤：查詢窗口清單失敗！"));
    for(size_t i=0; i<n; i++)
    {
        if(is_wm_win(wm, child[i], true))
            add_client(wm, child[i]);
        restack_win(wm, child[i]);
    }
    XFree(child);
}

static void init_imlib(void)
{
    imlib_context_set_dither(1);
    imlib_context_set_display(xinfo.display);
    imlib_context_set_visual(xinfo.visual);
    imlib_context_set_colormap(xinfo.colormap);
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

    Pixmap pixmap=create_pixmap_from_file(xinfo.root_win, name ? name : "");
    update_win_bg(xinfo.root_win, WIDGET_COLOR(wm, ROOT_WIN), pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}
