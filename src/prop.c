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
    "GWM_CURRENT_LAYOUT", "GWM_UPDATE_LAYOUT", "GWM_WIDGET_TYPE",
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
    unsigned long i, nitems=0, *m=(n ? n : &nitems), rest;
    unsigned char *p=NULL, *result=NULL;;
    Atom type;

    /* 对于XGetWindowProperty，把要接收的数据长度（第5个参数）设置得比实际长度
     * 長可简化代码，这样就不必考虑要接收的數據是否不足32位。以下同理。 */
    if( XGetWindowProperty(xinfo.display, win, prop, 0, ~0L, False,
        AnyPropertyType, &type, &fmt, m, &rest, &p) != Success
        || !type || !fmt || !*m || !p)
        return NULL;

    result=malloc_s(*m*fmt/8+1);
    /* 當fmt等於16時，p以short int類型存儲特性，當short int大於16位時，
     * 高位是沒用的填充數據；當fmt等於32時，p以long類型存儲特性，當long
     * 大於32位時，高位是沒用的填充數據。因此，要考慮跳過填充數據。 */
    for(i=0; i<*m; i++)
    {
        switch(fmt)
        {
            case  8: result[i]=p[i]; break;
            case 16: ((CARD16 *)result)[i]=((short int *)p)[i]; break;
            case 32: ((CARD32 *)result)[i]=((long *)p)[i]; break;
            default: free(result); return NULL;
        }
    }
    result[*m*fmt/8]='\0';
    XFree(p);

    return result;
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

void set_gwm_widget_type(Window win, long type)
{
    replace_cardinal_prop(win, gwm_atoms[GWM_WIDGET_TYPE], &type, 1);
}

bool get_gwm_widget_type(Window win, CARD32 *type)
{
    return get_cardinal_prop(win, gwm_atoms[GWM_WIDGET_TYPE], type);
}

void request_layout_update(void)
{
    long f=true;
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_UPDATE_LAYOUT], &f, 1);
}
