/* *************************************************************************
 *     prop.c：實現與X特性相關的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "gwm.h"
#include "misc.h"
#include "prop.h"

typedef struct
{
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
} MotifWmHints;

#define MWM_HINTS_DECORATIONS (1L << 1)

static const char *gwm_atom_names[GWM_ATOM_N]= // gwm自定義的標識符名稱
{
    "GWM_LAYOUT", "GWM_UPDATE_LAYOUT", "GWM_WIDGET_TYPE",
    "GWM_MAIN_COLOR_NAME"
};

static long get_long_prop(Window win, Atom prop, long fallback);
static unsigned char *get_prop(Window win, Atom prop, size_t size, unsigned long *n);
static size_t get_fmt_size(int fmt);
static void convert_type(unsigned char *dst, size_t dsize, const unsigned char *src, size_t ssize);
static bool is_little_endian_order(void);
static void change_prop(Window win, Atom prop, Atom type, int fmt, int mode, const unsigned char *data, size_t size, int n);
static unsigned long get_strings_size(const char **strs, int n);

static Atom gwm_atoms[GWM_ATOM_N];
static Atom utf8_string_atom;
static Atom motif_wm_hints_atom;

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

void set_motif_wm_hints_atom(void)
{
    motif_wm_hints_atom=XInternAtom(xinfo.display, "_MOTIF_WM_HINTS", False);
}

void set_motif_wm_hints(Window win, const MotifWmHints *hints)
{
    change_prop(win, motif_wm_hints_atom, motif_wm_hints_atom, 32,
        PropModeReplace, (const unsigned char *)hints, sizeof(long),
        CEIL_DIV(sizeof(MotifWmHints), sizeof(long)));
}

MotifWmHints *get_motif_wm_hints(Window win)
{
    return (MotifWmHints *)get_prop(win, motif_wm_hints_atom, sizeof(long), NULL);
}

bool has_motif_decoration(Window win)
{
    MotifWmHints *hints=get_motif_wm_hints(win);
    bool result=(!hints
        || !(hints->flags & MWM_HINTS_DECORATIONS)
        || hints->decorations);
    XFree(hints);

    return result;
}

Window get_transient_for(Window win)
{
    Window owner;
    return XGetTransientForHint(xinfo.display, win, &owner) ? owner : None;
}

// 返回prop特性的值，它是一個數組，其中每個元素的大小爲size，數量爲n個 
static unsigned char *get_prop(Window win, Atom prop, size_t size, unsigned long *n)
{
    assert(size==1 || size==2 || size==4 || size==8);

    int fmt;
    unsigned long rest=0, nitems=0;
    unsigned char *p=NULL;
    Atom type;
    unsigned char *values=NULL;

    /* 对于XGetWindowProperty，把要接收的数据长度（第5个参数）设置得比实际长度
     * 長可简化代码，这样就不必考虑要接收的數據是否不足32位。以下同理。 */
    if( XGetWindowProperty(xinfo.display, win, prop, 0, ~0L, False,
        AnyPropertyType, &type, &fmt, &nitems, &rest, &p) == Success
        && type && fmt && nitems && p) // 可能設置了特性，但設置得不正確
    {
        if(n)
            *n=nitems;

        size_t ssize=get_fmt_size(fmt);
        if(ssize > 0)
        {
            values=Malloc(nitems*size+1);
            memset(values, 0, nitems*size+1);
            for(unsigned long i=0; i<nitems; i++)
                convert_type(values+size*i, size, p+ssize*i, ssize);
        }

        XFree(p);
    }

    return values;
}

static size_t get_fmt_size(int fmt)
{
    switch(fmt)
    {
        case 8:  return sizeof(char);
        case 16: return sizeof(short int);
        case 32: return sizeof(long);
        default: return 0;
    }
}

// 把實際類型大小爲ssize的src轉換爲類型大小爲dsize的類型並保存在dst
static void convert_type(unsigned char *dst, size_t dsize, const unsigned char *src, size_t ssize)
{
    size_t min=MIN(dsize, ssize);

    if(is_little_endian_order())
        for(size_t i=0; i<min; i++)
            dst[i]=src[i];
    else
        for(size_t i=0; i<min; i++)
            dst[dsize-i-1]=src[i];
}

static bool is_little_endian_order(void)
{
    union { uint16_t s; uint8_t c[2]; } u = { .s=0x0102 };
    return u.c[0]==2 && u.c[1]==1;
}

// 修改prop特性的值，data是一個元素類型大小爲size的數組，n個元素
static void change_prop(Window win, Atom prop, Atom type, int fmt, int mode, const unsigned char *data, size_t size, int n)
{
    if(data==NULL || n<=0 || size==0)
        return;

    size_t dsize=get_fmt_size(fmt);
    if(dsize == 0)
        return;

    unsigned char *values=Malloc(n*dsize);
    memset(values, 0, n*dsize);
    for(int i=0; i<n; i++)
        convert_type(values+dsize*i, dsize, data+size*i, size);
    XChangeProperty(xinfo.display, win, prop, type, fmt, mode, values, n);
    XFree(values);
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

static long get_long_prop(Window win, Atom prop, long fallback)
{
    long result, *p=NULL;

    p=(long *)get_prop(win, prop, sizeof(long), NULL);
    result = p ? *p : fallback;
    XFree(p);

    return result;
}

long get_cardinal_prop(Window win, Atom prop, long fallback)
{
    return get_long_prop(win, prop, fallback);
}

long *get_cardinals_prop(Window win, Atom prop, unsigned long *n)
{
    return (long *)get_prop(win, prop, sizeof(long), n);
}

Atom get_atom_prop(Window win, Atom prop)
{
    return get_long_prop(win, prop, None);
}

Atom *get_atoms_prop(Window win, Atom prop, unsigned long *n)
{
    return (Atom *)get_prop(win, prop, sizeof(Atom), n);
}

Window get_window_prop(Window win, Atom prop)
{
    return get_long_prop(win, prop, None);
}

Window *get_windows_prop(Window win, Atom prop, unsigned long *n)
{
    return (Window *)get_prop(win, prop, sizeof(Window), n);
}

Pixmap get_pixmap_prop(Window win, Atom prop)
{
    return get_long_prop(win, prop, None);
}

char *get_utf8_string_prop(Window win, Atom prop)
{
    return (char *)get_prop(win, prop, 1, NULL);
}

char *get_utf8_strings_prop(Window win, Atom prop, unsigned long *n)
{
    return (char *)get_prop(win, prop, 1, n);
}

void replace_atom_prop(Window win, Atom prop, Atom value)
{
    change_prop(win, prop, XA_ATOM, 32, PropModeReplace,
        (const unsigned char *)&value, sizeof(Atom), 1);
}

void replace_atoms_prop(Window win, Atom prop, const Atom values[], int n)
{
    change_prop(win, prop, XA_ATOM, 32, PropModeReplace,
        (const unsigned char *)values, sizeof(Atom), n);
}

void replace_window_prop(Window win, Atom prop, Window value)
{
    change_prop(win, prop, XA_WINDOW, 32, PropModeReplace,
        (const unsigned char *)&value, sizeof(Window), 1);
}

void replace_windows_prop(Window win, Atom prop, const Window wins[], int n)
{
    change_prop(win, prop, XA_WINDOW, 32, PropModeReplace,
        (const unsigned char *)wins, sizeof(Window), n);
}

void replace_cardinal_prop(Window win, Atom prop, long value)
{
    change_prop(win, prop, XA_CARDINAL, 32, PropModeReplace,
        (const unsigned char *)&value, sizeof(long), 1);
}

void replace_cardinals_prop(Window win, Atom prop, const long values[], int n)
{
    change_prop(win, prop, XA_CARDINAL, 32, PropModeReplace,
        (const unsigned char *)values, sizeof(long), n);
}

/* UTF8_STRING需要特殊處理，因其是否屬於ICCCM尚存疑，詳見：
 *     https://gitlab.freedesktop.org/xorg/doc/xorg-docs/-/issues/5 */
void replace_utf8_prop(Window win, Atom prop, const void *str)
{
    change_prop(win, prop, utf8_string_atom, 8, PropModeReplace,
        (const unsigned char *)str, 1, strlen(str)+1);
}

void replace_utf8s_prop(Window win, Atom prop, const void *strs, int n)
{
    unsigned long size=get_strings_size((const char **)strs, n);
    unsigned char *p=Malloc(size);
    size_t len=0, sum=0;
    for(int i=0; i<n; i++)
    {
        char *s=((char **)strs)[i];
        len=strlen(s);
        memcpy(p+sum, s, len+1);
        sum+=len+1;
    }
    change_prop(win, prop, utf8_string_atom, 8, PropModeReplace,
        (const unsigned char *)p, 1, size);
    free(p);
}

// 計算字符數組strs的總尺寸，包含每個字符串的終止符
static unsigned long get_strings_size(const char **strs, int n)
{
    int size=0;
    for(int i=0; i<n; i++)
        size += strlen(strs[i])+1;
    return size;
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
            XChangeProperty(xinfo.display, dest, props[i], type, fmt,
                PropModeReplace, data, nitems);
            XFree(data);
        }
    }
    XFree(props);
}

void set_gwm_layout(int layout)
{
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_LAYOUT], layout);
}

int get_gwm_layout(void)
{
    return get_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_LAYOUT], 0);
}

void request_layout_update(void)
{
    replace_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_UPDATE_LAYOUT], 1);
}

void set_main_color_name(const char *name)
{
    replace_utf8_prop(xinfo.root_win, gwm_atoms[GWM_MAIN_COLOR_NAME], name);
}

char *get_main_color_name(void)
{
    return get_text_prop(xinfo.root_win, gwm_atoms[GWM_MAIN_COLOR_NAME]);
}
