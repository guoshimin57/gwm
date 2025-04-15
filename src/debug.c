/* *************************************************************************
 *     debug.c：實現調試功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "config.h"
#include "font.h"
#include "misc.h"
#include "taskbar.h"
#include "icccm.h"
#include "debug.h"

void print_clients(void)
{
    unsigned int n;
    Window root, parent, *child=NULL;

    if(XQueryTree(xinfo.display, xinfo.root_win, &root, &parent, &child, &n))
    {
        Client *c=NULL;
        printf(_("以下是自頂向底排列的客戶窗口和分層參照窗口列表：\n"));
        for(unsigned int i=0; i<n; i++)
            if((c=win_to_client(child[i])))
                printf("client frame: %lx\n", WIDGET_WIN(c->frame));
        XFree(child);
    }
    else
        printf(_("無法查詢根窗口的子窗口\n"));
}

void print_win_tree(Window win)
{
    unsigned int n;
    Window root, parent, *child=NULL;

    if(XQueryTree(xinfo.display, win, &root, &parent, &child, &n))
    {
        printf(_("以下是%lx窗口的子窗口：\n"), win);
        for(unsigned int i=0; i<n; i++)
            printf("%lx\n", child[i]);
        XFree(child);
    }
    else
        printf(_("無法查詢%lx窗口的子窗口\n"), win);
}

void print_net_wm_win_type(Window win)
{
    Net_wm_win_type type=get_net_wm_win_type(win);

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

void print_net_wm_state(Net_wm_state state)
{
    puts(_("以下是窗口狀態(即_NET_WM_STATE)："));
    printf("modal: %d\n", state.modal);
    printf("sticky: %d\n", state.sticky);
    printf("vmax: %d\n", state.vmax);
    printf("hmax: %d\n", state.hmax);
    printf("tmax: %d\n", state.tmax);
    printf("bmax: %d\n", state.bmax);
    printf("lmax: %d\n", state.lmax);
    printf("rmax: %d\n", state.rmax);
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

void print_place_info(Client *c)
{
    printf(_("以下是%lx窗口的位置信息：\n"), WIDGET_WIN(c));
    printf("ox=%d, oy=%d, ow=%d, oh=%d, olayer=%d, oarea=%d\n",
        c->ox, c->oy, c->ow, c->oh, c->olayer, c->oarea);
    printf("x=%d, y=%d, w=%d, h=%d, layer=%d, area=%d\n",
        WIDGET_X(c), WIDGET_Y(c), WIDGET_W(c), WIDGET_H(c), c->layer, c->area);
}

void print_all_client_win(void)
{
    printf(_("以下是自頂向底排列的客戶窗口列表：\n"));
    clients_for_each(c)
        print_client_win(c);
}

void print_client_win(Client *c)
{
    printf("win=%lx (frame=%lx)\n", WIDGET_WIN(c), WIDGET_WIN(c->frame));
}

void print_widget_state(Widget_state state)
{
    printf("disable=%d\n", state.disable);
    printf("active=%d\n", state.active);
    printf("warn=%d\n", state.warn);
    printf("hot=%d\n", state.hot);
    printf("urgent=%d\n", state.urgent);
    printf("attent=%d\n", state.attent);
    printf("chosen=%d\n", state.chosen);
    printf("unfocused=%d\n", state.unfocused);
}

void print_net_client_lists(void)
{
    unsigned long i=0, n=0;
    Window *wlist=NULL;

    wlist=get_net_client_list(&n);
    puts("從早到晚排列的客戶窗口列表：");
    for(i=0; i<n; i++)
        printf("%lx%s", wlist[i], i<n-1 ? ", " : "\n");
    Free(wlist);

    wlist=get_net_client_list_stacking(&n);
    puts("從下到上排列的客戶窗口列表：");
    for(i=0; i<n; i++)
        printf("%lx%s", wlist[i], i<n-1 ? ", " : "\n");
    Free(wlist);
}

void print_size_hints(Window win)
{
    XSizeHints h=get_size_hint(win);
    printf("以下是%lx窗口的尺寸條件特性：\n", win);
    printf("flags=%lx, x=%d, y=%d, w=%d, h=%d\n"
           "wmin=%d, hmin=%d, wmax=%d, hmax=%d, winc=%d, hinc=%d\n"
           "amin.x=%d, amin.y=%d, amax.x=%d, amax.y=%d\n"
           "wbase=%d, hbase=%d, gravity=%d\n",
            h.flags, h.x, h.y, h.width, h.height,
            h.min_width, h.min_height, h.max_width, h.max_height,
            h.width_inc, h.height_inc,
            h.min_aspect.x, h.min_aspect.y, h.max_aspect.x, h.max_aspect.y,
            h.base_width, h.base_height, h.win_gravity);
}
