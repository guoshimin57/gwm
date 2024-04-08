/* *************************************************************************
 *     prop.h：與prop.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef PROP_H
#define PROP_H

typedef enum // 與gwm自定義標識符名稱表(gwm_atom_names)相應的ID
{
    GWM_CURRENT_LAYOUT, GWM_UPDATE_LAYOUT, GWM_WIDGET_TYPE, GWM_DESKTOP_MASK, GWM_ATOM_N
} GWM_atom_id;

bool is_spec_gwm_atom(Atom spec, GWM_atom_id id);
void set_gwm_atoms(void);
Window get_transient_for(Window win);
unsigned char *get_prop(Window win, Atom prop, unsigned long *n);
char *get_text_prop(Window win, Atom atom);
bool get_cardinal_prop(Window win, Atom prop, CARD32 *result);
bool get_atom_prop(Window win, Atom prop, Atom *result);
void replace_atom_prop(Window win, Atom prop, const Atom *values, int n);
void replace_window_prop(Window win, Atom prop, const Window *wins, int n);
void replace_cardinal_prop(Window win, Atom prop, const long *values, int n);
void copy_prop(Window dest, Window src);
void set_gwm_current_layout(long cur_layout);
bool get_gwm_current_layout(int *cur_layout);
void set_gwm_desktop_mask(Window win, long mask);
bool get_gwm_desktop_mask(Window win, CARD32 *mask);
void request_layout_update(void);

#endif
