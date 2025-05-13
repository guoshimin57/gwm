/* *************************************************************************
 *     tprop.c：對prop模塊進行單元測試。
 *     版權 (C) 2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "../src/prop.c"
#include <assert.h>
#include <stdio.h>

static void test_get_transient_for(void);
static void test_get_text_prop(void);
static void test_get_fmt_size(void);
static void test_convert_type(void);
static void test_change_and_get_prop(void);
static void test_motif(void);
static void test_utf8_string(void);
static void test_cardinal_prop(void);
static void test_atom_prop(void);
static void test_window_prop(void);
static void test_gwm_prop(void);
static void test_copy_prop(void);

static Window win;
Atom prop=None;
Atom type=None;

int main(void)
{
    xinfo.display=XOpenDisplay(NULL);
    xinfo.screen=DefaultScreen(xinfo.display);
    xinfo.root_win=RootWindow(xinfo.display, xinfo.screen);
    win=XCreateSimpleWindow(xinfo.display, xinfo.root_win,
        -1, -1, 1, 1, 0, 0, 0);
    prop=XInternAtom(xinfo.display, "test_prop", False);
    type=XInternAtom(xinfo.display, "test_type", False);

    set_gwm_atoms();
    test_get_transient_for();
    test_get_text_prop();
    test_get_fmt_size();
    test_convert_type();
    test_change_and_get_prop();
    test_motif();
    test_utf8_string();
    test_cardinal_prop();
    test_atom_prop();
    test_window_prop();
    test_gwm_prop();
    test_copy_prop();
    XCloseDisplay(xinfo.display);

    return 0;
}

static void test_get_transient_for(void)
{
    assert(get_transient_for(win) == None);
    /* 未设计set_transient_for，故难以对get_text_prop进行更深入的测试 */
}

static void test_get_text_prop(void)
{
    assert(get_text_prop(win, prop) == NULL);
    /* 未设计set_text_prop，故难以对get_text_prop进行更深入的测试 */
}

static void test_get_fmt_size(void)
{
    assert(get_fmt_size(0) == 0);
    assert(get_fmt_size(-1) == 0);
    assert(get_fmt_size(1) == 0);
    assert(get_fmt_size(64) == 0);
    assert(get_fmt_size(8));
    assert(get_fmt_size(16));
    assert(get_fmt_size(32));
}

static void test_convert_type(void)
{
    uint64_t src=0x0102030405060708, dst=0;
    unsigned char *psrc=(unsigned char *)&src, *pdst=(unsigned char *)&dst;
    bool le=is_little_endian_order();

    convert_type(pdst, 1, psrc, 1);
    assert(le ? dst==0x08 : dst==0x01);

    dst=0;
    convert_type(pdst, 1, psrc, 2);
    assert(le ? dst==0x08 : dst==0x01);

    dst=0;
    convert_type(pdst, 2, psrc, 1);
    assert(le ? dst==0x08 : dst==0x01);

    dst=0;
    convert_type(pdst, 2, psrc, 2);
    assert(le ? dst==0x0708 : dst==0x0102);

    dst=0;
    convert_type(pdst, 2, psrc, 4);
    assert(le ? dst==0x0708 : dst==0x0102);
    
    dst=0;
    convert_type(pdst, 4, psrc, 2);
    assert(le ? dst==0x0708 : dst==0x0102);
    
    dst=0;
    convert_type(pdst, 4, psrc, 4);
    assert(le ? dst==0x05060708 : dst==0x01020304);
    
    dst=0;
    convert_type(pdst, 8, psrc, 4);
    assert(le ? dst==0x05060708 : dst==0x01020304);
} 

static void test_change_and_get_prop(void)
{
    char a1[]={1, 2, 3};
    unsigned long n=0, nitems=ARRAY_NUM(a1);
    change_prop(win, prop, type, 8, PropModeReplace,
        (unsigned char *)a1, 1, nitems);
    char *p1=(char *)get_prop(win, prop, 1, &n);
    assert(p1[0]==1 && p1[1]==2 && p1[2]==3 && n==nitems);
    uint16_t *p2=(uint16_t *)get_prop(win, prop, 2, &n);
    assert(p2[0]==1 && p2[1]==2 && p2[2]==3 && n==nitems);

    short int a2[]={0x1234, 0x5678};
    nitems=ARRAY_NUM(a2);
    change_prop(win, prop, type, 16, PropModeReplace,
        (unsigned char *)a2, sizeof(short int), nitems);
    int16_t *p3=(int16_t *)get_prop(win, prop, 2, &n);
    assert(p3[0]==0x1234 && p3[1]==0x5678 && n==nitems);
    int32_t *p4=(int32_t *)get_prop(win, prop, 4, &n);
    assert(p4[0]==0x1234 && p4[1]==0x5678 && n==nitems);

    long a3[]={0x10203040, 0x50607080};
    nitems=ARRAY_NUM(a3);
    change_prop(win, prop, type, 32, PropModeReplace,
        (unsigned char *)a3, sizeof(long), nitems);
    int32_t *p5=(int32_t *)get_prop(win, prop, 4, &n);
    assert(p5[0]==0x10203040 && p5[1]==0x50607080 && n==nitems);
    int64_t *p6=(int64_t *)get_prop(win, prop, 8, &n);
    assert(p6[0]==0x10203040 && p6[1]==0x50607080 && n==nitems);

    struct test_tag { int8_t a; int8_t b; } t1 = { 1, 2 }, *t2;
    change_prop(win, prop, type, 32, PropModeReplace,
        (unsigned char *)&t1, sizeof(long), CEIL_DIV(sizeof(t1), sizeof(long)));
    t2=(struct test_tag *)get_prop(win, prop, 4, &n);
    assert(t2->a==1 && t2->b==2 && n==1);

    vfree(p1, p2, p3, p4, p5, p6, t2);
}

static void test_motif(void)
{
    set_motif_wm_hints_atom();
    assert(motif_wm_hints_atom != None);
    assert(get_motif_wm_hints(win) == NULL);
    assert(has_motif_decoration(win));

    MotifWmHints *p=NULL, hints[]=
    {
        {.flags=MWM_HINTS_DECORATIONS, .decorations=1},
        {.flags=~0, .decorations=1},
        {.flags=~0, .decorations=0},
        {.flags=0, .decorations=1},
    };
    bool exps[]={true, true, false, true};
    for(size_t i=0; i<ARRAY_NUM(hints); i++)
    {
        set_motif_wm_hints(win, &hints[i]);
        assert((p=get_motif_wm_hints(win)));
        Free(p);
        assert(has_motif_decoration(win) == exps[i]);
    }
}

static void test_utf8_string(void)
{
    set_utf8_string_atom();
    assert(get_utf8_string_atom() != None);
    replace_utf8_prop(win, prop, "test");
    char *s1=get_utf8_string_prop(win, prop);
    assert(strcmp(s1, "test") == 0);
    replace_utf8_prop(win, prop, "");
    char *s2=get_utf8_string_prop(win, prop);
    assert(strcmp(s2, "") == 0);

    char *s[]={"just", "a", "test"};
    unsigned long n=0, nitems=ARRAY_NUM(s);
    replace_utf8s_prop(win, prop, s, nitems);
    char *s3=get_utf8_strings_prop(win, prop, &n), *p=s3;
    for(unsigned long i=0; i<nitems; i++)
    {
        assert(strcmp(s[i], p) == 0);
        p+=strlen(p)+1;
    }

    vfree(s1, s2, s3);
}

static void test_cardinal_prop(void)
{
    replace_cardinal_prop(win, prop, 1);
    assert(get_cardinal_prop(win, prop, 0) == 1);

    unsigned long n=0;
    long a[]={1, 2}, b=3, *p=NULL;
    replace_cardinals_prop(win, prop, a, ARRAY_NUM(a));
    p=get_cardinals_prop(win, prop, &n);
    assert(n == ARRAY_NUM(a));
    for(unsigned long i=0; i<n; i++)
        assert(p[i] == a[i]);
    Free(p);

    replace_cardinals_prop(win, prop, &b, 1);
    p=get_cardinals_prop(win, prop, &n);
    assert(n==1 && *p==b);
    Free(p);
}

static void test_atom_prop(void)
{
    replace_atom_prop(win, prop, 1);
    assert(get_atom_prop(win, prop) == 1);

    Atom a[]={1, 2}, *p=NULL;
    unsigned long n=0;
    replace_atoms_prop(win, prop, a, ARRAY_NUM(a));
    p=get_atoms_prop(win, prop, &n);
    assert(n == ARRAY_NUM(a));
    for(unsigned long i=0; i<n; i++)
        assert(p[i] == a[i]);
    Free(p);
}

static void test_window_prop(void)
{
    replace_window_prop(win, prop, 1);
    assert(get_window_prop(win, prop) == 1);

    Window a[]={1, 2}, *p=NULL;
    unsigned long n=0;
    replace_windows_prop(win, prop, a, ARRAY_NUM(a));
    p=get_windows_prop(win, prop, &n);
    assert(n == ARRAY_NUM(a));
    for(unsigned long i=0; i<n; i++)
        assert(p[i] == a[i]);
}

static void test_gwm_prop(void)
{
    for(GWM_atom_id id=0; id<GWM_ATOM_N; id++)
        assert(gwm_atoms[id] && is_spec_gwm_atom(gwm_atoms[id], id));
    assert(!is_spec_gwm_atom(None, GWM_LAYOUT));

    set_gwm_layout(1);
    assert(get_gwm_layout() == 1);

    request_layout_update();
    assert(get_cardinal_prop(xinfo.root_win, gwm_atoms[GWM_UPDATE_LAYOUT], -1) == 1);

    set_main_color_name("test");
    char *p=get_main_color_name();
    assert(strcmp(p, "test") == 0);
}

static void test_copy_prop(void)
{
    XDeleteProperty(xinfo.display, win, prop);
    replace_window_prop(win, prop, 1);
    Window w=XCreateSimpleWindow(xinfo.display, xinfo.root_win,
        -1, -1, 1, 1, 0, 0, 0);
    copy_prop(w, win);
    assert(get_window_prop(w, prop) == 1);
}
