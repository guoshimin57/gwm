/* *************************************************************************
 *     init.c：實現初始化gwm的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "clientop.h"
#include "misc.h"
#include "config.h"
#include "entry.h"
#include "image.h"
#include "file.h"
#include "font.h"
#include "handler.h"
#include "prop.h"
#include "icccm.h"
#include "focus.h"
#include "widget.h"
#include "wallpaper.h"
#include "grab.h"
#include "layout.h"
#include "bind_cfg.h"
#include "init.h"

static Display *open_display(void);
static void clear_zombies(int signum);
static void set_visual_info(void);
static void set_locale(void);
static void set_ewmh(void);
static void set_atoms(void);
static void init_imlib(void);
static void set_screensaver(void);
static void set_signals(void);
static void ready_to_quit(int signum);

void wm_init(void)
{
    xinfo.display=open_display();
    clear_zombies(0);
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
    create_layer_wins();
    config();
    init_imlib();
    set_net_current_desktop(cfg->default_cur_desktop);
    reg_event_handlers();
    load_fonts();
    alloc_color(cfg->main_color_name);
    init_wallpaper();
    set_default_wallpaper();
    create_cursors();
    set_cursor(xinfo.root_win, NO_OP);
    set_ewmh();
    init_layout();
    reg_binds(keybinds, buttonbinds);
    create_gwm_taskbar();
    cmd_entry=cmd_entry_new(RUN_CMD_ENTRY);
    color_entry=color_entry_new(COLOR_ENTRY);
    init_client_list();
    grab_keys();
    exec_autostart();
    set_screensaver();
    set_signals();
}

static Display *open_display(void)
{
    Display *display=XOpenDisplay(NULL);
    if(display == NULL)
        exit_with_msg("error: cannot open display");
    return display;
}

static void set_visual_info(void)
{
    XVisualInfo v;
    XMatchVisualInfo(xinfo.display, xinfo.screen, 32, TrueColor, &v);
    xinfo.depth=v.depth;
    xinfo.visual=v.visual;
    xinfo.colormap=XCreateColormap(xinfo.display, xinfo.root_win, v.visual, AllocNone);
}

static void set_ewmh(void)
{
    long sw=xinfo.screen_width, sh=xinfo.screen_height,
         th=get_font_height_by_pad(), wx=0, wy=0, ww=sw, wh=sh;

    if(cfg->show_taskbar)
    {
        wh-=th;
        if(cfg->taskbar_on_top)
            wy=th;
    }

    set_net_supported();
    set_net_number_of_desktops(DESKTOP_N);
    set_net_desktop_geometry(sw, sh);
    set_net_desktop_viewport(0, 0);
    set_net_current_desktop(cfg->default_cur_desktop);
    set_net_desktop_names(cfg->taskbar_button_text, DESKTOP_N);
    set_net_workarea(wx, wy, ww, wh, DESKTOP_N);
    set_net_supporting_wm_check("gwm");
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
    set_utf8_string_atom();
    set_motif_wm_hints_atom();
    set_gwm_atoms();
}

static void init_imlib(void)
{
    imlib_context_set_dither(1);
    imlib_context_set_display(xinfo.display);
    imlib_context_set_visual(xinfo.visual);
    imlib_context_set_colormap(xinfo.colormap);
}

void wm_deinit(void)
{
    clients_for_each_safe(c)
        remove_client(c, true);
    free_all_images();
    taskbar_del(get_gwm_taskbar());
    entry_del(cmd_entry);
    entry_del(color_entry);
    menu_del(act_center);
    del_layer_wins();
    XFreeModifiermap(xinfo.mod_map);
    free_cursors();
    XSetInputFocus(xinfo.display, xinfo.root_win, RevertToPointerRoot, CurrentTime);
    if(xinfo.xim)
        XCloseIM(xinfo.xim);
    close_fonts();
    XClearWindow(xinfo.display, xinfo.root_win);
    XFlush(xinfo.display);
    XCloseDisplay(xinfo.display);
    clear_zombies(0);
    free_wallpapers();
    Free(cfg);
}

static void clear_zombies(int signum)
{
    UNUSED(signum);
	while(0 < waitpid(-1, NULL, WNOHANG))
        ;
}

static void set_screensaver(void)
{
    XSetScreenSaver(xinfo.display, cfg->screen_saver_time_out,
        cfg->screen_saver_interval, PreferBlanking, AllowExposures);
}


static void set_signals(void)
{
	if(signal(SIGCHLD, clear_zombies) == SIG_ERR)
        perror(_("不能安裝SIGCHLD信號處理函數"));
	if(signal(SIGINT, ready_to_quit) == SIG_ERR)
        perror(_("不能安裝SIGINT信號處理函數"));
	if(signal(SIGTERM, ready_to_quit) == SIG_ERR)
        perror(_("不能安裝SIGTERM信號處理函數"));
	if(signal(SIGQUIT, ready_to_quit) == SIG_ERR)
        perror(_("不能安裝SIGQUIT信號處理函數"));
	if(signal(SIGHUP, ready_to_quit) == SIG_ERR)
        perror(_("不能安裝SIGHUP信號處理函數"));
}

static void ready_to_quit(int signum)
{
    UNUSED(signum);
    run_flag=0;
}
