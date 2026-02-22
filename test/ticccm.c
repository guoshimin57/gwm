/* *************************************************************************
 *     ticccm.c：對icccm模塊進行單元測試。
 *     版權 (C) 2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "../src/icccm.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define suite_assert(i, exp) \
    do { if(!(exp)) printf("在suite[%lu]發現錯誤:\n", i); assert(exp); } while(0);

static void test_icccm_atom(void)
{
    set_icccm_atoms();
    for(ICCCM_atom_id id=0; id<ICCCM_ATOMS_N; id++)
        assert(icccm_atoms[id] && is_spec_icccm_atom(icccm_atoms[id], id));
    assert(!is_spec_icccm_atom(None, WM_DELETE_WINDOW));
}

static void test_get_win_col(void)
{
    struct { int width; XSizeHints hints; int exp; } suite[]=
    {
        {2, {.flags=0}, 0},
        {2, {.flags=PBaseSize}, 0},
        {2, {.flags=PBaseSize|PResizeInc}, 0},
        {2, {.flags=PBaseSize|PResizeInc, .width_inc=1}, 2},
        {2, {.flags=PBaseSize|PResizeInc, .width_inc=1, .base_width=1}, 1},
    };
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, get_win_col(suite[i].width, &suite[i].hints) == suite[i].exp);
}

static void test_get_win_row(void)
{
    struct { int height; XSizeHints hints; int exp; } suite[]=
    {
        {2, {.flags=0}, 0},
        {2, {.flags=PBaseSize}, 0},
        {2, {.flags=PBaseSize|PResizeInc}, 0},
        {2, {.flags=PBaseSize|PResizeInc, .height_inc=1}, 2},
        {2, {.flags=PBaseSize|PResizeInc, .height_inc=1, .base_height=1}, 1},
    };
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, get_win_row(suite[i].height, &suite[i].hints) == suite[i].exp);
}

static Window create_test_win(void)
{
    return XCreateSimpleWindow(xinfo.display, xinfo.root_win, 0, 0, 1, 1, 0, 0, 0);
}

static void test_get_size_hints(void)
{
    Window win=create_test_win();

    XSizeHints get;
    struct { XSizeHints set, exp; } suite[]=
    {
        {{0}, {.flags=PResizeInc, .width_inc=1, .height_inc=1}},
        {{.flags=PBaseSize, .base_width=1},
         {.flags=PBaseSize|PMinSize|PResizeInc, .base_width=1,
            .min_width=1, .width_inc=1, .height_inc=1}},
        {{.flags=PMinSize, .min_width=1},
         {.flags=PBaseSize|PMinSize|PResizeInc, .base_width=1,
            .min_width=1, .width_inc=1, .height_inc=1}},
        {{.flags=PResizeInc, .width_inc=1}, {.flags=PResizeInc, .width_inc=1}},
    };
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
    {
        XSetWMNormalHints(xinfo.display, win, &suite[i].set);
        get=get_size_hint(win);
        suite_assert(i, memcmp(&get, &suite[i].exp, sizeof(XSizeHints)) == 0);
    }
}

static void test_is_resizable(void)
{
    struct { XSizeHints hints; bool exp; } suite[]=
    {
        {{0}, true},
        {{.flags=PMinSize|PMaxSize, .min_width=1, .max_width=2}, true},
        {{.flags=PMinSize|PMaxSize, .min_width=1, .max_width=2,
            .min_height=1, .max_height=2}, true},
        {{.flags=PMinSize|PMaxSize, .min_width=1,
            .max_width=1, .min_height=1, .max_height=1}, false},
        {{.flags=PMinSize, .min_width=1, .max_width=1,
            .min_height=1, .max_height=1}, true},
    };
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, is_resizable(&suite[i].hints) == suite[i].exp);
}

static void test_fix_win_size_by_hint(void)
{
    struct { XSizeHints hint; int w, h; int ew, eh; } suite[]=
    {
        {{.flags=0, .width=1, .height=2}, 3, 4, 3, 4},
        {{.flags=USSize, .width=1, .height=2}, 3, 4, 1, 2},
        {{.flags=PSize, .width=1, .height=2}, 3, 4, 1, 2},
        {{.flags=PBaseSize, .base_width=1, .base_height=2}, 3, 4, 3, 4},
        {{.flags=PResizeInc, .base_width=1, .base_height=2, .width_inc=3, .height_inc=4}, 5, 6, 5, 6},
        {{.flags=PBaseSize|PResizeInc, .base_width=1, .base_height=2, .width_inc=3, .height_inc=4}, 5, 6, 4, 6},
        {{.flags=PBaseSize|PResizeInc, .base_width=1, .width_inc=2}, 3, 4, 3, 4},
        {{.flags=PMinSize, .min_width=1, .min_height=2}, 2, 1, 2, 2},
        {{.flags=PMaxSize, .max_width=1, .max_height=2}, 2, 1, 1, 1},
        {{.flags=PMinSize|PMaxSize, .min_width=2, .min_height=3, .max_width=4, .max_height=5}, 1, 6, 2, 5},
        {{.flags=PMinSize|PMaxSize, .min_width=1, .min_height=1, .max_width=1, .max_height=1}, 1, 2, 1, 1},
        {{.flags=PAspect, .min_aspect={1, 2}, .max_aspect={3, 4}}, 8, 9, 7, 9},
        {{.flags=PAspect, .min_aspect={1, 2}, .max_aspect={3, 4}}, 1, 3, 1, 1},
    };
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
    {
        fix_win_size_by_hint(&suite[i].hint, &suite[i].w, &suite[i].h);
        suite_assert(i, suite[i].w==suite[i].ew && suite[i].h==suite[i].eh);
    }
}

static void test_is_prefer_width(void)
{
    struct { int w; XSizeHints hint; bool exp; } suite[]=
    {
        { 0, {.flags=0}, true},
        { 1, {.flags=~(PMinSize|PMaxSize|PBaseSize|PResizeInc)}, true},
        { 1, {.flags=PMinSize, .min_width=1}, true},
        { 1, {.flags=PMinSize, .min_width=2}, false},
        { 1, {.flags=PMaxSize, .max_width=1}, true},
        { 2, {.flags=PMaxSize, .max_width=1}, false},
        { 2, {.flags=PBaseSize|PResizeInc, .base_width=1, .width_inc=1}, true},
        { 2, {.flags=PBaseSize|PResizeInc, .base_width=1, .width_inc=2}, false},
        { 2, {.flags=PBaseSize, .base_width=1}, true},
        { 2, {.flags=PResizeInc, .width_inc=1}, true},
    };
    assert(is_prefer_width(0, NULL));
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, is_prefer_width(suite[i].w, &suite[i].hint) == suite[i].exp);
}

static void test_is_prefer_height(void)
{
    struct { int w; XSizeHints hint; bool exp; } suite[]=
    {
        { 0, {.flags=0}, true},
        { 1, {.flags=~(PMinSize|PMaxSize|PBaseSize|PResizeInc)}, true},
        { 1, {.flags=PMinSize, .min_height=1}, true},
        { 1, {.flags=PMinSize, .min_height=2}, false},
        { 1, {.flags=PMaxSize, .max_height=1}, true},
        { 2, {.flags=PMaxSize, .max_height=1}, false},
        { 2, {.flags=PBaseSize|PResizeInc, .base_height=1, .height_inc=1}, true},
        { 2, {.flags=PBaseSize|PResizeInc, .base_height=1, .height_inc=2}, false},
        { 2, {.flags=PBaseSize, .base_height=1}, true},
        { 2, {.flags=PResizeInc, .height_inc=1}, true},
    };

    assert(is_prefer_height(0, NULL));
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, is_prefer_height(suite[i].w, &suite[i].hint)
            == suite[i].exp);
}

static void test_is_in_size_limit(void)
{
    struct { int w, h; XSizeHints hint; bool exp; } suite[]=
    {
        { 0, 0, {.flags=0}, true},
        { 1, 1, {.flags=~(PMinSize|PMaxSize)}, true},
        { 1, 1, {.flags=PMinSize, .min_width=1}, true},
        { 1, 1, {.flags=PMinSize, .min_width=2}, false},
        { 1, 1, {.flags=PMaxSize, .max_width=1}, true},
        { 2, 1, {.flags=PMaxSize, .max_width=1}, false},
        { 1, 1, {.flags=PMinSize, .min_height=1}, true},
        { 1, 1, {.flags=PMinSize, .min_height=2}, false},
        { 1, 1, {.flags=PMaxSize, .max_height=1}, true},
        { 1, 2, {.flags=PMaxSize, .max_height=1}, false},
        { 1, 1, {.flags=PMinSize, .min_width=1, .min_height=1}, true},
        { 1, 1, {.flags=PMinSize, .min_width=2, .min_height=2}, false},
        { 1, 1, {.flags=PMaxSize, .max_width=1, .max_height=1}, true},
        { 2, 2, {.flags=PMinSize, .max_width=1, .max_height=1}, false},
    };
    assert(is_in_size_limit(0, 0, NULL));
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, is_in_size_limit(suite[i].w, suite[i].h, &suite[i].hint) == suite[i].exp);
}
static void test_is_prefer_aspect(void)
{
    struct { int w, h; XSizeHints hint; bool exp; } suite[]=
    {
        { 1, 1, {.flags=0}, true },
        { 0, 1, {.flags=PAspect}, true },
        { 1, 0, {.flags=PAspect}, true },
        { 1, 1, {.flags=PAspect, .min_aspect.x=0, .min_aspect.y=1}, true }, 
        { 1, 1, {.flags=PAspect, .min_aspect.x=1, .min_aspect.y=0}, true }, 
        { 1, 1, {.flags=PAspect, .max_aspect.x=0, .max_aspect.y=1}, true }, 
        { 1, 1, {.flags=PAspect, .max_aspect.x=1, .max_aspect.y=0}, true }, 
        { 1, 1, {.flags=PAspect, .min_aspect.x=1, .min_aspect.y=1,
                                 .max_aspect.x=1, .max_aspect.y=1}, true }, 
        { 2, 1, {.flags=PAspect, .min_aspect.x=1, .min_aspect.y=1,
                                 .max_aspect.x=2, .max_aspect.y=1}, true }, 
        { 1, 2, {.flags=PAspect, .min_aspect.x=1, .min_aspect.y=1,
                                 .max_aspect.x=1, .max_aspect.y=2}, false }, 
        { 1, 2, {.flags=PAspect, .min_aspect.x=1, .min_aspect.y=2,
                                 .max_aspect.x=1, .max_aspect.y=1}, true }, 
        { 2, 1, {.flags=PAspect, .min_aspect.x=2, .min_aspect.y=1,
                                 .max_aspect.x=1, .max_aspect.y=1}, false }, 
    };

    assert(is_prefer_aspect(1, 1, NULL));
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, is_prefer_aspect(suite[i].w, suite[i].h, &suite[i].hint)
            == suite[i].exp);
}

static void test_is_prefer_size(void)
{
    struct { int w, h; XSizeHints hint; bool exp; } suite[]=
    {
        { 0, 1, {0}, true},
        { 1, 0, {0}, true},
        { 1, 1, {0}, true},
        { 2, 2, {.flags=~0, .min_width=1, .min_height=1,
                 .max_width=3, .max_height=3,
                 .base_width=1, .base_height=1,
                 .width_inc=1, .height_inc=1,
                 .min_aspect={1, 2}, .max_aspect={2, 2}}, true 
        },
    };

    assert(is_prefer_size(1, 1, NULL));
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, is_prefer_size(suite[i].w, suite[i].h, &suite[i].hint)
            == suite[i].exp);
}

static void test_has_focus_hint(void)
{
    struct { XWMHints hint; bool exp; } suite[]=
    {
        { {.flags=InputHint, .input=true }, true },
        { {.flags=InputHint, .input=false }, false },
        { {.flags=0, .input=true }, false },
        { {.flags=0, .input=false}, false },
    };
    
    assert(has_focus_hint(NULL));
    for(size_t i=0; i<ARRAY_NUM(suite); i++)
        suite_assert(i, has_focus_hint(&suite[i].hint) == suite[i].exp);
}

static void test_has_spec_wm_protocol(void)
{
    Window win=create_test_win();
    for(size_t i=0; i<ICCCM_ATOMS_N; i++)
        suite_assert(i, !has_spec_wm_protocol(win, icccm_atoms[i]));
    XSetWMProtocols(xinfo.display, win, icccm_atoms, ICCCM_ATOMS_N);
    for(size_t i=0; i<ICCCM_ATOMS_N; i++)
        suite_assert(i, has_spec_wm_protocol(win, icccm_atoms[i]));
    XDestroyWindow(xinfo.display, win);
}

static void test_send_wm_protocol_msg(void)
{
    XEvent e;
    Window win=create_test_win();
    assert(!send_wm_protocol_msg(icccm_atoms[0], win));
    XSetWMProtocols(xinfo.display, win, icccm_atoms, ICCCM_ATOMS_N);
    for(size_t i=0; i<ICCCM_ATOMS_N; i++)
    {
        send_wm_protocol_msg(icccm_atoms[i], win);
        XSync(xinfo.display, False);
        assert(XCheckTypedWindowEvent(xinfo.display, win, ClientMessage, &e));
        suite_assert(i, is_spec_icccm_atom(e.xclient.message_type, WM_PROTOCOLS));
    }
    XDestroyWindow(xinfo.display, win);
}

static void test_set_urgency_hint(void)
{
    Window win=create_test_win();
    XWMHints *p=NULL, h0={0}, h1={.flags=XUrgencyHint};

    set_urgency_hint(win, NULL, true);
    p=XGetWMHints(xinfo.display, win);
    assert(p && (p->flags & XUrgencyHint));

    set_urgency_hint(win, NULL, false);
    p=XGetWMHints(xinfo.display, win);
    assert(p && !(p->flags & XUrgencyHint));

    set_urgency_hint(win, &h0, true);
    p=XGetWMHints(xinfo.display, win);
    assert(p && (p->flags & XUrgencyHint));

    set_urgency_hint(win, &h0, false);
    p=XGetWMHints(xinfo.display, win);
    assert(p && !(p->flags & XUrgencyHint));

    set_urgency_hint(win, &h1, true);
    p=XGetWMHints(xinfo.display, win);
    assert(p && (p->flags & XUrgencyHint));

    set_urgency_hint(win, &h1, false);
    p=XGetWMHints(xinfo.display, win);
    assert(p && !(p->flags & XUrgencyHint));

    XDestroyWindow(xinfo.display, win);
}

static void test_is_iconic_state(void)
{
    Window win=create_test_win();

    assert(!is_iconic_state(win));
    replace_cardinal_prop(win, icccm_atoms[WM_STATE], NormalState);
    assert(!is_iconic_state(win));
    replace_cardinal_prop(win, icccm_atoms[WM_STATE], IconicState);
    assert(is_iconic_state(win));

    XDestroyWindow(xinfo.display, win);
}

static bool done_close_win(Window win)
{
    if(!has_spec_wm_protocol(win, icccm_atoms[WM_DELETE_WINDOW]))
        return true;

    XEvent e;
    return XCheckTypedWindowEvent(xinfo.display, win, ClientMessage, &e);
}

static void test_close_win(void)
{
    Window win=create_test_win();

    /* 执行以下测试应通过，但XKillClient会导致测试程序终止，故忽略
    close_win(win);
    assert(done_close_win(win));
    */
	XSetWMProtocols(xinfo.display, win, &icccm_atoms[WM_DELETE_WINDOW], 1);
    close_win(win);
    assert(done_close_win(win));

    XDestroyWindow(xinfo.display, win);
}

static void test_get_wm_name(void)
{
    Window win=create_test_win();

    assert(!get_wm_name(win));
    XStoreName(xinfo.display, win, "test");
    assert(strcmp(get_wm_name(win), "test") == 0);

    XDestroyWindow(xinfo.display, win);
}

static void test_get_wm_icon_name(void)
{
    Window win=create_test_win();

    assert(!get_wm_icon_name(win));
    XSetIconName(xinfo.display, win, "test");
    assert(strcmp(get_wm_icon_name(win), "test") == 0);

    XDestroyWindow(xinfo.display, win);
}

static void test_set_client_leader(void)
{
    Window win=create_test_win();
    Window leader=create_test_win();
    assert(get_window_prop(win, icccm_atoms[WM_CLIENT_LEADER]) == None);
    set_client_leader(win, leader);
    assert(get_window_prop(win, icccm_atoms[WM_CLIENT_LEADER]) == leader);
}

int main(void)
{
    xinfo.display=XOpenDisplay(NULL);
    xinfo.screen=DefaultScreen(xinfo.display);
    xinfo.root_win=RootWindow(xinfo.display, xinfo.screen);

    test_icccm_atom();
    test_get_win_col();
    test_get_win_row();
    test_get_size_hints();
    test_is_resizable();
    test_fix_win_size_by_hint();
    test_is_prefer_width();
    test_is_prefer_height();
    test_is_prefer_aspect();
    test_is_prefer_size();
    test_is_in_size_limit();
    test_has_focus_hint();
    test_has_spec_wm_protocol();
    test_send_wm_protocol_msg();
    test_set_urgency_hint();
    test_is_iconic_state();
    test_close_win();
    test_get_wm_name();
    test_get_wm_icon_name();
    test_set_client_leader();

    XCloseDisplay(xinfo.display);

    return 0;
}
