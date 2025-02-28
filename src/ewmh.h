/* *************************************************************************
 *     ewmh.h：與ewmh.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef HINT_H
#define HINT_H

#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

typedef enum // 與EWMH規範標識符名稱表(ewmh_atom_names)相應的ID
{
    NET_SUPPORTED, NET_CLIENT_LIST, NET_CLIENT_LIST_STACKING,
    NET_NUMBER_OF_DESKTOPS, NET_DESKTOP_GEOMETRY,
    NET_DESKTOP_VIEWPORT, NET_CURRENT_DESKTOP, NET_DESKTOP_NAMES,
    NET_ACTIVE_WINDOW, NET_WORKAREA, NET_SUPPORTING_WM_CHECK,
    NET_VIRTUAL_ROOTS, NET_DESKTOP_LAYEROUT, NET_SHOWING_DESKTOP,
    NET_CLOSE_WINDOW, NET_MOVERESIZE_WINDOW, NET_WM_MOVERESIZE,
    NET_RESTACK_WINDOW, NET_REQUEST_FRAME_EXTENTS, NET_WM_NAME,
    NET_WM_VISIBLE_NAME, NET_WM_ICON_NAME, NET_WM_VISIBLE_ICON_NAME,
    NET_WM_DESKTOP, NET_WM_WINDOW_TYPE, NET_WM_WINDOW_TYPE_DESKTOP,
    NET_WM_WINDOW_TYPE_DOCK, NET_WM_WINDOW_TYPE_TOOLBAR,
    NET_WM_WINDOW_TYPE_MENU, NET_WM_WINDOW_TYPE_UTILITY,
    NET_WM_WINDOW_TYPE_SPLASH, NET_WM_WINDOW_TYPE_DIALOG,
    NET_WM_WINDOW_TYPE_DROPDOWN_MENU, NET_WM_WINDOW_TYPE_POPUP_MENU,
    NET_WM_WINDOW_TYPE_TOOLTIP, NET_WM_WINDOW_TYPE_NOTIFICATION,
    NET_WM_WINDOW_TYPE_COMBO, NET_WM_WINDOW_TYPE_DND,
    NET_WM_WINDOW_TYPE_NORMAL, NET_WM_STATE, NET_WM_STATE_MODAL,
    NET_WM_STATE_STICKY, NET_WM_STATE_MAXIMIZED_VERT,
    NET_WM_STATE_MAXIMIZED_HORZ, NET_WM_STATE_SHADED,
    NET_WM_STATE_SKIP_TASKBAR, NET_WM_STATE_SKIP_PAGER,
    NET_WM_STATE_HIDDEN, NET_WM_STATE_FULLSCREEN,
    NET_WM_STATE_ABOVE, NET_WM_STATE_BELOW,
    NET_WM_STATE_DEMANDS_ATTENTION, NET_WM_STATE_FOCUSED,
    NET_WM_ALLOWED_ACTIONS, NET_WM_ACTION_MOVE,
    NET_WM_ACTION_RESIZE, NET_WM_ACTION_MINIMIZE,
    NET_WM_ACTION_SHADE, NET_WM_ACTION_STICK,
    NET_WM_ACTION_MAXIMIZE_HORZ, NET_WM_ACTION_MAXIMIZE_VERT,
    NET_WM_ACTION_FULLSCREEN, NET_WM_ACTION_CHANGE_DESKTOP,
    NET_WM_ACTION_CLOSE, NET_WM_ACTION_ABOVE, NET_WM_ACTION_BELOW,
    NET_WM_STRUT, NET_WM_STRUT_PARTIAL, NET_WM_ICON_GEOMETRY,
    NET_WM_ICON, NET_WM_PID, NET_WM_HANDLED_ICONS,
    NET_WM_USER_TIME, NET_WM_USER_TIME_WINDOW, NET_FRAME_EXTENTS,
    NET_WM_OPAQUE_REGION, NET_WM_BYPASS_COMPOSITOR, NET_WM_PING,
    NET_WM_SYNC_REQUEST, NET_WM_FULLSCREEN_MONITORS,
    NET_WM_FULL_PLACEMENT,
    GWM_WM_STATE_MAXIMIZED_TOP, GWM_WM_STATE_MAXIMIZED_BOTTOM,
    GWM_WM_STATE_MAXIMIZED_LEFT, GWM_WM_STATE_MAXIMIZED_RIGHT,
    EWMH_ATOM_N
} EWMH_atom_id;

typedef struct // 與_NET_WM_WINDOW_TYPE列表相應的類型標志
{
    unsigned int desktop : 1;
    unsigned int dock : 1;
    unsigned int toolbar : 1;
    unsigned int menu : 1;
    unsigned int utility : 1;
    unsigned int splash : 1;
    unsigned int dialog : 1;
    unsigned int dropdown_menu : 1;
    unsigned int popup_menu : 1;
    unsigned int tooltip : 1;
    unsigned int notification : 1;
    unsigned int combo : 1;
    unsigned int dnd : 1;
    unsigned int normal : 1;
    unsigned int none : 1;
} Net_wm_win_type;

typedef struct // 與_NET_WM_STATE列表相應的狀態標志
{
    unsigned int modal : 1;
    unsigned int sticky : 1;
    unsigned int vmax : 1;
    unsigned int hmax : 1;
    unsigned int shaded : 1;
    unsigned int skip_taskbar : 1;
    unsigned int skip_pager : 1;
    unsigned int hidden : 1;
    unsigned int fullscreen : 1;
    unsigned int above : 1;
    unsigned int below : 1;
    unsigned int attent : 1;
    unsigned int focused : 1;
    unsigned int tmax : 1;
    unsigned int bmax : 1;
    unsigned int lmax : 1;
    unsigned int rmax : 1;
} Net_wm_state;

bool is_spec_ewmh_atom(Atom spec, EWMH_atom_id id);
void set_ewmh_atoms(void);
void set_net_supported(void);
void set_net_client_list(const Window *wins, int n);
void set_net_client_list_stacking(const Window *wins, int n);
long *get_net_client_list(unsigned long *n);
long *get_net_client_list_stacking(unsigned long *n);
void set_net_number_of_desktops(int n);
int get_net_number_of_desktops(void);
void set_net_desktop_geometry(int w, int h);
void set_net_desktop_viewport(int x, int y);
void set_net_current_desktop(unsigned int cur_desktop);
unsigned int get_net_current_desktop(void);
unsigned int get_net_wm_desktop(Window win);
void set_net_desktop_names(const char **names, int n);
void set_net_active_window(Window act_win);
void set_net_workarea(int x, int y, int w, int h, int ndesktop);
void get_net_workarea(int *x, int *y, int *w, int *h);
void set_net_supporting_wm_check(Window check_win, const char *wm_name);
void set_net_showing_desktop(bool show);
void set_net_wm_allowed_actions(Window win);
Net_wm_win_type get_net_wm_win_type(Window win);
Net_wm_state get_net_wm_state(Window win);
void update_net_wm_state(Window win, Net_wm_state state);
void update_net_wm_state_for_no_max(Window win, Net_wm_state state);
Net_wm_state get_net_wm_state_mask(const long *full_act);
bool is_win_state_max(Net_wm_state state);
bool have_compositor(void);
Window get_compositor(void);
char *get_net_wm_name(Window win);
char *get_net_wm_icon_name(Window win);
long *get_net_wm_icon(Window win);

#endif
