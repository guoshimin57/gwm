/* *************************************************************************
 *     grab.c：實現獨享輸入設備的功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "misc.h"
#include "config.h"
#include "drawable.h"
#include "grab.h"

static bool is_func_click(const Widget_id id, const Buttonbind *bind, XButtonEvent *be);
static unsigned int get_num_lock_mask(void);

static Cursor cursors[POINTER_ACT_N]; // 光標

bool is_valid_click(const Widget *widget, const Buttonbind *bind, XButtonEvent *be)
{
    Widget_id id = widget ? widget->id :
        (be->window==xinfo.root_win ? ROOT_WIN : UNUSED_WIDGET_ID);

    if(!is_func_click(id, bind, be))
        return false;

    if(!widget || widget_get_draggable(widget))
        return true;

    if(!grab_pointer(be->window, CHOOSE))
        return false;

    XEvent ev;
    do
    {
        XMaskEvent(xinfo.display, ROOT_EVENT_MASK|POINTER_MASK, &ev);
        handle_event(&ev);
    }while(!is_match_button_release(be, &ev.xbutton));
    XUngrabPointer(xinfo.display, CurrentTime);

    return is_equal_modifier_mask(be->state, ev.xbutton.state)
        && is_pointer_on_win(ev.xbutton.window);
}

static bool is_func_click(const Widget_id id, const Buttonbind *bind, XButtonEvent *be)
{
    return (bind->widget_id == id
        && bind->button == be->button
        && is_equal_modifier_mask(bind->modifier, be->state));
}

// 當需要拖拽操作時可考慮使用此函數獨享定位器
bool grab_pointer(Window win, Pointer_act act)
{
    return XGrabPointer(xinfo.display, win, False, POINTER_MASK,
        GrabModeAsync, GrabModeAsync, None, cursors[act], CurrentTime)
        == GrabSuccess;
}

// 當需要從非聚焦窗口上獲取鍵盤輸入時可考慮使用此函數獨享按鍵
void grab_keys(void)
{
    unsigned int num_lock_mask=get_num_lock_mask();
    unsigned int masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    KeyCode code;
    XUngrabKey(xinfo.display, AnyKey, AnyModifier, xinfo.root_win);
    for(const Keybind *p=get_keybinds(); p && p->func; p++)
        if((code=XKeysymToKeycode(xinfo.display, p->keysym)))
            for(size_t i=0; i<ARRAY_NUM(masks); i++)
                XGrabKey(xinfo.display, code, p->modifier|masks[i],
                    xinfo.root_win, True, GrabModeAsync, GrabModeAsync);
}

static unsigned int get_num_lock_mask(void)
{
	XModifierKeymap *m=XGetModifierMapping(xinfo.display);
    KeyCode code=XKeysymToKeycode(xinfo.display, XK_Num_Lock);

    if(code)
        for(int i=0; i<8; i++)
            for(int j=0; j<m->max_keypermod; j++)
                if(m->modifiermap[i*m->max_keypermod+j] == code)
                    { XFreeModifiermap(m); return (1<<i); }
    return 0;
}

/* 當主動獨享無法獲得按鈕輸入時可考慮使用此函數獨享按鈕。譬如客戶窗口已經選擇了
 * 點擊事件，WM無法再選擇點擊事件，此時可以用此獨享按鈕。非必要則不應使用它。在
 * buttonbinds設置的綁定默認會使用主動獨享，並不會使用此函數來設置被動獨享。*/
void grab_buttons(const Widget *widget)
{
    unsigned int num_lock_mask=get_num_lock_mask(),
                 masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};

    XUngrabButton(xinfo.display, AnyButton, AnyModifier, WIDGET_WIN(widget));
    for(const Buttonbind *p=get_buttonbinds(); p && p->func; p++)
    {
        if(p->widget_id == widget->id)
        {
            int m=is_equal_modifier_mask(0, p->modifier) ?
                GrabModeSync : GrabModeAsync;
            for(size_t i=0; i<ARRAY_NUM(masks); i++)
                XGrabButton(xinfo.display, p->button, p->modifier|masks[i],
                    WIDGET_WIN(widget), False, BUTTON_MASK, m, m, None, None);
        }
    }
}

void create_cursors(void)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        cursors[i]=XCreateFontCursor(xinfo.display, cfg->cursor_shape[i]);
}

void set_cursor(Window win, Pointer_act act)
{
    XDefineCursor(xinfo.display, win, cursors[act]);
}

void free_cursors(void)
{
    for(size_t i=0; i<POINTER_ACT_N; i++)
        XFreeCursor(xinfo.display, cursors[i]);
}
