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
static void set_ewmh(WM *wm);
static void set_atoms(void);
static void create_cursors(WM *wm);
static void create_hint_win(void);
static void create_client_menu(void);
static void create_clients(WM *wm);
static void init_imlib(void);
static void init_wallpaper_files(WM *wm);
static void init_root_win_background(void);
static void exec_autostart(void);

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
    set_atoms();
    set_gwm_widget_type(xinfo.root_win, ROOT_WIN);
    XSelectInput(xinfo.display, xinfo.root_win, ROOT_EVENT_MASK);
    set_visual_info();
    create_refer_wins(wm);
    wm->gc=XCreateGC(xinfo.display, wm->wm_check_win, 0, NULL);
    config();
    init_imlib();
    if(cfg->wallpaper_paths)
        init_wallpaper_files(wm);
    init_desktop(wm);
    reg_event_handlers(wm);
    load_font();
    alloc_color();
    init_root_win_background();
    create_cursors(wm);
    XDefineCursor(xinfo.display, xinfo.root_win, wm->cursors[NO_OP]);
    set_workarea(wm);
    set_ewmh(wm);
    set_gwm_current_layout(DESKTOP(wm)->cur_layout);
    taskbar=create_taskbar();
    cmd_entry=create_cmd_entry(RUN_CMD_ENTRY);
    create_hint_win();
    create_client_menu();
    create_clients(wm);
    grab_keys();
    exec_autostart();
}

static void create_refer_wins(WM *wm)
{
    Window w=xinfo.root_win;
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        wm->top_wins[i]=create_widget_win(NON_WIDGET, w, -1, -1, 1, 1, 0, 0, 0);
    wm->wm_check_win=create_widget_win(NON_WIDGET, w, -1, -1, 1, 1, 0, 0, 0);
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
    long sw=xinfo.screen_width, sh=xinfo.screen_height, th=get_font_height_by_pad();

    wm->workarea=(Rect){0, 0, sw, sh};
    if(cfg->show_taskbar)
    {
        wm->workarea.h-=th+cfg->win_gap;
        if(cfg->taskbar_on_top)
            wm->workarea.y=th+cfg->win_gap;
    }
}

static void set_ewmh(WM *wm)
{
    set_net_supported();
    set_net_number_of_desktops(DESKTOP_N);
    set_net_desktop_geometry(xinfo.screen_width, xinfo.screen_height);
    set_net_desktop_viewport(0, 0);
    set_net_current_desktop(wm->cur_desktop-1);
    set_net_desktop_names(&cfg->taskbar_button_text[DESKTOP_BUTTON_BEGIN], DESKTOP_N);
    set_net_workarea(wm->workarea.x, wm->workarea.y, wm->workarea.w, wm->workarea.h, DESKTOP_N);
    set_net_supporting_wm_check(wm->wm_check_win, "gwm");
    set_net_showing_desktop(false);
}

static void exec_autostart(void)
{
    char cmd[BUFSIZ];
    sprintf(cmd, "[ -x '%s' ] && '%s'", cfg->autostart, cfg->autostart);
    const char *sh_cmd[]={"/bin/sh", "-c", cfg->autostart, NULL};
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
    set_gwm_atoms();
}

static void create_cursors(WM *wm)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        wm->cursors[i]=XCreateFontCursor(xinfo.display, cfg->cursor_shape[i]);
}

static void create_hint_win(void)
{
    xinfo.hint_win=create_widget_win(HINT_WIN, xinfo.root_win, 0, 0, 1, 1, 0, 0,
        get_widget_color(HINT_WIN_COLOR));
    XSelectInput(xinfo.display, xinfo.hint_win, ExposureMask);
}

static void create_client_menu(void)
{
    Widget_type types[CLIENT_MENU_ITEM_N];
    for(int i=0; i<CLIENT_MENU_ITEM_N; i++)
        types[i]=CLIENT_MENU_ITEM_BEGIN+i;
    client_menu=create_menu(CLIENT_MENU, types, cfg->client_menu_item_text,
        CLIENT_MENU_ITEM_N, 1);
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
    const char *paths=cfg->wallpaper_paths, *reg="*.png|*.jpg|*.svg|*.webp";
    wm->wallpapers=get_files_in_paths(paths, reg, NOSORT, true, NULL);
    wm->cur_wallpaper=wm->wallpapers->next;
}

static void init_root_win_background(void)
{
    const char *name=cfg->wallpaper_filename;

    Pixmap pixmap=create_pixmap_from_file(xinfo.root_win, name ? name : "");
    update_win_bg(xinfo.root_win, get_widget_color(ROOT_WIN_COLOR), pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}
