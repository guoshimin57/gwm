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
#include "file.h"
#include "font.h"
#include "handler.h"
#include "prop.h"
#include "icccm.h"
#include "focus.h"
#include "widget.h"
#include "wallpaper.h"
#include "bind_cfg.h"
#include "init.h"

static void set_visual_info(void);
static void set_locale(void);
static void set_ewmh(void);
static void set_atoms(void);
static void init_imlib(void);

void init_wm(void)
{
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
    reg_bind(keybind, buttonbind);
    create_gwm_taskbar();
    cmd_entry=cmd_entry_new(RUN_CMD_ENTRY);
    color_entry=color_entry_new(COLOR_ENTRY);
    init_client_list();
    grab_keys();
    exec_autostart();
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
