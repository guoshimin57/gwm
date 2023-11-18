/* *************************************************************************
 *     icccm.h：與icccm.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef ICCCM_H
#define ICCCM_H

typedef enum // 與ICCCM規範標識符名稱表(icccm_atiom_names)相應的ID
{
    WM_PROTOCOLS, WM_DELETE_WINDOW, WM_STATE, WM_CHANGE_STATE, WM_TAKE_FOCUS,
    UTF8_STRING, ICCCM_ATOMS_N
} ICCCM_atom_id;

bool is_spec_icccm_atom(Atom spec, ICCCM_atom_id id);
Atom get_utf8_string_atom(void);
void replace_utf8_prop(Window win, Atom prop, const void *strs, int n);
void set_icccm_atoms(void);
int get_win_col(int width, const XSizeHints *hint);
int get_win_row(int height, const XSizeHints *hint);
void update_size_hint(Window win, int fallback_inc, XSizeHints *size_hint);
void fix_win_size_by_hint(const XSizeHints *size_hint, int *w, int *h);
bool is_prefer_size(int w, int h, const XSizeHints *hint);
void set_input_focus(Window win, const XWMHints *hint);
bool has_focus_hint(const XWMHints *hint);
void set_urgency(Window win, XWMHints *h, bool urg);
bool is_iconic_state(Window win);
void close_win(Window win);
char *get_wm_name(Window win);
char *get_wm_icon_name(Window win);

#endif
