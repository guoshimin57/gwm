/* *************************************************************************
 *     prop.h：與prop.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef PROP_H
#define PROP_H

#include <stdbool.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>

typedef enum // 與gwm自定義標識符名稱表(gwm_atom_names)相應的ID
{
    GWM_LAYOUT, GWM_UPDATE_LAYOUT, GWM_WIDGET_TYPE,
    GWM_MAIN_COLOR_NAME, GWM_ATOM_N
} GWM_atom_id;

bool is_spec_gwm_atom(Atom spec, GWM_atom_id id);
void set_gwm_atoms(void);
void set_utf8_string_atom(void);
Atom get_utf8_string_atom(void);
void set_motif_wm_hints_atom(void);
bool has_motif_decoration(Window win);
Window get_transient_for(Window win);
char *get_text_prop(Window win, Atom atom);
long get_cardinal_prop(Window win, Atom prop, long fallback);
long *get_cardinals_prop(Window win, Atom prop, unsigned long *n);
Atom get_atom_prop(Window win, Atom prop);
Atom *get_atoms_prop(Window win, Atom prop, unsigned long *n);
Window get_window_prop(Window win, Atom prop);
Window *get_windows_prop(Window win, Atom prop, unsigned long *n);
Pixmap get_pixmap_prop(Window win, Atom prop);
char *get_utf8_string_prop(Window win, Atom prop);
char *get_utf8_strings_prop(Window win, Atom prop, unsigned long *n);
void replace_atom_prop(Window win, Atom prop, Atom value);
void replace_atoms_prop(Window win, Atom prop, const Atom values[], int n);
void replace_window_prop(Window win, Atom prop, Window value);
void replace_windows_prop(Window win, Atom prop, const Window wins[], int n);
void replace_cardinal_prop(Window win, Atom prop, long value);
void replace_cardinals_prop(Window win, Atom prop, const long values[], int n);
void replace_utf8_prop(Window win, Atom prop, const void *str);
void replace_utf8s_prop(Window win, Atom prop, const void *strs, int n);
void copy_prop(Window dest, Window src);
void set_gwm_layout(int layout);
int get_gwm_layout(void);
void request_layout_update(void);
void set_main_color_name(const char *name);
char *get_main_color_name(void);

#endif
