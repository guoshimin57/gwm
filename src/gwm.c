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

int main(int argc, char *argv[])
{
    WM wm;
    set_signals();
    clear_zombies(0);
    init_wm(&wm);
    handle_events(&wm);
    return EXIT_SUCCESS;
}

static void set_signals(void)
{
	if(signal(SIGCHLD, clear_zombies) == SIG_ERR)
    exit_with_perror("不能安裝SIGCHLD信號處理函數");
}
