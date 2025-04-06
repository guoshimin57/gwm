/* *************************************************************************
 *     place.c：實現與窗口放置位置相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "clientop.h"
#include "func.h"
#include "prop.h"
#include "focus.h"
#include "place.h"

static void key_change_place(Place type);

void pointer_change_place(XEvent *e, Arg arg)
{
    XEvent ev;
    Client *from=get_cur_focus_client(), *to;

    UNUSED(arg);
    if(get_gwm_layout()!=TILE || !from || !get_valid_click(CHANGE, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    Window win=ev.xbutton.window, subw=ev.xbutton.subwindow;
    to=win_to_client(subw);
    if(ev.xbutton.x == 0)
        move_client(from, NULL, SECOND_AREA);
    else if(ev.xbutton.x == (long)xinfo.screen_width-1)
        move_client(from, NULL, FIXED_AREA);
    else if(win==xinfo.root_win && subw==None)
        move_client(from, NULL, MAIN_AREA);
    else if(to)
        move_client(from, to, ANY_PLACE);
    update_net_wm_state_for_no_max(WIDGET_WIN(from), from->win_state);
}

void change_to_main(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_change_place(MAIN_AREA);
}

void change_to_second(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_change_place(SECOND_AREA);
}

void change_to_fixed(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_change_place(FIXED_AREA);
}

void change_to_above(XEvent *e, Arg arg)
{
    UNUSED(e), UNUSED(arg);
    key_change_place(ABOVE_LAYER);
}

static void key_change_place(Place type)
{
    Client *c=get_cur_focus_client();
    move_client(c, NULL, type);
    update_net_wm_state_for_no_max(WIDGET_WIN(c), c->win_state);
}

void pointer_swap_clients(XEvent *e, Arg arg)
{
    UNUSED(arg);
    XEvent ev;
    Client *from=get_cur_focus_client(), *to=NULL;
    if(get_gwm_layout()!=TILE || !from || !get_valid_click(SWAP, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    if((to=win_to_client(ev.xbutton.subwindow)))
        swap_clients(from, to);
}
