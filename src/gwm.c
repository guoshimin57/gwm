/* *************************************************************************
 *     gwm.c：實現窗口管理器的主要部分。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "gwm.h"
#include "client.h"
#include "desktop.h"
#include "func.h"
#include "grab.h"
#include "handler.h"
#include "init.h"
#include "layout.h"
#include "menu.h"
#include "misc.h"

static void set_signals(void);
static void ready_to_quit(int unused);

sig_atomic_t run_flag=1;

int main(int argc, char *argv[])
{
    WM wm;
    set_signals();
    clear_zombies(0);
    init_wm(&wm);
    init_imlib(&wm);
    init_root_win_background(&wm);
    handle_events(&wm);
    clear_wm(&wm);
    return EXIT_SUCCESS;
}

static void set_signals(void)
{
	if(signal(SIGCHLD, clear_zombies) == SIG_ERR)
        perror("不能安裝SIGCHLD信號處理函數");
	if(signal(SIGINT, ready_to_quit) == SIG_ERR)
        perror("不能安裝SIGINT信號處理函數");
	if(signal(SIGTERM, ready_to_quit) == SIG_ERR)
        perror("不能安裝SIGTERM信號處理函數");
	if(signal(SIGQUIT, ready_to_quit) == SIG_ERR)
        perror("不能安裝SIGQUIT信號處理函數");
}

static void ready_to_quit(int unused)
{
    run_flag=0;
}
