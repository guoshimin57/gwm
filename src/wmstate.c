/* *************************************************************************
 *     wmstate.c：實現處理窗口狀態(即_NET_WM_STATE)的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "misc.h"
#include "prop.h"
#include "clientop.h"
#include "focus.h"
#include "taskbar.h"
#include "wmstate.h"

#define NET_WM_STATE_REMOVE 0
#define NET_WM_STATE_ADD    1
#define NET_WM_STATE_TOGGLE 2

#define SHOULD_ADD_STATE(c, act, flag) \
    (act==NET_WM_STATE_ADD || (act==NET_WM_STATE_TOGGLE && !c->win_state.flag))

static void change_net_wm_state_for_modal(Client *c, long act);
static void change_net_wm_state_for_sticky(Client *c, long act);
static void change_net_wm_state_for_vmax(Client *c, long act);
static void change_net_wm_state_for_hmax(Client *c, long act);
static void change_net_wm_state_for_tmax(Client *c, long act);
static void change_net_wm_state_for_bmax(Client *c, long act);
static void change_net_wm_state_for_lmax(Client *c, long act);
static void change_net_wm_state_for_rmax(Client *c, long act);
static void change_net_wm_state_for_hidden(Client *c, long act);
static void change_net_wm_state_for_fullscreen(Client *c, long act);
static void change_net_wm_state_for_shaded(Client *c, long act);
static void change_net_wm_state_for_skip_taskbar(Client *c, long act);
static void change_net_wm_state_for_skip_pager(Client *c, long act);
static void change_net_wm_state_for_above(Client *c, long act);
static void change_net_wm_state_for_below(Client *c, long act);
static void change_net_wm_state_for_attent(Client *c, long act);
static void change_net_wm_state_for_focused(Client *c, long act);

void change_net_wm_state(Client *c, long *full_act)
{
    long act=full_act[0];
    if((act!=NET_WM_STATE_REMOVE && act!=NET_WM_STATE_ADD && act!=NET_WM_STATE_TOGGLE))
        return;

    Net_wm_state mask=get_net_wm_state_mask(full_act);

    if(mask.modal)          change_net_wm_state_for_modal(c, act);
    if(mask.sticky)         change_net_wm_state_for_sticky(c, act);
    if(mask.vmax)           change_net_wm_state_for_vmax(c, act);
    if(mask.hmax)           change_net_wm_state_for_hmax(c, act);
    if(mask.tmax)           change_net_wm_state_for_tmax(c, act);
    if(mask.bmax)           change_net_wm_state_for_bmax(c, act);
    if(mask.lmax)           change_net_wm_state_for_lmax(c, act);
    if(mask.rmax)           change_net_wm_state_for_rmax(c, act);
    if(mask.shaded)         change_net_wm_state_for_shaded(c, act);
    if(mask.skip_taskbar)   change_net_wm_state_for_skip_taskbar(c, act);
    if(mask.skip_pager)     change_net_wm_state_for_skip_pager(c, act);
    if(mask.hidden)         change_net_wm_state_for_hidden(c, act);
    if(mask.fullscreen)     change_net_wm_state_for_fullscreen(c, act);
    if(mask.above)          change_net_wm_state_for_above(c, act);
    if(mask.below)          change_net_wm_state_for_below(c, act);
    if(mask.attent)         change_net_wm_state_for_attent(c, act);
    if(mask.focused)        change_net_wm_state_for_focused(c, act);

    update_net_wm_state(WIDGET_WIN(c), c->win_state);
}

static void change_net_wm_state_for_modal(Client *c, long act)
{
    c->win_state.modal=SHOULD_ADD_STATE(c, act, modal);
}

static void change_net_wm_state_for_sticky(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, sticky);

    if(add)
        c->desktop_mask=~0U;
    else
        c->desktop_mask=get_desktop_mask(get_net_current_desktop());
    request_layout_update();
    c->win_state.sticky=add;
}

static void change_net_wm_state_for_vmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, vmax))
        maximize_client(c, VERT_MAX);
    else
        restore_client(c);
}

static void change_net_wm_state_for_hmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hmax))
        maximize_client(c, HORZ_MAX);
    else
        restore_client(c);
}

static void change_net_wm_state_for_tmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, tmax))
        maximize_client(c, TOP_MAX);
    else
        restore_client(c);
}

static void change_net_wm_state_for_bmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, bmax))
        maximize_client(c, BOTTOM_MAX);
    else
        restore_client(c);
}

static void change_net_wm_state_for_lmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, lmax))
        maximize_client(c, LEFT_MAX);
    else
        restore_client(c);
}

static void change_net_wm_state_for_rmax(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, rmax))
        maximize_client(c, RIGHT_MAX);
    else
        restore_client(c);
}

static void change_net_wm_state_for_hidden(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, hidden))
        iconify_client(c);
    else
        deiconify_client(is_iconic_client(c) ? c : NULL);
}

static void change_net_wm_state_for_fullscreen(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, fullscreen))
        set_fullscreen(c);
    else
        restore_client(c);
}

static void change_net_wm_state_for_shaded(Client *c, long act)
{
    toggle_shade_mode(c, SHOULD_ADD_STATE(c, act, shaded));
}

static void change_net_wm_state_for_skip_taskbar(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, skip_taskbar);

    if(add && is_iconic_client(c))
        deiconify_client(c);
    c->win_state.skip_taskbar=add;
}

/* 暫未實現分頁器 */
static void change_net_wm_state_for_skip_pager(Client *c, long act)
{
    c->win_state.skip_pager=SHOULD_ADD_STATE(c, act, skip_pager);
}

static void change_net_wm_state_for_above(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, above))
    {
        if(c->layer != ABOVE_LAYER)
            save_place_info_of_client(c);
        move_client(c, NULL, ABOVE_LAYER, ANY_AREA);
    }
    else
        restore_client(c);
}

static void change_net_wm_state_for_below(Client *c, long act)
{
    if(SHOULD_ADD_STATE(c, act, below))
    {
        if(c->layer != BELOW_LAYER)
            save_place_info_of_client(c);
        move_client(c, NULL, BELOW_LAYER, ANY_AREA);
    }
    else
        restore_client(c);
}

static void change_net_wm_state_for_attent(Client *c, long act)
{
    c->win_state.attent=SHOULD_ADD_STATE(c, act, attent);
    taskbar_set_attention(c->desktop_mask);
}

static void change_net_wm_state_for_focused(Client *c, long act)
{
    bool add=SHOULD_ADD_STATE(c, act, focused);

    focus_client(add ? c : NULL);
    c->win_state.focused=add;
}
