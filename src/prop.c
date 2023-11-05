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

void replace_cardinal_prop(Window win, Atom prop, const CARD32 *values, int n)
{
    XChangeProperty(xinfo.display, win, prop, XA_CARDINAL, 32, PropModeReplace,
        (unsigned char *)&values, n);
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

bool send_client_msg(Atom wm_protocols, Atom proto, Window win)
{
    if(has_spec_wm_protocol(win, proto))
    {
        XEvent event;
        event.type=ClientMessage;
        event.xclient.window=win;
        event.xclient.message_type=wm_protocols;
        event.xclient.format=32;
        event.xclient.data.l[0]=proto;
        event.xclient.data.l[1]=CurrentTime;
        return XSendEvent(xinfo.display, win, False, NoEventMask, &event);
    }
    return false;
}

bool has_spec_wm_protocol(Window win, Atom protocol)
{
	int i, n;
	Atom *protocols=NULL;
	if(XGetWMProtocols(xinfo.display, win, &protocols, &n))
        for(i=0; i<n; i++)
            if(protocols[i] == protocol)
                { XFree(protocols); return true; }
    return false;
}
