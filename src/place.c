/* *************************************************************************
 *     place.c：實現使用輸入設備改變客戶窗口放置位置的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "clientop.h"
#include "prop.h"
#include "focus.h"
#include "grab.h"
#include "place.h"

static bool get_valid_click(Pointer_act act, XEvent *oe, XEvent *ne);

static bool get_valid_click(Pointer_act act, XEvent *oe, XEvent *ne);

void pointer_change_place(XEvent *e)
{
    XEvent ev;
    Client *from=get_cur_focus_client(), *to;

    if(get_gwm_layout()!=TILE || !from || !get_valid_click(CHANGE, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    Window win=ev.xbutton.window, subw=ev.xbutton.subwindow;
    to=win_to_client(subw);
    if(ev.xbutton.x == 0)
        move_client(from, NULL, TILE_LAYER, SECOND_AREA);
    else if(ev.xbutton.x == (long)xinfo.screen_width-1)
        move_client(from, NULL, TILE_LAYER, FIXED_AREA);
    else if(win==xinfo.root_win && subw==None)
        move_client(from, NULL, TILE_LAYER, MAIN_AREA);
    else if(to)
        move_client(from, to, ANY_LAYER, ANY_AREA);
}

static bool get_valid_click(Pointer_act act, XEvent *oe, XEvent *ne)
{
    if(!grab_pointer(xinfo.root_win, act))
        return false;

    do
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, ne);
        event_handler(ne);
    }while(!is_match_button_release(&oe->xbutton, &ne->xbutton));
    XUngrabPointer(xinfo.display, CurrentTime);

    return is_equal_modifier_mask(oe->xbutton.state, ne->xbutton.state)
        && is_pointer_on_win(ne->xbutton.window);
}

void client_change_place(Layer layer, Area area)
{
    move_client(get_cur_focus_client(), NULL, layer, area);
}

void pointer_swap_clients(XEvent *e)
{
    XEvent ev;
    Client *from=get_cur_focus_client(), *to=NULL;
    if(get_gwm_layout()!=TILE || !from || !get_valid_click(SWAP, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    if((to=win_to_client(ev.xbutton.subwindow)))
        swap_clients(from, to);
}
