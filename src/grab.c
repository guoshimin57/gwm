/* *************************************************************************
 *     grab.c：實現獨享按鍵、按鈕功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

static unsigned int get_num_lock_mask(void);
static unsigned int get_valid_mask(unsigned int mask);
static unsigned int get_modifier_mask(KeySym key_sym);

void grab_keys(void)
{
    unsigned int num_lock_mask=get_num_lock_mask();
    unsigned int masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};
    KeyCode code;
    XUngrabKey(xinfo.display, AnyKey, AnyModifier, xinfo.root_win);
    for(const Keybind *kb=cfg->keybind; kb->func; kb++)
        if((code=XKeysymToKeycode(xinfo.display, kb->keysym)))
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabKey(xinfo.display, code, kb->modifier|masks[j],
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
    
void grab_buttons(Client *c)
{
    unsigned int num_lock_mask=get_num_lock_mask(),
                 masks[]={0, LockMask, num_lock_mask, num_lock_mask|LockMask};

    XUngrabButton(xinfo.display, AnyButton, AnyModifier, c->win);
    for(const Buttonbind *b=cfg->buttonbind; b->func; b++)
    {
        if(b->widget_type == CLIENT_WIN)
        {
            int m=is_equal_modifier_mask(0, b->modifier) ?
                GrabModeSync : GrabModeAsync;
            for(size_t j=0; j<ARRAY_NUM(masks); j++)
                XGrabButton(xinfo.display, b->button, b->modifier|masks[j],
                    c->win, False, BUTTON_MASK, m, m, None, None);
        }
    }
}

bool is_equal_modifier_mask(unsigned int m1, unsigned int m2)
{
    return (get_valid_mask(m1) == get_valid_mask(m2));
}

static unsigned int get_valid_mask(unsigned int mask)
{
    return (mask & ~(LockMask|get_modifier_mask(XK_Num_Lock))
        & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask));
}

static unsigned int get_modifier_mask(KeySym key_sym)
{
    KeyCode kc;
    if((kc=XKeysymToKeycode(xinfo.display, key_sym)) != 0)
    {
        for(int i=0; i<8*xinfo.mod_map->max_keypermod; i++)
            if(xinfo.mod_map->modifiermap[i] == kc)
                return 1 << (i/xinfo.mod_map->max_keypermod);
        fprintf(stderr, _("錯誤：找不到指定的鍵符號相應的功能轉換鍵！\n"));
    }
    else
        fprintf(stderr, _("錯誤：指定的鍵符號不存在對應的鍵代碼！\n"));
    return 0;
}

bool grab_pointer(WM *wm, Window win, Pointer_act act)
{
    return XGrabPointer(xinfo.display, win, False, POINTER_MASK,
        GrabModeAsync, GrabModeAsync, None, wm->cursors[act], CurrentTime)
        == GrabSuccess;
}
