/* *************************************************************************
 *     taskbar->c：實現任務欄相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"

typedef struct // 狀態欄
{
    Widget base;
    char *label;
} Statusbar;

typedef struct cbutton_tag
{
    Button *button;
    Window cwin; // 縮微客戶窗口
    struct cbutton_tag *next;
} Cbutton;

typedef struct // 縮微窗口欄
{
    Widget base;
    Cbutton *cbuttons;
} Iconbar;

struct _taskbar_tag // 任務欄
{
    Widget base;
    int urgency_n[DESKTOP_N], attent_n[DESKTOP_N]; // 各桌面緊急、注意提示窗口數
    Button *buttons[TASKBAR_BUTTON_N];
    Iconbar *iconbar;
    Statusbar *statusbar;
};

static void create_taskbar_buttons(void);
static bool is_chosen_taskbar_button(Widget_id id);
static void create_iconbar(void);
static void create_statusbar(void);
static void create_act_center(void);
static Cbutton *create_cbutton(Window parent, int x, int y, int w, int h, Window cwin);
static void set_cbutton_icon(Cbutton *cbutton);
static void add_cbutton(Cbutton *node);
static void del_cbutton(Cbutton *node);
static void destroy_cbutton(Cbutton *cbutton);
static bool have_similar_cbutton(const Cbutton *cbutton);
static Cbutton *win_to_cbutton(Window cwin);

Taskbar *taskbar=NULL;

Taskbar *create_taskbar(void)
{
    int w=xinfo.screen_width, h=get_font_height_by_pad(),
        y=(cfg->taskbar_on_top ? 0 : xinfo.screen_height-h);

    taskbar=malloc_s(sizeof(Taskbar));
    init_widget(WIDGET(taskbar), TASKBAR, UNUSED_TYPE, WIDGET_STATE_1(current),
        xinfo.root_win, 0, y, w, h);
    XSelectInput(xinfo.display, WIDGET_WIN(taskbar), CROSSING_MASK);

    create_taskbar_buttons();
    create_iconbar();
    create_statusbar();
    create_act_center();

    return taskbar;
}

static void create_taskbar_buttons(void)
{
    int w=cfg->taskbar_button_width, h=WIDGET_H(taskbar);

    for(int i=0; i<TASKBAR_BUTTON_N; i++)
    {
        Widget_id id=TASKBAR_BUTTON_BEGIN+i;
        Widget_state state={.chosen=is_chosen_taskbar_button(id), .current=1};
        taskbar->buttons[i]=create_button(id, state, taskbar->base.win,
            w*i, 0, w, h, cfg->taskbar_button_text[i]);
        set_widget_tooltip(WIDGET(taskbar->buttons[i]), cfg->tooltip[id]);
    }
}

static bool is_chosen_taskbar_button(Widget_id id)
{
    unsigned int n, lay;

    return (get_net_current_desktop(&n) && id==DESKTOP_BUTTON_BEGIN+n)
        || (get_gwm_current_layout((int *)&lay) && id==LAYOUT_BUTTON_BEGIN+lay);
}

static void create_iconbar(void)
{
    int bw=cfg->taskbar_button_width*TASKBAR_BUTTON_N, x=bw,
        w=WIDGET_W(taskbar)-bw, h=WIDGET_H(taskbar);

    taskbar->iconbar=malloc_s(sizeof(Iconbar));
    init_widget(WIDGET(taskbar->iconbar), ICONBAR, UNUSED_TYPE,
        WIDGET_STATE(taskbar), WIDGET_WIN(taskbar), x, 0, w, h);
    taskbar->iconbar->cbuttons=NULL;
}

static void create_statusbar(void)
{
    int w=0;
    char *p=get_text_prop(xinfo.root_win, XA_WM_NAME);

    taskbar->statusbar=malloc_s(sizeof(Statusbar));
    if(!p)
        p=copy_string("gwm");
    get_string_size(p, &w, NULL);
    w += 2*get_font_pad();
    if(w > cfg->status_area_width_max)
        w=cfg->status_area_width_max;
    else if(w == 0)
        w=1;
    init_widget(WIDGET(taskbar->statusbar), STATUSBAR, UNUSED_TYPE,
        WIDGET_STATE(taskbar), WIDGET_WIN(taskbar), WIDGET_W(taskbar)-w,
        0, w, WIDGET_H(taskbar));
    XSelectInput(xinfo.display, WIDGET_WIN(taskbar->statusbar), ExposureMask);
    taskbar->statusbar->label=copy_string(p);
}

static void create_act_center(void)
{
    act_center=create_menu(ACT_CENTER, xinfo.root_win,
        cfg->act_center_item_icon, cfg->act_center_item_symbol,
        cfg->act_center_item_label, ACT_CENTER_ITEM_N, cfg->act_center_col);
}

void taskbar_add_cbutton(Window cwin)
{
    int h=WIDGET_H(taskbar->iconbar), w=h;
    Cbutton *c=create_cbutton(WIDGET_WIN(taskbar->iconbar), 0, 0, w, h, cwin);

    add_cbutton(c);
    update_iconbar();
    show_button(c->button);
}

static Cbutton *create_cbutton(Window parent, int x, int y, int w, int h, Window cwin)
{
    Cbutton *cbutton=malloc_s(sizeof(Cbutton));
    char *icon_title=get_icon_title_text(cwin, "");

    cbutton->button=create_button(CLIENT_ICON, WIDGET_STATE_1(current),
        parent, x, y, w, h, icon_title);
    set_button_align(cbutton->button, CENTER_LEFT);

    cbutton->cwin=cwin;
    cbutton->next=NULL;

    set_cbutton_icon(cbutton);
    set_widget_tooltip(WIDGET(cbutton), icon_title);
    free_s(icon_title);

    return cbutton;
}

static void set_cbutton_icon(Cbutton *cbutton)
{
    XClassHint class_hint={NULL, NULL};

    XGetClassHint(xinfo.display, cbutton->cwin, &class_hint);
    Imlib_Image image=get_icon_image(cbutton->cwin, class_hint.res_name,
        cfg->icon_image_size, cfg->cur_icon_theme);

    set_button_icon(cbutton->button, image, class_hint.res_name, NULL);
    XFree(class_hint.res_name), XFree(class_hint.res_class);
}

static void add_cbutton(Cbutton *node)
{
    node->next=taskbar->iconbar->cbuttons;
    taskbar->iconbar->cbuttons=node;
}

void taskbar_del_cbutton(Window cwin)
{
    for(Cbutton *c=taskbar->iconbar->cbuttons; c; c=c->next)
    {
        if(c->cwin == cwin)
        {
            del_cbutton(c);
            destroy_cbutton(c);
            update_iconbar();
            break;
        }
    }
}

static void del_cbutton(Cbutton *node)
{
    for(Cbutton *prev=NULL, *p=taskbar->iconbar->cbuttons; p; prev=p, p=p->next)
    {
        if(p == node)
        {
            if(prev == NULL)
                taskbar->iconbar->cbuttons=p->next;
            else
                prev->next=p->next;
            break;
        }
    }
}

static void destroy_cbutton(Cbutton *cbutton)
{
    destroy_button(cbutton->button);
    free_s(cbutton);
}

Window get_iconic_win(Window button_win)
{
    for(unsigned int i=0; i<DESKTOP_N; i++)
        for(Cbutton *c=taskbar->iconbar->cbuttons; c; c=c->next)
            if(WIDGET_WIN(c->button) == button_win)
                return c->cwin;
    return None;
}

void update_iconbar(void)
{
    int x=0, w=0, h=WIDGET_H(taskbar->iconbar), wi=h, wl=0;

    for(Cbutton *c=taskbar->iconbar->cbuttons; c; c=c->next)
    {
        Button *b=c->button;
        if(have_similar_cbutton(c))
        {
            puts(get_button_label(b));
            get_string_size(get_button_label(b), &wl, NULL);
            w=MIN(wi+wl, cfg->icon_win_width_max);
        }
        else
            w=WIDGET_W(b);
        move_resize_widget(WIDGET(b), x, WIDGET_Y(b), w, WIDGET_H(b)); 
        x+=w+cfg->icon_gap;
    }
}

static bool have_similar_cbutton(const Cbutton *cbutton)
{
    XClassHint ch={NULL, NULL}, ph;
    if(!XGetClassHint(xinfo.display, cbutton->cwin, &ch))
        return false;

    bool same=false;
    for(Cbutton *p=taskbar->iconbar->cbuttons; !same && p; p=p->next)
    {
        if(p!=cbutton && XGetClassHint(xinfo.display, p->cwin, &ph))
        {
            same = (strcmp(ph.res_class, ch.res_class) == 0
                && strcmp(ph.res_name, ch.res_name) == 0);
            XFree(ph.res_class), XFree(ph.res_name);
        }
    }
    
    XFree(ch.res_class), XFree(ch.res_name);

    return same;
}

void update_iconbar_by_state(Window cwin)
{
    Net_wm_state state=get_net_wm_state(cwin);
    Cbutton *cbutton=win_to_cbutton(cwin);
    
    if(cbutton)
    {
        if(!state.hidden)
            taskbar_del_cbutton(cwin);
        if(state.focused)
            WIDGET_STATE(WIDGET(cbutton->button)).current=1,
            update_widget_bg(WIDGET(cbutton->button));
    }
    else if(state.hidden)
        taskbar_add_cbutton(cwin);
}

static Cbutton *win_to_cbutton(Window cwin)
{
    for(Cbutton *p=taskbar->iconbar->cbuttons; p; p=p->next)
        if(p->cwin == cwin)
            return p;
    return NULL;
}

void update_taskbar_buttons_bg(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        update_widget_bg(WIDGET(taskbar->buttons[i]));
}

void update_statusbar_fg(void)
{
    Statusbar *b=taskbar->statusbar;
    XftColor fg=get_widget_fg(WIDGET_STATE(b));
    if(b->label)
    {
        Str_fmt fmt={0, 0, WIDGET_W(b), WIDGET_H(b), CENTER, true, false, 0, fg};
        draw_string(WIDGET_WIN(b), b->label, &fmt);
    }
}

void set_statusbar_label(const char *label)
{
    int w=0, h=WIDGET_H(taskbar->statusbar);

    free_s(taskbar->statusbar->label);
    taskbar->statusbar->label=copy_string(label);
    get_string_size(label, &w, NULL);
    w += 2*get_font_pad();
    if(w > cfg->status_area_width_max)
        w=cfg->status_area_width_max;
    if(w != taskbar->statusbar->base.w)
        move_resize_widget(WIDGET(taskbar->statusbar), taskbar->base.w-w, 0, w, h);
    update_statusbar_fg();
}

void update_taskbar_bg(void)
{
    update_taskbar_buttons_bg();
    update_widget_bg(WIDGET(taskbar->iconbar));
    /* Xlib手冊說窗口收到Expose事件時會更新背景，但事實上不知道爲何，上邊的語句
     * 雖然給iconbar->win發送了Expose事件，但實際上沒更新背景。也許當窗口沒有內容
     * 時，收到Expose事件並不會更新背景。故只好調用本函數強制更新背景。 */
    XClearWindow(xinfo.display, taskbar->iconbar->base.win);
    update_widget_bg(WIDGET(taskbar->statusbar));
}

void destroy_taskbar(Taskbar *taskbar)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        destroy_button(taskbar->buttons[i]);
    destroy_widget(WIDGET(taskbar->iconbar));
    free_s(taskbar->statusbar->label);
    destroy_widget(WIDGET(taskbar->statusbar));
    destroy_widget(WIDGET(taskbar));
}

void set_taskbar_urgency(unsigned int desktop_mask, bool urg)
{
    unsigned int cur_desktop;
    if(get_net_current_desktop(&cur_desktop))
        return;

    int incr = (urg ? 1 : -1);
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if( (desktop_mask & get_desktop_mask(i+1) && i!=cur_desktop)
            && taskbar->urgency_n[i]+incr >= 0)
             taskbar->urgency_n[i] += incr;
    update_taskbar_buttons_bg();
}

void set_taskbar_attention(unsigned int desktop_mask, bool attent)
{
    unsigned int cur_desktop;
    if(get_net_current_desktop(&cur_desktop))
        return;

    int incr = attent ? 1 : -1;
    for(unsigned int i=0; i<DESKTOP_N; i++)
        if((desktop_mask & get_desktop_mask(i+1) && i!=cur_desktop)
            && taskbar->attent_n[i]+incr >= 0)
            taskbar->attent_n[i] += incr;
    update_taskbar_buttons_bg();
}

void update_taskbar_buttons_bg_by_chosen(void)
{
    for(int i=0; i<TASKBAR_BUTTON_N; i++)
        WIDGET_STATE(taskbar->buttons[i]).chosen=
            is_chosen_taskbar_button(TASKBAR_BUTTON_BEGIN+i);
    update_taskbar_buttons_bg();
}
