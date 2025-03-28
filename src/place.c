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

#include "func.h"
#include "minimax.h"
#include "prop.h"
#include "layout.h"
#include "focus.h"
#include "place.h"

static void key_change_place(Place_type type);
static bool is_valid_move(Client *from, Client *to, Place_type type);
static bool is_valid_to_normal_layer_sec(Client *c);
static int cmp_client_store_order(Client *c1, Client *c2);
static void set_place_type_for_subgroup(Client *subgroup_leader, Place_type type);
static void swap_clients(Client *a, Client *b);

void pointer_change_place(WM *wm, XEvent *e, Arg arg)
{
    XEvent ev;
    Client *from=get_cur_focus_client(), *to;

    UNUSED(arg);
    if(!is_spec_layout(TILE) || !from || !get_valid_click(wm, CHANGE, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    Window win=ev.xbutton.window, subw=ev.xbutton.subwindow;
    to=win_to_client(subw);
    if(ev.xbutton.x == 0)
        move_client(from, NULL, TILE_LAYER_SECOND);
    else if(ev.xbutton.x == (long)xinfo.screen_width-1)
        move_client(from, NULL, TILE_LAYER_FIXED);
    else if(win==xinfo.root_win && subw==None)
        move_client(from, NULL, TILE_LAYER_MAIN);
    else if(to)
        move_client(from, to, ANY_PLACE);
    update_net_wm_state_for_no_max(WIDGET_WIN(from), from->win_state);
}

void change_to_main(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    key_change_place(TILE_LAYER_MAIN);
}

void change_to_second(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    key_change_place(TILE_LAYER_SECOND);
}

void change_to_fixed(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    key_change_place(TILE_LAYER_FIXED);
}

void change_to_float(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    key_change_place(FLOAT_LAYER);
}

static void key_change_place(Place_type type)
{
    Client *c=get_cur_focus_client();
    move_client(c, NULL, type);
    update_net_wm_state_for_no_max(WIDGET_WIN(c), c->win_state);
}

void pointer_swap_clients(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(arg);
    XEvent ev;
    Client *from=get_cur_focus_client(), *to=NULL;
    if(!is_spec_layout(TILE) || !from || !get_valid_click(wm, SWAP, e, &ev))
        return;

    /* 因爲窗口不隨定位器動態移動，故釋放按鈕時定位器已經在按下按鈕時
     * 定位器所在的窗口的外邊。因此，接收事件的是根窗口。 */
    if((to=win_to_client(ev.xbutton.subwindow)))
        swap_clients(from, to);
}

void show_desktop(WM *wm, XEvent *e, Arg arg)
{
    UNUSED(wm), UNUSED(e), UNUSED(arg);
    static bool show=false;

    toggle_showing_desktop_mode(show=!show);
}

void toggle_showing_desktop_mode(bool show)
{
    if(show)
        iconify_all_clients();
    else
        deiconify_all_clients();
    set_net_showing_desktop(show);
}

void move_client(Client *from, Client *to, Place_type type)
{
    if(move_client_node(from, to, type))
    {
        set_place_type_for_subgroup(from->subgroup_leader,
            to ? to->place_type : type);
        request_layout_update();
    }
}

bool move_client_node(Client *from, Client *to, Place_type type)
{
    if(!is_valid_move(from, to, type))
        return false;

    Client *head=NULL;
    del_subgroup(from->subgroup_leader);
    if(to)
        head = cmp_client_store_order(from, to) < 0 ? to : list_prev_entry(to, Client, list);
    else
    {
        head=get_head_client(type);
        if(from->place_type==TILE_LAYER_MAIN && type==TILE_LAYER_SECOND)
            head=list_next_entry(head, Client, list);
    }
    add_subgroup(head, from->subgroup_leader);
    return true;
}

static bool is_valid_move(Client *from, Client *to, Place_type type)
{
    Place_type t = to ? to->place_type : type;

    return from
        && (!to || from->subgroup_leader!=to->subgroup_leader)
        && (t!=TILE_LAYER_SECOND || is_valid_to_normal_layer_sec(from))
        && (is_spec_layout(TILE) || !is_normal_layer(t));
}

static bool is_valid_to_normal_layer_sec(Client *c)
{
    return c->place_type!=TILE_LAYER_MAIN
        || get_clients_n(TILE_LAYER_SECOND, false, false, false);
}

static int cmp_client_store_order(Client *c1, Client *c2)
{
    if(c1 == c2)
        return 0;
    clients_for_each_from(c1)
        if(c1 == c2)
            return -1;
    return 1;
}

static void set_place_type_for_subgroup(Client *subgroup_leader, Place_type type)
{
    for(Client *ld=subgroup_leader, *c=ld; ld && c->subgroup_leader==ld; c=list_prev_entry(c, Client, list))
        c->place_type=type;
}

static void swap_clients(Client *a, Client *b)
{
    if(a->subgroup_leader == b->subgroup_leader)
        return;

    Client *tmp, *top, *a_begin, *b_begin, *a_prev, *a_leader, *b_leader;

    if(cmp_client_store_order(a, b) > 0)
        tmp=a, a=b, b=tmp;

    a_leader=a->subgroup_leader, b_leader=b->subgroup_leader;

    top=get_top_transient_client(a_leader, false);
    a_begin=(top ? top : a_leader);
    a_prev=list_prev_entry(a_begin, Client, list);

    top=get_top_transient_client(b_leader, false);
    b_begin=(top ? top : b_leader);

    del_subgroup(a_leader);
    add_subgroup(b_leader, a_leader);
    if(list_next_entry(a_leader, Client, list) != b_begin) //不相邻
        del_subgroup(b_leader), add_subgroup(a_prev, b_leader);

    request_layout_update();
}
