/* *************************************************************************
 *     ewmh.c：實現EWMH規範。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "prop.h"
#include "icccm.h"
#include "ewmh.h"

static const char *ewmh_atom_names[EWMH_ATOM_N]= // EWMH規範標識符名稱
{
    "_NET_SUPPORTED", "_NET_CLIENT_LIST", "_NET_CLIENT_LIST_STACKING",
    "_NET_NUMBER_OF_DESKTOPS", "_NET_DESKTOP_GEOMETRY",
    "_NET_DESKTOP_VIEWPORT", "_NET_CURRENT_DESKTOP", "_NET_DESKTOP_NAMES",
    "_NET_ACTIVE_WINDOW", "_NET_WORKAREA", "_NET_SUPPORTING_WM_CHECK",
    "_NET_VIRTUAL_ROOTS", "_NET_DESKTOP_LAYEROUT", "_NET_SHOWING_DESKTOP",
    "_NET_CLOSE_WINDOW", "_NET_MOVERESIZE_WINDOW", "_NET_WM_MOVERESIZE",
    "_NET_RESTACK_WINDOW", "_NET_REQUEST_FRAME_EXTENTS", "_NET_WM_NAME",
    "_NET_WM_VISIBLE_NAME", "_NET_WM_ICON_NAME", "_NET_WM_VISIBLE_ICON_NAME",
    "_NET_WM_DESKTOP", "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_DESKTOP",
    "_NET_WM_WINDOW_TYPE_DOCK", "_NET_WM_WINDOW_TYPE_TOOLBAR",
    "_NET_WM_WINDOW_TYPE_MENU", "_NET_WM_WINDOW_TYPE_UTILITY",
    "_NET_WM_WINDOW_TYPE_SPLASH", "_NET_WM_WINDOW_TYPE_DIALOG",
    "_NET_WM_WINDOW_TYPE_DROPDOWN_MENU", "_NET_WM_WINDOW_TYPE_POPUP_MENU",
    "_NET_WM_WINDOW_TYPE_TOOLTIP", "_NET_WM_WINDOW_TYPE_NOTIFICATION",
    "_NET_WM_WINDOW_TYPE_COMBO", "_NET_WM_WINDOW_TYPE_DND",
    "_NET_WM_WINDOW_TYPE_NORMAL", "_NET_WM_STATE", "_NET_WM_STATE_MODAL",
    "_NET_WM_STATE_STICKY", "_NET_WM_STATE_MAXIMIZED_VERT",
    "_NET_WM_STATE_MAXIMIZED_HORZ", "_NET_WM_STATE_SHADED",
    "_NET_WM_STATE_SKIP_TASKBAR", "_NET_WM_STATE_SKIP_PAGER",
    "_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_FULLSCREEN",
    "_NET_WM_STATE_ABOVE", "_NET_WM_STATE_BELOW",
    "_NET_WM_STATE_DEMANDS_ATTENTION", "_NET_WM_STATE_FOCUSED",
    "_NET_WM_ALLOWED_ACTIONS", "_NET_WM_ACTION_MOVE",
    "_NET_WM_ACTION_RESIZE", "_NET_WM_ACTION_MINIMIZE",
    "_NET_WM_ACTION_SHADE", "_NET_WM_ACTION_STICK",
    "_NET_WM_ACTION_MAXIMIZE_HORZ", "_NET_WM_ACTION_MAXIMIZE_VERT",
    "_NET_WM_ACTION_FULLSCREEN", "_NET_WM_ACTION_CHANGE_DESKTOP",
    "_NET_WM_ACTION_CLOSE", "_NET_WM_ACTION_ABOVE", "_NET_WM_ACTION_BELOW",
    "_NET_WM_STRUT", "_NET_WM_STRUT_PARTIAL", "_NET_WM_ICON_GEOMETRY",
    "_NET_WM_ICON", "_NET_WM_PID", "_NET_WM_HANDLED_ICONS",
    "_NET_WM_USER_TIME", "_NET_WM_USER_TIME_WINDOW", "_NET_FRAME_EXTENTS",
    "_NET_WM_OPAQUE_REGION", "_NET_WM_BYPASS_COMPOSITOR", "_NET_WM_PING",
    "_NET_WM_SYNC_REQUEST", "_NET_WM_FULLSCREEN_MONITORS",
    "_NET_WM_FULL_PLACEMENT",
    "GWM_WM_STATE_MAXIMIZED_TOP", "GWM_WM_STATE_MAXIMIZED_BOTTOM",
    "GWM_WM_STATE_MAXIMIZED_LEFT", "GWM_WM_STATE_MAXIMIZED_RIGHT",
};

static Atom ewmh_atoms[EWMH_ATOM_N]; // EWMH規範標識符，與上表相應

bool is_spec_ewmh_atom(Atom spec, EWMH_atom_id id)
{
    return spec==ewmh_atoms[id];
}

void set_ewmh_atoms(void)
{
    for(int i=0; i<EWMH_ATOM_N; i++)
        ewmh_atoms[i]=XInternAtom(xinfo.display, ewmh_atom_names[i], False);
}

void set_net_supported(void)
{
    Atom prop=ewmh_atoms[NET_SUPPORTED];
    replace_atom_prop(xinfo.root_win, prop, ewmh_atoms, EWMH_ATOM_N);
}

/* 設置當前桌面按從早到遲的映射順序排列的客戶窗口列表。需要注意的是，EWMH沒說
 * 針對當前桌面，而說所有X窗口。這是不太合理的，絕大部分任務欄也是把EWMH所說的
 * 所有X窗口視爲前桌面的。因此，gwm也這麼做以兼容這些任務欄。詳見：
 * https://specifications.freedesktop.org/wm-spec/wm-spec-latest.html#idm46485863998912
 * */
void set_net_client_list(const Window *wins, int n)
{
    set_net_client_list_by_order(wins, n, false);
}

/* 設置當前桌面按從下到上的疊次序排列的客戶窗口列表。僅針對當前桌面的理由同上。 */
void set_net_client_list_stacking(const Window *wins, int n)
{
    set_net_client_list_by_order(wins, n, true);
}

/* 設置當前桌面按指定疊次序排列的客戶窗口列表。僅針對當前桌面的理由同上。 */
void set_net_client_list_by_order(const Window *wins, int n, bool stack)
{
    Atom prop=ewmh_atoms[stack ? NET_CLIENT_LIST_STACKING : NET_CLIENT_LIST];

    if(n == 0)
        XDeleteProperty(xinfo.display, xinfo.root_win, prop);
    else
        replace_window_prop(xinfo.root_win, prop, wins, n);
}

void set_net_number_of_desktops(int n)
{
    Atom prop=ewmh_atoms[NET_NUMBER_OF_DESKTOPS];
    long num=n;

    replace_cardinal_prop(xinfo.root_win, prop, &num, 1);
}

int get_net_number_of_desktops(void)
{
    Atom prop=ewmh_atoms[NET_NUMBER_OF_DESKTOPS];
    CARD32 *p=get_cardinal_prop(xinfo.root_win, prop);
    return p ? *p : 1;
}

void set_net_desktop_geometry(int w, int h)
{
    Atom prop=ewmh_atoms[NET_DESKTOP_GEOMETRY];
    long size[2]={w, h};
    
    replace_cardinal_prop(xinfo.root_win, prop, size, 2);
}

void set_net_desktop_viewport(int x, int y)
{
    Atom prop=ewmh_atoms[NET_DESKTOP_GEOMETRY];
    long pos[2]={x, y};

    replace_cardinal_prop(xinfo.root_win, prop, pos, 2);
}

void set_net_current_desktop(unsigned int cur_desktop)
{
    long cur=cur_desktop;
    Atom prop=ewmh_atoms[NET_CURRENT_DESKTOP];

    replace_cardinal_prop(xinfo.root_win, prop, &cur, 1);
}

unsigned int get_net_current_desktop(void)
{
    CARD32 *p=get_cardinal_prop(xinfo.root_win, ewmh_atoms[NET_CURRENT_DESKTOP]);
    return p ? *p : 0;
}

/* 因爲EWMH規定窗口要麼在某個桌面，要麼在所有窗口，不能同時在幾個桌面上，
 * 而gwm支持後者，故不創建與get_net_wm_desktop對應的set_net_wm_desktop。
 */
unsigned int get_net_wm_desktop(Window win)
{
    CARD32 *p=get_cardinal_prop(win, ewmh_atoms[NET_WM_DESKTOP]);
    return p ? *p : get_net_current_desktop();
}

void set_net_desktop_names(const char **names, int n)
{
    Atom prop=ewmh_atoms[NET_DESKTOP_NAMES];
    int size=0;

    for(int i=0; i<n; i++)
        size += strlen(names[i])+1;
    replace_utf8_prop(xinfo.root_win, prop, names, size);
}

void set_net_active_window(Window act_win)
{
    Atom prop=ewmh_atoms[NET_ACTIVE_WINDOW];

    replace_window_prop(xinfo.root_win, prop, &act_win, 1);
}

void set_net_workarea(int x, int y, int w, int h, int ndesktop)
{
    Atom prop=ewmh_atoms[NET_WORKAREA];
    long rect[ndesktop][4];

    for(int i=0; i<ndesktop; i++)
        rect[i][0]=x, rect[i][1]=y, rect[i][2]=w, rect[i][3]=h;
    replace_cardinal_prop(xinfo.root_win, prop, rect[0], ndesktop*4);
}

void set_net_supporting_wm_check(Window check_win, const char *wm_name)
{
    Atom prop=ewmh_atoms[NET_SUPPORTING_WM_CHECK];

    replace_window_prop(xinfo.root_win, prop, &check_win, 1);
    replace_window_prop(check_win, prop, &check_win, 1);
    prop=ewmh_atoms[NET_WM_NAME];
    replace_utf8_prop(check_win, prop, wm_name, strlen(wm_name)+1);
}

void set_net_showing_desktop(bool show)
{
    long showing=show;
    Atom prop=ewmh_atoms[NET_SHOWING_DESKTOP];

    replace_cardinal_prop(xinfo.root_win, prop, &showing, 1);
}

void set_net_wm_allowed_actions(Window win)
{
    Atom prop=ewmh_atoms[NET_WM_ALLOWED_ACTIONS];
    unsigned long acts[]=
    {
        ewmh_atoms[NET_WM_ACTION_MOVE],
        ewmh_atoms[NET_WM_ACTION_RESIZE],
        ewmh_atoms[NET_WM_ACTION_MINIMIZE],
        ewmh_atoms[NET_WM_ACTION_SHADE],
        ewmh_atoms[NET_WM_ACTION_STICK],
        ewmh_atoms[NET_WM_ACTION_MAXIMIZE_HORZ],
        ewmh_atoms[NET_WM_ACTION_MAXIMIZE_VERT],
        ewmh_atoms[NET_WM_ACTION_FULLSCREEN],
        ewmh_atoms[NET_WM_ACTION_CHANGE_DESKTOP],
        ewmh_atoms[NET_WM_ACTION_CLOSE],
        ewmh_atoms[NET_WM_ACTION_ABOVE],
        ewmh_atoms[NET_WM_ACTION_BELOW],
    };

    replace_atom_prop(win, prop, acts, ARRAY_NUM(acts));
}

/* 根據EWMH，窗口可能有多種類型，但實際上絕大部分窗口只設置一種類型 */
Net_wm_win_type get_net_wm_win_type(Window win)
{
    Net_wm_win_type r={0}, unknown={.none=1};
    unsigned long n=0;
    Atom *a=ewmh_atoms,
         *t=(Atom *)get_prop(win, a[NET_WM_WINDOW_TYPE], &n);

    if(!t)
        return unknown;

    for(unsigned long i=0; i<n; i++)
    {
        if     (t[i] == a[NET_WM_WINDOW_TYPE_DESKTOP])       r.desktop=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_DOCK])          r.dock=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_TOOLBAR])       r.toolbar=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_MENU])          r.menu=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_UTILITY])       r.utility=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_SPLASH])        r.splash=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_DIALOG])        r.dialog=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_DROPDOWN_MENU]) r.dropdown_menu=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_POPUP_MENU])    r.popup_menu=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_TOOLTIP])       r.tooltip=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_NOTIFICATION])  r.notification=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_COMBO])         r.combo=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_DND])           r.dnd=1;
        else if(t[i] == a[NET_WM_WINDOW_TYPE_NORMAL])        r.normal=1;
        else                                                 r.none=1;
    }
    XFree(t);

    return r;
}

/* EWMH未說明窗口可否同時有多種狀態，但實際上絕大部分窗口不設置或只設置一種 */
Net_wm_state get_net_wm_state(Window win)
{
    Net_wm_state r={0};
    unsigned long n=0;
    Atom *a=ewmh_atoms, *s=(Atom *)get_prop(win, a[NET_WM_STATE], &n);

    if(!s)
        return r;

    for(unsigned long i=0; i<n; i++)
    {
        if     (s[i] == a[NET_WM_STATE_MODAL])              r.modal=1;
        else if(s[i] == a[NET_WM_STATE_STICKY])             r.sticky=1;
        else if(s[i] == a[NET_WM_STATE_MAXIMIZED_VERT])     r.vmax=1;
        else if(s[i] == a[NET_WM_STATE_MAXIMIZED_HORZ])     r.hmax=1;
        else if(s[i] == a[GWM_WM_STATE_MAXIMIZED_TOP])      r.tmax=1;
        else if(s[i] == a[GWM_WM_STATE_MAXIMIZED_BOTTOM])   r.bmax=1;
        else if(s[i] == a[GWM_WM_STATE_MAXIMIZED_LEFT])     r.lmax=1;
        else if(s[i] == a[GWM_WM_STATE_MAXIMIZED_RIGHT])    r.rmax=1;
        else if(s[i] == a[NET_WM_STATE_SHADED])             r.shaded=1;
        else if(s[i] == a[NET_WM_STATE_SKIP_TASKBAR])       r.skip_taskbar=1;
        else if(s[i] == a[NET_WM_STATE_SKIP_PAGER])         r.skip_pager=1;
        else if(s[i] == a[NET_WM_STATE_HIDDEN])             r.hidden=1;
        else if(s[i] == a[NET_WM_STATE_FULLSCREEN])         r.fullscreen=1;
        else if(s[i] == a[NET_WM_STATE_ABOVE])              r.above=1;
        else if(s[i] == a[NET_WM_STATE_BELOW])              r.below=1;
        else if(s[i] == a[NET_WM_STATE_DEMANDS_ATTENTION])  r.attent=1;
        else if(s[i] == a[NET_WM_STATE_FOCUSED])            r.focused=1;
    }
    XFree(s);

    return r;
}

void update_net_wm_state(Window win, Net_wm_state state)
{
    // 目前EWMH規範中NET_WM_STATE共有13種狀態，GWM自定義4種狀態
    Atom *a=ewmh_atoms, prop=a[NET_WM_STATE], states[17]={0};
    int n=0;

    if(state.modal)          states[n++]=a[NET_WM_STATE_MODAL];
    if(state.sticky)         states[n++]=a[NET_WM_STATE_STICKY];
    if(state.vmax)           states[n++]=a[NET_WM_STATE_MAXIMIZED_VERT];
    if(state.hmax)           states[n++]=a[NET_WM_STATE_MAXIMIZED_HORZ];
    if(state.tmax)           states[n++]=a[GWM_WM_STATE_MAXIMIZED_TOP];
    if(state.bmax)           states[n++]=a[GWM_WM_STATE_MAXIMIZED_BOTTOM];
    if(state.lmax)           states[n++]=a[GWM_WM_STATE_MAXIMIZED_LEFT];
    if(state.rmax)           states[n++]=a[GWM_WM_STATE_MAXIMIZED_RIGHT];
    if(state.shaded)         states[n++]=a[NET_WM_STATE_SHADED];
    if(state.skip_taskbar)   states[n++]=a[NET_WM_STATE_SKIP_TASKBAR];
    if(state.skip_pager)     states[n++]=a[NET_WM_STATE_SKIP_PAGER];
    if(state.hidden)         states[n++]=a[NET_WM_STATE_HIDDEN];
    if(state.fullscreen)     states[n++]=a[NET_WM_STATE_FULLSCREEN];
    if(state.above)          states[n++]=a[NET_WM_STATE_ABOVE];
    if(state.below)          states[n++]=a[NET_WM_STATE_BELOW];
    if(state.attent)         states[n++]=a[NET_WM_STATE_DEMANDS_ATTENTION];
    if(state.focused)        states[n++]=a[NET_WM_STATE_FOCUSED];
    replace_atom_prop(win, prop, states, n);
}

void update_net_wm_state_for_no_max(Window win, Net_wm_state state)
{
    state.vmax=state.hmax=state.tmax=state.bmax=state.lmax=state.rmax=0;
    update_net_wm_state(win, state);
}

Net_wm_state get_net_wm_state_mask(const long *full_act)
{
    Net_wm_state m={0};
    Atom *a=ewmh_atoms;

    for(long p=0, i=1; i<=2 && (p=full_act[i]); i++)
    {
        if     (p == (long)a[NET_WM_STATE_MODAL])             m.modal=1;
        else if(p == (long)a[NET_WM_STATE_STICKY])            m.sticky=1;
        else if(p == (long)a[NET_WM_STATE_MAXIMIZED_VERT])    m.vmax=1;
        else if(p == (long)a[NET_WM_STATE_MAXIMIZED_HORZ])    m.hmax=1;
        else if(p == (long)a[GWM_WM_STATE_MAXIMIZED_TOP])     m.tmax=1;
        else if(p == (long)a[GWM_WM_STATE_MAXIMIZED_BOTTOM])  m.bmax=1;
        else if(p == (long)a[GWM_WM_STATE_MAXIMIZED_LEFT])    m.lmax=1;
        else if(p == (long)a[GWM_WM_STATE_MAXIMIZED_RIGHT])   m.rmax=1;
        else if(p == (long)a[NET_WM_STATE_SHADED])            m.shaded=1;
        else if(p == (long)a[NET_WM_STATE_SKIP_TASKBAR])      m.skip_taskbar=1;
        else if(p == (long)a[NET_WM_STATE_SKIP_PAGER])        m.skip_pager=1;
        else if(p == (long)a[NET_WM_STATE_HIDDEN])            m.hidden=1;
        else if(p == (long)a[NET_WM_STATE_FULLSCREEN])        m.fullscreen=1;
        else if(p == (long)a[NET_WM_STATE_ABOVE])             m.above=1;
        else if(p == (long)a[NET_WM_STATE_BELOW])             m.below=1;
        else if(p == (long)a[NET_WM_STATE_DEMANDS_ATTENTION]) m.attent=1;
        else if(p == (long)a[NET_WM_STATE_FOCUSED])           m.focused=1;
    }

    return m;
}

bool is_win_state_max(Net_wm_state state)
{
    return state.vmax || state.hmax || state.tmax
        || state.bmax || state.lmax || state.rmax;
}

/* 判斷是否存在（遵從EWMH標準的）合成器 */
bool have_compositor(void)
{
    return get_compositor() != None;
}

/* 獲取（遵從EWMH標準的）合成器的ID，它未必是真實的窗口 */
Window get_compositor(void)
{
    char prop_name[32];

    // 遵守EWMH標準的合成器都會獲取名爲_NET_WM_CM_Sn的選擇區所有權
    snprintf(prop_name, 32, "_NET_WM_CM_S%d", xinfo.screen);
    Atom prop_atom=XInternAtom(xinfo.display, prop_name, False);
    return XGetSelectionOwner(xinfo.display, prop_atom);
}

char *get_net_wm_name(Window win)
{
    return get_text_prop(win, ewmh_atoms[NET_WM_NAME]);
}

char *get_net_wm_icon_name(Window win)
{
    return get_text_prop(win, ewmh_atoms[NET_WM_ICON_NAME]);
}

CARD32 *get_net_wm_icon(Window win)
{
    return (CARD32 *)get_prop(win, ewmh_atoms[NET_WM_ICON], NULL);
}
