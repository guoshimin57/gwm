/* *************************************************************************
 *     gui.c：實現圖形用戶界面相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "config.h"
#include "client.h"
#include "prop.h"
#include "font.h"
#include "file.h"
#include "taskbar.h"
#include "entry.h"
#include "menu.h"
#include "misc.h"
#include "grab.h"
#include "wallpaper.h"
#include "gui.h"

static void create_taskbar(void);
static void create_cmd_entry(Widget_id id);
static Strings *entry_get_cmd_completion(Entry *entry);
static char *entry_get_part_match_regex(Entry *entry);
static void create_color_entry(Widget_id id);

Entry *cmd_entry=NULL; // 輸入命令並執行的構件
Entry *color_entry=NULL; // 输入颜色名并设置颜色的構件

void init_gui(void)
{
    create_cursors();
    set_cursor(xinfo.root_win, NO_OP);
    load_fonts();
    alloc_color(cfg->main_color_name);
    init_wallpaper();
    set_default_wallpaper();
    create_taskbar();
    create_cmd_entry(RUN_CMD_ENTRY);
    create_color_entry(COLOR_ENTRY);
}

void deinit_gui(void)
{
    free_cursors();
    close_fonts();
    XClearWindow(xinfo.display, xinfo.root_win);
    free_wallpapers();
    taskbar_del();
    entry_del(cmd_entry);
    entry_del(color_entry);
}

static void create_taskbar(void)
{
    int w=xinfo.screen_width, h=get_font_height_by_pad(),
        x=0, y=(cfg->taskbar_on_top ? 0 : xinfo.screen_height-h);

    taskbar_new(NULL, x, y, w, h);
    if(cfg->show_taskbar)
        widget_show(WIDGET(get_taskbar()));
}

void create_cmd_entry(Widget_id id)
{
    int sw=xinfo.screen_width, sh=xinfo.screen_height, bw=cfg->border_width,
        x, y, w, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(cfg->cmd_entry_hint, &w, NULL);
    w += 2*pad, w = (w>=sw/4 && w<=sw-2*bw) ? w : sw/4;
    x=(sw-w)/2-bw, y=(sh-h)/2-bw;

    cmd_entry=entry_new(NULL, id, x, y, w, h, cfg->cmd_entry_hint,
        entry_get_cmd_completion);
    listview_set_nmax(entry_get_listview(cmd_entry), (sh-y-h-h)/h);
    widget_set_poppable(WIDGET(cmd_entry), true);
}

static Strings *entry_get_cmd_completion(Entry *entry)
{
    char *regex=entry_get_part_match_regex(entry);
    char *paths=getenv("PATH");
    Strings *cmds=get_files_in_paths(paths, regex, false);
    Free(regex);
    return cmds;
}

void create_color_entry(Widget_id id)
{
    int sw=xinfo.screen_width, sh=xinfo.screen_height, bw=cfg->border_width,
        x, y, w, h=get_font_height_by_pad(), pad=get_font_pad();

    get_string_size(cfg->color_entry_hint, &w, NULL);
    w += 2*pad, w = (w>=sw/4 && w<=sw-2*bw) ? w : sw/4;
    x=(sw-w)/2-bw, y=(sh-h)/2-bw;

    color_entry=entry_new(NULL, id, x, y, w, h, cfg->color_entry_hint, NULL);
    widget_set_poppable(WIDGET(color_entry), true);
}

static char *entry_get_part_match_regex(Entry *entry)
{
    char text[FILENAME_MAX]={0};
    wcstombs(text, entry_get_text(entry), FILENAME_MAX);
    return copy_strings(text, ".*", NULL);
}

void open_color_settings(void)
{
    entry_clear(color_entry);
    entry_show(WIDGET(color_entry));
}
    
void open_run_cmd(void)
{
    entry_clear(cmd_entry);
    entry_show(WIDGET(cmd_entry));
}

void key_set_color(XKeyEvent *e)
{
    if(!entry_input(color_entry, e))
        return;

    char color[BUFSIZ]={0};
    wcstombs(color, entry_get_text(color_entry), BUFSIZ);
    set_main_color_name(color);
    update_gui();
    entry_clear(color_entry);
}

void key_run_cmd(XKeyEvent *e)
{
    if(!entry_input(cmd_entry, e))
        return;

    char cmd[BUFSIZ]={0};
    wcstombs(cmd, entry_get_text(cmd_entry), BUFSIZ);
    exec_cmd(SH_CMD(cmd));
    entry_clear(cmd_entry);
}

void update_gui(void)
{
    // 以下函數會產生Expose事件，而處理Expose事件時會更新窗口的文字
    // 內容及其顏色，故此處不必更新構件文字顏色。
    char *name=get_main_color_name();

    alloc_color(name);
    Free(name);
    update_all_widget_bg();
    if(cfg->wallpaper_filename == NULL)
        update_win_bg(xinfo.root_win, get_root_color(), None);
}
