/* *************************************************************************
 *     init.c：實現初始化gwm的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
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
static void init_imlib(void);
static void init_wallpaper_files(WM *wm);

Window gwin;
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
    load_fonts();
    alloc_color(cfg->main_color_name);
    init_root_win_background();
    create_cursors();
    set_cursor(xinfo.root_win, NO_OP);
    set_workarea(wm);
    set_ewmh(wm);
    set_gwm_current_layout(DESKTOP(wm)->cur_layout);
    create_taskbar();
    if(cfg->show_taskbar)
        show_widget(WIDGET(taskbar));
    cmd_entry=create_cmd_entry(RUN_CMD_ENTRY);
    create_hint_win();
    create_clients(wm);
    grab_keys();
    exec_autostart();
}

static void create_refer_wins(WM *wm)
{
    Window w=xinfo.root_win;
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        wm->top_wins[i]=create_widget_win(w, -1, -1, 1, 1, 0, 0, 0);
    wm->wm_check_win=create_widget_win(w, -1, -1, 1, 1, 0, 0, 0);
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
        wm->workarea.h-=th;
        if(cfg->taskbar_on_top)
            wm->workarea.y=th;
    }
}

static void set_ewmh(WM *wm)
{
    set_net_supported();
    set_net_number_of_desktops(DESKTOP_N);
    set_net_desktop_geometry(xinfo.screen_width, xinfo.screen_height);
    set_net_desktop_viewport(0, 0);
    set_net_current_desktop(wm->cur_desktop-1);
    set_net_desktop_names(cfg->taskbar_button_text, DESKTOP_N);
    set_net_workarea(wm->workarea.x, wm->workarea.y, wm->workarea.w, wm->workarea.h, DESKTOP_N);
    set_net_supporting_wm_check(wm->wm_check_win, "gwm");
    set_net_showing_desktop(false);
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
    wm->wallpapers=get_files_in_paths(paths, reg, true);
    wm->cur_wallpaper=list_first_entry(&wm->wallpapers->list, Strings, list);
}
