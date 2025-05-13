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

#include <X11/Xutil.h>
#include "misc.h"
#include "config.h"
#include "grab.h"

static unsigned int get_num_lock_mask(void);

static Cursor cursors[POINTER_ACT_N]; // 光標
static const Keybind *keybinds=NULL;
static const Buttonbind *buttonbinds=NULL;

void reg_binds(const Keybind *kbinds, const Buttonbind *bbinds)
{
    keybinds=kbinds;
    buttonbinds=bbinds;
}

const Keybind *get_keybinds(void)
{
    return keybinds;
}

const Buttonbind *get_buttonbinds(void)
{
    return buttonbinds;
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
void grab_buttons(Window win, Widget_id id)
{
    unsigned int num_lock_mask=get_num_lock_mask(),
                 masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};

    XUngrabButton(xinfo.display, AnyButton, AnyModifier, win);
    for(const Buttonbind *p=get_buttonbinds(); p && p->func; p++)
    {
        if(p->widget_id == id)
        {
            int m=is_equal_modifier_mask(0, p->modifier) ?
                GrabModeSync : GrabModeAsync;
            for(size_t i=0; i<ARRAY_NUM(masks); i++)
                XGrabButton(xinfo.display, p->button, p->modifier|masks[i],
                    win, False, BUTTON_MASK, m, m, None, None);
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
