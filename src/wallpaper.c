/* *************************************************************************
 *     wallpaper.c：實現壁紙功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <time.h>
#include <Imlib2.h>
#include "config.h"
#include "color.h"
#include "drawable.h"
#include "ewmh.h"
#include "file.h"
#include "misc.h"
#include "wallpaper.h"

static Pixmap create_pixmap_from_file(Window win, const char *filename);

static Strings *wallpapers=NULL, *cur_wallpaper=NULL; // 壁紙文件列表、当前壁纸文件

void init_wallpaper(void)
{
    if(cfg->wallpaper_paths == NULL)
        return;

    const char *paths=cfg->wallpaper_paths, *reg="*.png|*.jpg|*.svg|*.webp";
    wallpapers=get_files_in_paths(paths, reg, true);
    cur_wallpaper=list_first_entry(&wallpapers->list, Strings, list);
}

void set_default_wallpaper(void)
{
    const char *name=cfg->wallpaper_filename;

    Pixmap pixmap=create_pixmap_from_file(xinfo.root_win, name ? name : "");
    update_win_bg(xinfo.root_win, get_root_color(), pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}

static Pixmap create_pixmap_from_file(Window win, const char *filename)
{
    int w, h;
    unsigned int d;
    Imlib_Image image=imlib_load_image(filename);

    if(!image || !get_geometry(win, NULL, NULL, &w, &h, NULL, &d))
        return None;

    Pixmap bg=XCreatePixmap(xinfo.display, win, w, h, d);
    set_visual_for_imlib(win);
    imlib_context_set_image(image);
    imlib_context_set_drawable(bg);   
    imlib_render_image_on_drawable_at_size(0, 0, w, h);
    imlib_free_image();
    return bg;
}

void switch_to_next_wallpaper(void)
{
    srand((unsigned int)time(NULL));
    unsigned long r1=rand(), r2=rand(), color=(r1<<16)|r2|0xff000000UL;
    Pixmap pixmap=None;

    if(cfg->wallpaper_paths)
    {
        pixmap=create_pixmap_from_file(xinfo.root_win, cur_wallpaper->str);
        cur_wallpaper=list_next_entry(cur_wallpaper, Strings, list);
        if(list_entry_is_head(cur_wallpaper, &wallpapers->list, list))
            cur_wallpaper=list_next_entry(cur_wallpaper, Strings, list);
    }

    update_win_bg(xinfo.root_win, color, pixmap);
    if(pixmap && !have_compositor())
        XFreePixmap(xinfo.display, pixmap);
}

void free_wallpapers(void)
{
    vfree_strings(wallpapers);
}
