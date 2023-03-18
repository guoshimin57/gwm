/* *************************************************************************
 *     focus.c：實現窗口聚焦相關功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "client.h"
#include "desktop.h"
#include "drawable.h"
#include "focus.h"

static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c);
static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c);
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c);
static void update_client_look(WM *wm, unsigned int desktop_n, Client *c);

/* 若在調用本函數之前cur_focus_client或prev_focus_client因某些原因
 * （如移動到其他虛擬桌面）而未更新時，則應使用值爲NULL的c來調用本
 * 函數。這樣會自動推斷出合適的規則來取消原聚焦和聚焦新的client。*/
void focus_client(WM *wm, unsigned int desktop_n, Client *c)
{
    update_focus_client_pointer(wm, desktop_n, c);

    Desktop *d=wm->desktop+desktop_n-1;
    Client *pc=d->cur_focus_client;

    if(desktop_n == wm->cur_desktop)
    {
        if(pc->win == wm->root_win)
            XSetInputFocus(wm->display, wm->root_win, RevertToPointerRoot, CurrentTime);
        else if(pc->area_type != ICONIFY_AREA)
            set_input_focus(wm, pc->wm_hint, pc->win);
    }
    update_client_look(wm, desktop_n, pc);
    update_client_look(wm, desktop_n, d->prev_focus_client);
    if(pc->area_type!=ICONIFY_AREA || d->cur_layout==PREVIEW)
        raise_client(wm, desktop_n);
}

static void update_focus_client_pointer(WM *wm, unsigned int desktop_n, Client *c)
{
    Desktop *d=wm->desktop+desktop_n-1;
    Client *p=NULL, **pp=&d->prev_focus_client, **pc=&d->cur_focus_client;
 
    if(!c)  // 當某個client在desktop_n中變得不可見時，即既有可能被刪除了，
    {       // 也可能是被縮微化了，還有可能是移動到其他虛擬桌面了。
        if(!is_map_client(wm, desktop_n, *pc))
            p=win_to_client(wm, (*pc)->owner);
        if(!is_map_client(wm, desktop_n, p))
            p=*pp;
        if(!is_map_client(wm, desktop_n, p))
            p=get_prev_map_client(wm, desktop_n, *pp);
        if(!p)
            p=get_next_map_client(wm, desktop_n, *pp);
        *pc = p ? p : wm->clients;
        if(!is_map_client(wm, desktop_n, *pp) || *pp==*pc)
            p=win_to_client(wm, (*pp)->owner);
        if(!is_map_client(wm, desktop_n, p))
            p=get_prev_map_client(wm, desktop_n, *pp);
        if(!p)
            p=get_next_map_client(wm, desktop_n, *pp);
        *pp = p ? p : wm->clients;
    }
    else if(c != *pc)
        *pp=*pc, *pc=c;
}

static bool is_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    if(c && c->area_type!=ICONIFY_AREA && is_on_desktop_n(desktop_n, c))
        for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
            if(p == c)
                return true;
    return false;
}

/* 取得存儲結構意義上的上一個處於映射狀態的客戶窗口 */
static Client *get_prev_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->prev; p!=wm->clients; p=p->prev)
        if(p->area_type!=ICONIFY_AREA && is_on_desktop_n(desktop_n, p))
            return p;
    return NULL;
}

/* 取得存儲結構意義上的下一個處於映射狀態的客戶窗口 */
static Client *get_next_map_client(WM *wm, unsigned int desktop_n, Client *c)
{
    for(Client *p=c->next; p!=wm->clients; p=p->next)
        if(p->area_type!=ICONIFY_AREA && is_on_desktop_n(desktop_n, p))
            return p;
    return NULL;
}

void set_input_focus(WM *wm, XWMHints *hint, Window win)
{
    if(!hint || ((hint->flags & InputHint) && hint->input)) // 不抗拒鍵盤輸入
        XSetInputFocus(wm->display, win, RevertToPointerRoot, CurrentTime);
    send_event(wm, wm->icccm_atoms[WM_TAKE_FOCUS], win);
}

static void update_client_look(WM *wm, unsigned int desktop_n, Client *c)
{
    if(c && c!=wm->clients)
    {
        Desktop *d=wm->desktop+desktop_n-1;
        if(c->area_type==ICONIFY_AREA && d->cur_layout!=PREVIEW)
            update_win_background(wm, c->icon->win, c==d->cur_focus_client ?
                wm->widget_color[ENTERED_NORMAL_BUTTON_COLOR].pixel :
                wm->widget_color[ICON_AREA_COLOR].pixel, None);
        else
            update_frame(wm, desktop_n,  c);
    }
}
