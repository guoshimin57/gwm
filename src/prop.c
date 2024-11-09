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
#include "prop.h"

static const char *gwm_atom_names[GWM_ATOM_N]= // gwm自定義的標識符名稱
{
    "GWM_CURRENT_LAYOUT", "GWM_UPDATE_LAYOUT", "GWM_WIDGET_TYPE",
    "GWM_MAIN_COLOR_NAME"
};

static Atom gwm_atoms[GWM_ATOM_N];
static Atom utf8_string_atom;

bool is_spec_gwm_atom(Atom spec, GWM_atom_id id)
{
    return spec==gwm_atoms[id];
}

void set_gwm_atoms(void)
{
    for(int i=0; i<GWM_ATOM_N; i++)
        gwm_atoms[i]=XInternAtom(xinfo.display, gwm_atom_names[i], False);
}

void set_utf8_string_atom(void)
{
    utf8_string_atom=XInternAtom(xinfo.display, "UTF8_STRING", False);
}

Atom get_utf8_string_atom(void)
{
    return utf8_string_atom;
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

long get_cardinal_prop(Window win, Atom prop, long fallback)
{
    long result, *p=(long *)get_prop(win, prop, NULL);

    result = p ? *p : fallback;
    XFree(p);

    return result;
}

Atom get_atom_prop(Window win, Atom prop)
{
    Atom *p=(Atom *)get_prop(win, prop, NULL);
    return p ? *p : 0;
}

/* 根據XChangeProperty手冊顯示，當format爲32時，data必須是long類型的數組。
 * 另外，手冊沒有解析當n爲0時會發生什麼，實測此時並非什麼也不幹，而是會有
 * 些不可預知的行爲。後同。 */
void replace_atom_prop(Window win, Atom prop, const Atom values[], int n)
{
    if(n <= 0)
        return;

    long v[n];
    for(int i=0; i<n; i++)
        v[i]=values[i];

    XChangeProperty(xinfo.display, win, prop, XA_ATOM, 32, PropModeReplace,
        (unsigned char *)v, n);
}

void replace_window_prop(Window win, Atom prop, const Window wins[], int n)
{
    if(n <= 0)
        return;

    long v[n];
    for(int i=0; i<n; i++)
        v[i]=wins[i];

    XChangeProperty(xinfo.display, win, prop, XA_WINDOW, 32, PropModeReplace,
        (unsigned char *)v, n);
}

void replace_cardinal_prop(Window win, Atom prop, const long values[], int n)
{
    if(n <= 0)
        return;

    XChangeProperty(xinfo.display, win, prop, XA_CARDINAL, 32, PropModeReplace,
        (unsigned char *)values, n);
}

/* UTF8_STRING需要特殊處理，因其是否屬於ICCCM尚存疑，詳見：
 *     https://gitlab.freedesktop.org/xorg/doc/xorg-docs/-/issues/5 */
void replace_utf8_prop(Window win, Atom prop, const void *strs, int n)
{
    int size=0; // 字符數組strs的總尺寸，包含每個字符串的終止符
    const char *s=strs;

    for(int i=0; i<n; i++)
    {
        size += strlen(s)+1;
        s += size;
    }

    XChangeProperty(xinfo.display, win, prop, utf8_string_atom, 8,
        PropModeReplace, (unsigned char *)strs, size);
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
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_CURRENT_LAYOUT], &cur_layout, 1);
}

int get_gwm_current_layout(void)
{
    return get_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_CURRENT_LAYOUT], 0);
}

void request_layout_update(void)
{
    long f=true;
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_UPDATE_LAYOUT], &f, 1);
}

void set_main_color_name(const char *name)
{
    replace_utf8_prop(xinfo.root_win, gwm_atoms[GWM_MAIN_COLOR_NAME], name, 1);
}

char *get_main_color_name(void)
{
    return get_text_prop(xinfo.root_win, gwm_atoms[GWM_MAIN_COLOR_NAME]);
}
