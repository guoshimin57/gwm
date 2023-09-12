/* *************************************************************************
 *     drawable.h：與drawable.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef DRAWABLE_H
#define DRAWABLE_H

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

Window get_transient_for(WM *wm, Window w);
Atom *get_atom_props(WM *wm, Window win, Atom prop, unsigned long *n);
Atom get_atom_prop(WM *wm, Window win, Atom prop);
unsigned char *get_prop(WM *wm, Window win, Atom prop, unsigned long *n);
char *get_text_prop(WM *wm, Window win, Atom atom);
char *get_title_text(WM *wm, Window win, const char *fallback);
char *get_icon_title_text(WM *wm, Window win, const char *fallback);
void copy_prop(WM *wm, Window dest, Window src);
bool send_event(WM *wm, Atom protocol, Window win);
bool is_pointer_on_win(WM *wm, Window win);
bool is_on_screen(WM *wm, int x, int y, int w, int h);
void print_area(WM *wm, Drawable d, int x, int y, int w, int h);
bool is_wm_win(WM *wm, Window win, bool before_wm);
void update_win_bg(WM *wm, Window win, unsigned long color, Pixmap pixmap);
void set_override_redirect(WM *wm, Window win);
bool get_geometry(WM *wm, Drawable drw, int *x, int *y, int *w, int *h, int *bw, unsigned int *depth);
void set_pos_for_click(WM *wm, Window click, int cx, int *px, int *py, int pw, int ph);
bool is_win_exist(WM *wm, Window win, Window parent);
Pixmap create_pixmap_from_file(WM *wm, Window win, const char *filename);
void close_win(WM *wm, Window win);
Window create_widget_win(WM *wm, Window parent, int x, int y, int w, int h, int border_w, unsigned long border_pixel, unsigned long bg_pixel);
void set_visual_for_imlib(WM *wm, Drawable d);
void restack_win(WM *wm, Window win);
Net_wm_win_type get_net_wm_win_type(WM *wm, Window win);
Net_wm_state get_net_wm_state(WM *wm, Window win);

#endif
