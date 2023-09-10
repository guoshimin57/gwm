/* *************************************************************************
 *     debug.c：實現調試功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

void print_client_and_top_win(WM *wm)
{
    unsigned int n;
    Window root, parent, *child=NULL;

    if(XQueryTree(wm->display, wm->root_win, &root, &parent, &child, &n))
    {
        Client *c=NULL;
        printf(_("以下是自底向頂排列的客戶窗口和分層參照窗口列表：\n"));
        for(unsigned int i=0; i<n; i++)
        {
            if((c=win_to_client(wm, child[i])))
                printf("client frame: %lx\n", c->frame);
            else
                for(unsigned int j=0; j<TOP_WIN_TYPE_N; j++)
                    if(wm->top_wins[j] == child[i])
                        printf("top win: %lx\n", child[i]);
        }
        XFree(child);
    }
    else
        printf(_("無法查詢根窗口的子窗口\n"));
}

void print_win_tree(WM *wm, Window win)
{
    unsigned int n;
    Window root, parent, *child=NULL;

    if(XQueryTree(wm->display, win, &root, &parent, &child, &n))
    {
        printf(_("以下是%lx窗口的子窗口：\n"), win);
        for(unsigned int i=0; i<n; i++)
            printf("%lx\n", child[i]);
        XFree(child);
    }
    else
        printf(_("無法查詢%lx窗口的子窗口\n"), win);
}

void print_net_wm_win_type(WM *wm, Window win)
{
    Net_wm_win_type type=get_net_wm_win_type(wm, win);

    printf(_("以下是%lx窗口窗口類型(即_NET_WM_WINDOW_TYPE)：\n"), win);
    printf("desktop: %d\n", type.desktop);
    printf("dock: %d\n", type.dock);
    printf("toolbar: %d\n", type.toolbar);
    printf("menu: %d\n", type.menu);
    printf("utility: %d\n", type.utility);
    printf("splash: %d\n", type.splash);
    printf("dialog: %d\n", type.dialog);
    printf("dropdown_menu: %d\n", type.dropdown_menu);
    printf("popup_menu: %d\n", type.popup_menu);
    printf("tooltip: %d\n", type.tooltip);
    printf("notification: %d\n", type.notification);
    printf("combo: %d\n", type.combo);
    printf("dnd: %d\n", type.dnd);
    printf("normal: %d\n", type.normal);
    printf("none: %d\n", type.none);
}

void print_net_wm_state(WM *wm, Window win)
{
    Net_wm_state state=get_net_wm_state(wm, win);

    printf(_("以下是%lx窗口窗口狀態(即_NET_WM_STATE)：\n"), win);
    printf("modal: %d\n", state.modal);
    printf("sticky: %d\n", state.sticky);
    printf("vmax: %d\n", state.vmax);
    printf("hmax: %d\n", state.hmax);
    printf("shaded: %d\n", state.shaded);
    printf("skip_taskbar: %d\n", state.skip_taskbar);
    printf("skip_pager: %d\n", state.skip_pager);
    printf("hidden: %d\n", state.hidden);
    printf("fullscreen: %d\n", state.fullscreen);
    printf("above: %d\n", state.above);
    printf("below: %d\n", state.below);
    printf("attent: %d\n", state.attent);
    printf("focused: %d\n", state.focused);
}

int print_atom_name(WM *wm, Atom atom)
{
    for(int i=0; i<ICCCM_ATOMS_N; i++)
        if(wm->icccm_atoms[i] == atom)
            return printf("icccm_atom=%ld, name=%s, index=%d\n", (long)atom, ICCCM_NAMES[i], i);
    for(int i=0; i<EWMH_ATOM_N; i++)
        if(wm->ewmh_atom[i] == atom)
            return printf("ewmh_atom=%ld, name=%s, index=%d\n", (long)atom, EWMH_NAME[i], i);
    return printf("unknown_atom=%ld\n", (long)atom);
}

void print_all_atom_name(WM *wm)
{
    for(int i=0; i<ICCCM_ATOMS_N; i++)
        printf("icccm_atom=%ld, name=%s, index=%d\n", (long)wm->icccm_atoms[i], ICCCM_NAMES[i], i);
    for(int i=0; i<EWMH_ATOM_N; i++)
        printf("ewmh_atom=%ld, name=%s, index=%d\n", (long)wm->ewmh_atom[i], EWMH_NAME[i], i);
}
