/* *************************************************************************
 *     gwm.c：實現窗口管理器的主要部分。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "clientop.h"
#include "handler.h"
#include "init.h"

sig_atomic_t run_flag=1;
Event_handler event_handler=NULL; // 事件處理器

/* 以下全局變量一經顯式初始化，就不再修改 */
Xinfo xinfo;

int main(void)
{
    init_gwm();
    manage_exsit_clients();
    handle_events();
    deinit_gwm();

    return EXIT_SUCCESS;
}
