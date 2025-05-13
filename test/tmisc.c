/* *************************************************************************
 *     tmisc.c：對misc模塊進行單元測試。
 *     版權 (C) 2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <stdlib.h>
#include <assert.h>
#include "../src/misc.c"

static void test_MIN(void);
static void test_MAX(void);
static void test_ARRAY_NUM(void);
static void test_CEIL_DIV(void);
static void incr(int *a);
static void test_vfunc(void);
static void test_mem_op(void);
static void test_base_n_floor(void);
static void test_base_n_ceil(void);
static void test_is_match_button_release(void);
static void test_get_desktop_mask(void);
static void test_quit(void);
static void want_quit(XEvent *e);
static void test_event_hendler(void);
static void test_is_equal_modifier_mask(void);

int main(void)
{
    xinfo.display=XOpenDisplay(NULL);
    xinfo.mod_map=XGetModifierMapping(xinfo.display);
    test_MIN();
    test_MAX();
    test_ARRAY_NUM();
    test_CEIL_DIV();
    test_vfunc();
    test_mem_op();
    test_base_n_floor();
    test_base_n_ceil();
    test_is_match_button_release();
    test_get_desktop_mask();
    test_quit();
    test_event_hendler();
    test_is_equal_modifier_mask();
    XFreeModifiermap(xinfo.mod_map);
    XCloseDisplay(xinfo.display);

    return EXIT_SUCCESS;
}

static void test_MIN(void)
{
    assert(MIN(1, 2) == 1);
    assert(MIN(-1, 1) == -1);
    assert(MIN(1.0, 1.1) == 1.0);
    assert(MIN(-1.0, 1.1) == -1.0);
    assert(MIN(1, 1.0) == 1);
}

static void test_MAX(void)
{
    assert(MAX(1, 2) == 2);
    assert(MAX(-1, 1) == 1);
    assert(MAX(1.0, 1.1) == 1.1);
    assert(MAX(-1.0, 1.1) == 1.1);
    assert(MAX(1, 1.0) == 1);
}

static void test_ARRAY_NUM(void)
{
    int a[1], b[2];
    void *c[1], *d[2];
    assert(ARRAY_NUM(a) == 1);
    assert(ARRAY_NUM(b) == 2);
    assert(ARRAY_NUM(c) == 1);
    assert(ARRAY_NUM(d) == 2);
}

static void test_CEIL_DIV(void)
{
    assert(CEIL_DIV(0, 2) == 0);
    assert(CEIL_DIV(1, 2) == 1);
    assert(CEIL_DIV(2, 2) == 1);
    assert(CEIL_DIV(2, 2L) == 1);
    assert(CEIL_DIV(2, 3L) == 1);
    assert(CEIL_DIV(3, 2) == 2);
}

static void incr(int *a)
{
    (*a)++;
}

static void test_vfunc(void)
{
    int a=0, b=1, c=2;
    vfunc(int, incr, &a);
    assert(a==1 && b==1 && c==2);
    vfunc(int, incr, &a, &b);
    assert(a==2 && b==2 && c==2);
    vfunc(int, incr, &a, &b, &c);
    assert(a==3 && b==3 && c==3);
}

static void test_mem_op(void)
{
    char *p=NULL, *p1=NULL, *p2=NULL, *p3=NULL;;

    assert(copy_string(NULL) == NULL);
    p=copy_string("test");
    assert(strcmp(p, "test") == 0);
    Free(p);
    assert(p == NULL);
    p=copy_string("");
    assert(strcmp(p, "") == 0);
    Free(p);
    assert(p == NULL);

    assert(copy_strings(NULL) == NULL);
    p1=copy_strings("test", NULL);
    assert(strcmp(p1, "test") == 0);
    vfree(p1);
    assert(p1);
    p2=copy_strings("test", "", NULL);
    assert(strcmp(p2, "test") == 0);
    p3=copy_strings("test", ".", NULL);
    assert(strcmp(p3, "test.") == 0);
    vfree(p2, p3);
    assert(p2 && p3);
}

static void test_base_n_floor(void)
{
    int suit[][3]={{3, 2, 2}, {3, 1, 3}, {4, 2, 4}, {5, 2, 4}, {5, 3, 3}};
    int n=ARRAY_NUM(suit);
    for(int i=0; i<n; i++)
        assert(base_n_floor(suit[i][0], suit[i][1]) == suit[i][2]);
}

static void test_base_n_ceil(void)
{
    int suit[][3]={{3, 2, 4}, {3, 1, 3}, {4, 2, 4}, {5, 2, 6}, {5, 3, 6}};
    int n=ARRAY_NUM(suit);
    for(int i=0; i<n; i++)
        assert(base_n_ceil(suit[i][0], suit[i][1]) == suit[i][2]);
}

static void test_is_match_button_release(void)
{
    XButtonEvent be1={.type=ButtonPress, .button=Button1},
        be2={.type=ButtonPress, .button=Button2},
        be3={.type=ButtonRelease, .button=Button1},
        be4={.type=ButtonRelease, .button=Button2};
    assert(!is_match_button_release(&be1, &be2));
    assert(is_match_button_release(&be1, &be3));
    assert(!is_match_button_release(&be1, &be4));
    assert(!is_match_button_release(&be3, &be1));
}

static void test_get_desktop_mask(void)
{
    unsigned int suit[][2]={{~0, ~0}, {0, 1}, {1, 2}, {2, 4}};
    int n=ARRAY_NUM(suit);
    for(int i=0; i<n; i++)
        assert(get_desktop_mask(suit[i][0]) == suit[i][1]);
}

static void test_quit(void)
{
    quit_flag=false;
    assert(!should_quit());
    request_quit();
    assert(should_quit());
}

static void want_quit(XEvent *e)
{
    UNUSED(e);
    quit_flag=true;
}

static void test_event_hendler(void)
{
    XEvent ev={0};
    quit_flag=false;
    init_event_handler(want_quit);
    handle_event(&ev);
    assert(should_quit());
}

static void test_is_equal_modifier_mask(void)
{
    unsigned int num_lock_mask=get_modifier_mask(XK_Num_Lock);
    unsigned int suit[][3]=
    {
        {ShiftMask, ShiftMask, 1},
        {ControlMask|Mod1Mask, ControlMask, 0},
        {ControlMask|Mod1Mask, ControlMask|Mod1Mask, 1},
        {Mod2Mask, Mod2Mask|LockMask, 1},
        {Mod2Mask, Mod3Mask, 0},
        {Mod4Mask, Mod4Mask|num_lock_mask, 1},
        {Mod5Mask, Mod5Mask|LockMask|num_lock_mask, 1},
        {ShiftMask|Mod5Mask, ShiftMask|Mod5Mask|LockMask, 1},
    };
    int n=ARRAY_NUM(suit);

    for(int i=0; i<n; i++)
        assert(is_equal_modifier_mask(suit[i][0], suit[i][1]) == suit[i][2]);
}
