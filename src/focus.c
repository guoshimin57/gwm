/* *************************************************************************
 *     focus.c：實現窗口聚焦功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "icccm.h"
#include "desktop.h"
#include "focus.h"

typedef enum // 窗口疊次序分層類型
{
    FULLSCREEN_TOP,
    ABOVE_TOP,
    DOCK_TOP,
    FLOAT_TOP,
    NORMAL_TOP,
    BELOW_TOP,
    DESKTOP_TOP,
    TOP_WIN_TYPE_N
} Top_win_type;

static void update_focus_client_pointer(Client *c);
static bool is_map_client(Client *c);
static Client *get_first_map_client(void);
static Client *get_first_map_diff_client(Client *key);
static void raise_client(Client *c);
static Window get_top_win(Client *c);

Window top_wins[TOP_WIN_TYPE_N]; // 窗口疊次序分層參照窗口列表，即分層層頂窗口

/* 若在調用本函數之前cur_focus_client或prev_focus_client因某些原因（如移動到
 * 其他虛擬桌面、刪除、縮微）而未更新時，則應使用值爲NULL的c來調用本函數。這
 * 樣會自動推斷出合適的規則來取消原聚焦和聚焦新的client。*/
void focus_client(Client *c)
{
    update_focus_client_pointer(c);

    Client *pc=get_cur_focus_client(), *pp=get_prev_focus_client();

    if(!pc)
        XSetInputFocus(xinfo.display, xinfo.root_win, RevertToPointerRoot, CurrentTime);
    else if(!is_iconic_client(pc))
        set_input_focus(WIDGET_WIN(pc), pc->wm_hint);

    if(pc)
    {
        client_set_state_unfocused(pc, 0);
        update_client_bg(pc);
        raise_client(pc);
    }

    if(pp && pp!=pc)
    {
        client_set_state_unfocused(pp, 1);
        update_client_bg(pp);
    }

    set_net_active_window(pc ? WIDGET_WIN(pc) : None);
}

static void update_focus_client_pointer(Client *c)
{
    Client *p=NULL, *pf=get_prev_focus_client(), *cf=get_cur_focus_client(),
           *po=(pf ? pf->owner : NULL), *co=(cf ? cf->owner : NULL);

    if(c == cf)
        return;
    else if(!c) // 某個client可能被刪除、縮微化、移動到其他桌面、非wm手段關閉了
    {
        if(!is_map_client(cf) && !is_map_client(cf = co ? co : pf))
            cf=get_first_map_client();
        if((!is_map_client(pf) || pf==cf) && !is_map_client(pf=po))
            pf=get_first_map_diff_client(pf);
    }
    else
    {
        p=get_top_transient_client(c->subgroup_leader, true);
        pf=cf, cf=(p ? p : c);
    }
    if(pf == cf)
        pf=get_first_map_diff_client(cf);

    set_prev_focus_client(pf);
    set_cur_focus_client(cf);
}

static bool is_map_client(Client *c)
{
    if(c && !is_iconic_client(c) && is_on_cur_desktop(c->desktop_mask))
        clients_for_each(p)
            if(p == c)
                return true;
    return false;
}

static Client *get_first_map_client(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c))
            return c;
    return NULL;
}

static Client *get_first_map_diff_client(Client *key)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_iconic_client(c) && c!=key)
            return c;
    return NULL;
}

/* 僅在移動窗口、聚焦窗口時或窗口類型、狀態發生變化才有可能需要提升 */
static void raise_client(Client *c)
{
    int n=get_subgroup_n(c), i=n;
    Window wins[n+1];

    wins[0]=get_top_win(c);
    subgroup_for_each(p, c->subgroup_leader)
        wins[i--]=WIDGET_WIN(p->frame);

    XRestackWindows(xinfo.display, wins, n+1);
    set_all_net_client_list();
}

static Window get_top_win(Client *c)
{
    size_t index[]=
    {
        [FULLSCREEN_LAYER]=FULLSCREEN_TOP,
        [ABOVE_LAYER]=ABOVE_TOP,
        [DOCK_LAYER]=DOCK_TOP,
        [FLOAT_LAYER]=FLOAT_TOP,
        [TILE_LAYER_MAIN]=NORMAL_TOP,
        [TILE_LAYER_SECOND]=NORMAL_TOP,
        [TILE_LAYER_FIXED]=NORMAL_TOP,
        [BELOW_LAYER]=BELOW_TOP,
        [DESKTOP_LAYER]=DESKTOP_TOP,
    };
    return top_wins[index[c->place_type]];
}

void create_refer_top_wins(void)
{
    for(int i=TOP_WIN_TYPE_N-1; i>=0; i--)
        top_wins[i]=create_widget_win(xinfo.root_win, -1, -1, 1, 1, 0, 0, 0);
}

void del_refer_top_wins(void)
{
    for(size_t i=0; i<TOP_WIN_TYPE_N; i++)
        XDestroyWindow(xinfo.display, top_wins[i]);
}
