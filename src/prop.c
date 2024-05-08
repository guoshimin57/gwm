/* *************************************************************************
 *     prop.c：實現與X特性相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static const char *gwm_atom_names[GWM_ATOM_N]= // gwm自定義的標識符名稱
{
    "GWM_CURRENT_LAYOUT", "GWM_UPDATE_LAYOUT", "GWM_WIDGET_TYPE", "GWM_DESKTOP_MASK",
    "GWM_MAIN_COLOR_NAME"
};

static Atom gwm_atoms[GWM_ATOM_N];

bool is_spec_gwm_atom(Atom spec, GWM_atom_id id)
{
    return spec==gwm_atoms[id];
}

void set_gwm_atoms(void)
{
    for(int i=0; i<GWM_ATOM_N; i++)
        gwm_atoms[i]=XInternAtom(xinfo.display, gwm_atom_names[i], False);
}

Window get_transient_for(Window win)
{
    Window owner;
    return XGetTransientForHint(xinfo.display, win, &owner) ? owner : None;
}

unsigned char *get_prop(Window win, Atom prop, unsigned long *n)
{
    int fmt;
    unsigned long nitems=0, *m=(n ? n : &nitems), rest;
    unsigned char *p=NULL;
    Atom type;

    /* 对于XGetWindowProperty，把要接收的数据长度（第5个参数）设置得比实际长度
     * 長可简化代码，这样就不必考虑要接收的數據是否不足32位。以下同理。 */

    if( XGetWindowProperty(xinfo.display, win, prop, 0, ~0L, False,
        AnyPropertyType, &type, &fmt, m, &rest, &p) != Success
        || !type || !fmt || !*m || !p) // 可能設置了特性，但設置得不正確
        return NULL;

    return p;
}

char *get_text_prop(Window win, Atom atom)
{
    int n;
    char **list=NULL, *result=NULL;
    XTextProperty name;

    if(!XGetTextProperty(xinfo.display, win, &name, atom))
        return NULL;

    if(name.encoding == XA_STRING)
        result=copy_string((char *)name.value);
    else if(Xutf8TextPropertyToTextList(xinfo.display, &name, &list, &n) == Success
        && n && *list) // 與手冊不太一致的是，返回Success並不一定真的成功，狗血！
        result=copy_string(*list), XFreeStringList(list);
    XFree(name.value); // 手冊沒提及要釋放name或name.value，但很多WM都會釋放後者
    return result;
}

bool get_cardinal_prop(Window win, Atom prop, CARD32 *result)
{
    CARD32 *p=(CARD32 *)get_prop(win, prop, NULL);
    if(!p)
        return false;
    *result=*p;
    XFree(p);
    return true;
}

bool get_atom_prop(Window win, Atom prop, Atom *result)
{
    Atom *p=(Atom *)get_prop(win, prop, NULL);
    if(!p)
        return false;
    *result=*p;
    XFree(p);
    return true;
}

void replace_atom_prop(Window win, Atom prop, const Atom *values, int n)
{
    XChangeProperty(xinfo.display, win, prop, XA_ATOM, 32, PropModeReplace,
        (unsigned char *)values, n);
}

void replace_window_prop(Window win, Atom prop, const Window *wins, int n)
{
    XChangeProperty(xinfo.display, win, prop, XA_WINDOW, 32, PropModeReplace,
        (unsigned char *)wins, n);
}

/* 根據XChangeProperty顯示，當format爲32時，data必須是long類型的數組 */
void replace_cardinal_prop(Window win, Atom prop, const long *values, int n)
{
    XChangeProperty(xinfo.display, win, prop, XA_CARDINAL, 32, PropModeReplace,
        (unsigned char *)values, n);
}

void copy_prop(Window dest, Window src)
{
    int i=0, n=0, fmt=0;
    Atom type, *props=XListProperties(xinfo.display, src, &n);

    if(!props)
        return;

    unsigned long nitems, rest;
    for(unsigned char *data=NULL; i<n; i++)
    {
        if( XGetWindowProperty(xinfo.display, src, props[i], 0, ~0L, False,
            AnyPropertyType, &type, &fmt, &nitems, &rest, &data)==Success
            && data && nitems)
        {
            XChangeProperty(xinfo.display, dest, props[i], type, fmt, PropModeReplace,
                data, nitems);
            XFree(data);
        }
    }
    XFree(props);
}

void set_gwm_current_layout(long cur_layout)
{
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_CURRENT_LAYOUT],
        &cur_layout, 1);
}

bool get_gwm_current_layout(int *cur_layout)
{
    CARD32 cur;
    bool flag=get_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_CURRENT_LAYOUT], &cur);
    *cur_layout=cur;
    return flag;
}

void set_gwm_desktop_mask(Window win, long mask)
{
    replace_cardinal_prop(win, gwm_atoms[GWM_DESKTOP_MASK], &mask, 1);
}

bool get_gwm_desktop_mask(Window win, CARD32 *mask)
{
    return get_cardinal_prop(win, gwm_atoms[GWM_DESKTOP_MASK], mask);
}

void request_layout_update(void)
{
    long f=true;
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_UPDATE_LAYOUT], &f, 1);
}

char *get_main_color_name(void)
{
    return get_text_prop(xinfo.root_win, gwm_atoms[GWM_MAIN_COLOR_NAME]);
}
