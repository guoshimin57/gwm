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

#include "misc.h"
#include "config.h"
#include "entry.h"
#include "file.h"
#include "font.h"
#include "handler.h"
#include "prop.h"
#include "icccm.h"
#include "focus.h"
#include "taskbar.h"
#include "layout.h"
#include "init.h"

static Rect compute_taskbar_rect(void);
static void set_visual_info(void);
static void set_locale(void);
static void create_refer_wins(WM *wm);
static void set_workarea(WM *wm);
static void set_ewmh(WM *wm);
static void set_atoms(void);
static void init_imlib(void);
static void init_wallpaper_files(WM *wm);

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
    config();
    init_imlib();
    if(cfg->wallpaper_paths)
        init_wallpaper_files(wm);
    set_net_current_desktop(cfg->default_cur_desktop);
    reg_event_handlers(wm);
    load_fonts();
    alloc_color(cfg->main_color_name);
    init_root_win_background();
    create_cursors();
    set_cursor(xinfo.root_win, NO_OP);
    set_workarea(wm);
    set_ewmh(wm);
    init_layout();
    Rect r=compute_taskbar_rect();
    wm->taskbar=taskbar_new(NULL, r.x, r.y, r.w, r.h);
    if(cfg->show_taskbar)
        widget_show(WIDGET(wm->taskbar));
    cmd_entry=cmd_entry_new(RUN_CMD_ENTRY);
    color_entry=color_entry_new(COLOR_ENTRY);
    create_hint_win();
    reg_focus_func(focus_client);
    init_client_list();
    grab_keys();
    exec_autostart();
}

static Rect compute_taskbar_rect(void)
{
    int w=xinfo.screen_width, h=get_font_height_by_pad(),
        x=0, y=(cfg->taskbar_on_top ? 0 : xinfo.screen_height-h);
    return (Rect){x, y, w, h};
}

static void create_refer_wins(WM *wm)
{
    Window w=xinfo.root_win;
    wm->wm_check_win=create_widget_win(w, -1, -1, 1, 1, 0, 0, 0);
    create_refer_top_wins();
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
    set_net_current_desktop(cfg->default_cur_desktop);
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

static void init_wallpaper_files(WM *wm)
{
    const char *paths=cfg->wallpaper_paths, *reg="*.png|*.jpg|*.svg|*.webp";
    wm->wallpapers=get_files_in_paths(paths, reg, true);
    wm->cur_wallpaper=list_first_entry(&wm->wallpapers->list, Strings, list);
}
