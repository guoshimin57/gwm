/* *************************************************************************
 *     prop.c：實現與X特性相關的功能。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

Window get_transient_for(Display *display, Window win)
{
    Window owner;
    return XGetTransientForHint(display, win, &owner) ? owner : None;
}

unsigned char *get_prop(Display *display, Window win, Atom prop, unsigned long *n)
{
    int fmt;
    unsigned long nitems=0, *m=(n ? n : &nitems), rest;
    unsigned char *p=NULL;
    Atom type;

    /* 对于XGetWindowProperty，把要接收的数据长度（第5个参数）设置得比实际长度
     * 長可简化代码，这样就不必考虑要接收的數據是否不足32位。以下同理。 */
    if( XGetWindowProperty(display, win, prop, 0, ~0L, False,
        AnyPropertyType, &type, &fmt, m, &rest, &p)==Success && p && *m)
        return p;
    return NULL;
}

char *get_text_prop(Display *display, Window win, Atom atom)
{
    int n;
    char **list=NULL, *result=NULL;
    XTextProperty name;

    if(!XGetTextProperty(display, win, &name, atom))
        return NULL;

    if(name.encoding == XA_STRING)
        result=copy_string((char *)name.value);
    else if(Xutf8TextPropertyToTextList(display, &name, &list, &n) == Success
        && n && *list) // 與手冊不太一致的是，返回Success並不一定真的成功，狗血！
        result=copy_string(*list), XFreeStringList(list);
    XFree(name.value); // 手冊沒提及要釋放name或name.value，但很多WM都會釋放後者
    return result;
}

void replace_atom_prop(Display *display, Window win, Atom prop, const Atom *values, int n)
{
    XChangeProperty(display, win, prop, XA_ATOM, 32, PropModeReplace,
        (unsigned char *)values, n);
}

void replace_window_prop(Display *display, Window win, Atom prop, const Window *wins, int n)
{
    XChangeProperty(display, win, prop, XA_WINDOW, 32, PropModeReplace,
        (unsigned char *)wins, n);
}

void replace_cardinal_prop(Display *display, Window win, Atom prop, const CARD32 *values, int n)
{
    XChangeProperty(display, win, prop, XA_CARDINAL, 32, PropModeReplace,
        (unsigned char *)&values, n);
}

void copy_prop(Display *display, Window dest, Window src)
{
    int i=0, n=0, fmt=0;
    Atom type, *props=XListProperties(display, src, &n);

    if(!props)
        return;

    unsigned long nitems, rest;
    for(unsigned char *data=NULL; i<n; i++)
    {
        if( XGetWindowProperty(display, src, props[i], 0, ~0L, False,
            AnyPropertyType, &type, &fmt, &nitems, &rest, &data)==Success
            && data && nitems)
        {
            XChangeProperty(display, dest, props[i], type, fmt, PropModeReplace,
                data, nitems);
            XFree(data);
        }
    }
    XFree(props);
}

bool send_client_msg(Display *display, Atom wm_protocols, Atom proto, Window win)
{
    if(has_spec_wm_protocol(display, win, proto))
    {
        XEvent event;
        event.type=ClientMessage;
        event.xclient.window=win;
        event.xclient.message_type=wm_protocols;
        event.xclient.format=32;
        event.xclient.data.l[0]=proto;
        event.xclient.data.l[1]=CurrentTime;
        return XSendEvent(display, win, False, NoEventMask, &event);
    }
    return false;
}

bool has_spec_wm_protocol(Display *display, Window win, Atom protocol)
{
	int i, n;
	Atom *protocols=NULL;
	if(XGetWMProtocols(display, win, &protocols, &n))
        for(i=0; i<n; i++)
            if(protocols[i] == protocol)
                { XFree(protocols); return true; }
    return false;
}
