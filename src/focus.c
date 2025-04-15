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
#include "focus.h"

static void update_focus_client_pointer(Client *c);
static bool is_viewable_client(const Client *c);
static Client *get_first_map_client(void);
static Client *get_first_map_diff_client(Client *key);
static void raise_client(Client *c);
static Window get_top_win(const Client *c);
static void set_all_net_client_list(void);
static Window *get_client_win_list(int *n);
static Window *get_client_win_list_stacking(int *n);

// 分別爲各桌面的當前聚焦結點、前一個聚焦結點数组
static Client *cur_focus_client[DESKTOP_N]={NULL};
static Client *prev_focus_client[DESKTOP_N]={NULL};

Window top_wins[LAYER_N]; // 窗口疊次序分層參照窗口列表，即分層層頂窗口

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
        raise_client(pp);
    }

    set_net_active_window(pc ? WIDGET_WIN(pc) : None);
    set_all_net_client_list();
}

void set_cur_focus_client(Client *c)
{
    cur_focus_client[get_net_current_desktop()]=c;
}

Client *get_cur_focus_client(void)
{
    return cur_focus_client[get_net_current_desktop()];
}

void set_prev_focus_client(Client *c)
{
    prev_focus_client[get_net_current_desktop()]=c;
}

Client *get_prev_focus_client(void)
{
    return prev_focus_client[get_net_current_desktop()];
}

static void update_focus_client_pointer(Client *c)
{
    Client *p=NULL, *pf=get_prev_focus_client(), *cf=get_cur_focus_client(),
           *po=(pf ? pf->owner : NULL), *co=(cf ? cf->owner : NULL);

    if(c == cf)
        return;
    else if(!c) // 某個client可能被刪除、縮微化、移動到其他桌面、非wm手段關閉了
    {
        if(!is_viewable_client(cf) && !is_viewable_client(cf = co ? co : pf))
            cf=get_first_map_client();
        if(!is_viewable_client(pf) || pf==cf)
            pf= (po && po!=cf && is_viewable_client(po)) ? po : get_first_map_diff_client(cf);
    }
    else
    {
        p=get_top_transient_client(c->subgroup_leader, true);
        pf=cf, cf=(p ? p : c);
    }

    set_prev_focus_client(pf);
    set_cur_focus_client(cf);
}

static bool is_viewable_client(const Client *c)
{
    XWindowAttributes a;

    return c && XGetWindowAttributes(xinfo.display, WIDGET_WIN(c), &a)
        && a.map_state==IsViewable;
}

static Client *get_first_map_client(void)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_viewable_client(c))
            return c;
    return NULL;
}

static Client *get_first_map_diff_client(Client *key)
{
    clients_for_each(c)
        if(is_on_cur_desktop(c->desktop_mask) && is_viewable_client(c) && c!=key)
            return c;
    return NULL;
}

/* 僅在移動窗口、聚焦窗口時或窗口類型、狀態發生變化才有可能需要提升 */
static void raise_client(Client *c)
{
    if(c == get_cur_focus_client())
        XRaiseWindow(xinfo.display, WIDGET_WIN(c->frame));
    else
    {
        int n=get_subgroup_n(c), i=n;
        Window wins[n+1];

        wins[0]=get_top_win(c);
        subgroup_for_each(p, c->subgroup_leader)
            wins[i--]=WIDGET_WIN(p->frame);

        XRestackWindows(xinfo.display, wins, n+1);
    }
}

static Window get_top_win(const Client *c)
{
    return top_wins[c->layer];
}

void create_layer_wins(void)
{
    for(int i=LAYER_N-1; i>=0; i--)
        top_wins[i]=create_widget_win(xinfo.root_win, -1, -1, 1, 1, 0, 0, 0);
}

void del_layer_wins(void)
{
    for(size_t i=0; i<LAYER_N; i++)
        XDestroyWindow(xinfo.display, top_wins[i]);
}

static void set_all_net_client_list(void)
{
    int n=0;
    Window *wlist=get_client_win_list(&n);

    if(wlist)
        set_net_client_list(wlist, n), Free(wlist);

    wlist=get_client_win_list_stacking(&n);
    if(wlist)
        set_net_client_list_stacking(wlist, n), Free(wlist);
}

/* 獲取當前桌面按從早到遲的映射順序排列的客戶窗口列表 */
static Window *get_client_win_list(int *n)
{
    *n=get_clients_n(ANY_LAYER, ANY_AREA, true, true, true);
    if(*n == 0)
        return NULL;

    unsigned long i=0, j=0, new_n=*n, old_n=0;
    Window *old_list=get_net_client_list(&old_n);
    Window *wlist=NULL;

    if(new_n > old_n) // 添加了客戶窗口
    {
        wlist=Malloc(new_n*sizeof(Window));
        for(unsigned long i=0; i<old_n; i++)
            wlist[i]=old_list[i];
        clients_for_each(c)
        {
            for(i=0; i<old_n && WIDGET_WIN(c)!=old_list[i]; i++)
                ;
            if(i == old_n)
                { wlist[old_n]=WIDGET_WIN(c); break; }
        }
    }
    else if(new_n < old_n) // 刪除了客戶窗口
    {
        wlist=Malloc(new_n*sizeof(Window));
        for(i=0, j=0; i<old_n; i++)
            if(win_to_client(old_list[i]))
                wlist[j++]=old_list[i];
    }

    XFree(old_list);

    return wlist;
}

/* 獲取當前桌面按從下到上的疊次序排列的客戶窗口列表 */
static Window *get_client_win_list_stacking(int *n)
{
    *n=get_clients_n(ANY_LAYER, ANY_AREA, true, true, true);
    if(*n == 0)
        return NULL;

    unsigned int tree_n=0;
    Window *tree=query_win_list(&tree_n);
    if(!tree)
        return NULL;

    Client *c=NULL;
    Window *wlist=Malloc(*n*sizeof(Window));
    for(unsigned int i=0, j=0; i<tree_n; i++)
        if((c=win_to_client(tree[i])))
            wlist[j++]=WIDGET_WIN(c);
    Free(tree);

    return wlist;
}
