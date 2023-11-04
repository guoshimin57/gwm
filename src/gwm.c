/* *************************************************************************
 *     gwm.c：實現窗口管理器的主要部分。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static void set_ewmh(WM *wm);
static void set_signals(void);
static void ready_to_quit(int signum);

sig_atomic_t run_flag=1;

int main(void)
{
    WM wm;
    clear_zombies(0);
    init_wm(&wm);
    XSetScreenSaver(wm.display, wm.cfg->screen_saver_time_out,
        wm.cfg->screen_saver_interval, PreferBlanking, AllowExposures);
    set_signals();
    set_ewmh(&wm);
    handle_events(&wm);
    return EXIT_SUCCESS;
}

static void set_ewmh(WM *wm)
{
    set_net_supported(wm->display, wm->root_win);
    set_net_number_of_desktops(wm->display, wm->root_win, DESKTOP_N);
    set_net_desktop_geometry(wm->display, wm->root_win, wm->screen_width, wm->screen_height);
    set_net_desktop_viewport(wm->display, wm->root_win, 0, 0);
    set_net_current_desktop(wm->display, wm->root_win, wm->cur_desktop-1);
    set_net_desktop_names(wm->display, wm->root_win, &wm->cfg->taskbar_button_text[DESKTOP_BUTTON_BEGIN], DESKTOP_N);
    set_net_workarea(wm->display, wm->root_win, wm->workarea.x, wm->workarea.y, wm->workarea.w, wm->workarea.h, DESKTOP_N);
    set_net_supporting_wm_check(wm->display, wm->root_win, wm->wm_check_win, "gwm");
    set_net_showing_desktop(wm->display, wm->root_win, false);
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
