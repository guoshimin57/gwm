/* *************************************************************************
 *     icon.c：實現圖符化的相關功能。
 *     版權 (C) 2020-2022 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "client.h"
#include "desktop.h"
#include "font.h"
#include "icon.h"
#include "misc.h"

#if USE_IMAGE_ICON

// 存儲圖標主題規範所說的Per-Directory Keys的結構
struct icon_dir_info_tag
{
    int size, scale, max_size, min_size, threshold;
    char type[10]; // char context[32]; 目前用不上
};
typedef struct icon_dir_info_tag Icon_dir_info;

static void draw_icon_image(WM *wm, Client *c);
static void draw_image(WM *wm, Imlib_Image image, Drawable d, int x, int y, unsigned int w, unsigned int h);
static void set_icon_image(WM *wm, Client *c);
static Imlib_Image get_icon_image_from_hint(WM *wm, Client *c);
static Imlib_Image get_icon_image_from_prop(WM *wm, Client *c);
static Imlib_Image get_icon_image_from_file(WM *wm, Client *c);
static char *find_icon(const char *name, int size, int scale, const char *context_dir);
static char *find_icon_helper(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir);
static char *lookup_icon(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir);
static char *lookup_fallback_icon(const char *name, char *const *base_dirs);
static bool is_dir_match_size(const char *base_dir, const char *theme, const char *sub_dir, int size, int scale);
static int get_dir_size_distance(const char *base_dir, const char *theme, const char *sub_dir, int size, int scale);
static char **get_base_dirs(void);
static char **get_sub_dirs(const char *base_dir, const char *theme, const char *context_dir);
static char **get_list_val_from_index_theme(const char *base_dir, const char *theme, const char *key, const char *filter);
static char *grep_index_theme(const char *base_dir, const char *theme, const char *key, char *buf, size_t size);
static bool get_dir_info_from_index_theme(const char *base_dir, const char *theme, const char *sub_dir, Icon_dir_info *info);
static bool get_icon_dir_info_from_buf(char *buf, size_t size, Icon_dir_info *info);
static void fix_icon_dir_info(Icon_dir_info *info);
static FILE *open_index_theme(const char *base_dir, const char *theme);
static size_t get_spec_char_num(const char *str, int ch);
static char **get_parent_themes(const char *base_dir, const char *theme);
static bool is_accessible(const char *filename);

static void draw_icon_image(WM *wm, Client *c)
{
    if(c && c->icon && c->image)
        draw_image(wm, c->image, c->icon->win, 0, 0, ICON_SIZE, ICON_SIZE);
}

static void draw_image(WM *wm, Imlib_Image image, Drawable d, int x, int y, unsigned int w, unsigned int h)
{
    imlib_context_set_image(image);
    imlib_context_set_drawable(d);   
    imlib_render_image_on_drawable_at_size(x, y, w, h);
}

static void set_icon_image(WM *wm, Client *c)
{
    /* 根據加載效率依次嘗試 */
    if( c && !c->image
        && !( (c->image=get_icon_image_from_hint(wm, c))
        || (c->image=get_icon_image_from_prop(wm, c))
        || (c->image=get_icon_image_from_file(wm, c))))
        c->image=NULL;
}

static Imlib_Image get_icon_image_from_hint(WM *wm, Client *c)
{
    if(c->wm_hint && (c->wm_hint->flags & IconPixmapHint))
    {
        unsigned int w, h, d;
        Pixmap pixmap=c->wm_hint->icon_pixmap, mask=c->wm_hint->icon_mask;
        if(!get_geometry(wm, pixmap, &w, &h, &d))
            return NULL;
        imlib_context_set_drawable(pixmap);   
        return imlib_create_image_from_drawable(mask, 0, 0, w, h, 0);
    }
    return NULL;
}

static Imlib_Image get_icon_image_from_prop(WM *wm, Client *c)
{
    unsigned long i, n=0, w=0, h=0, *data=NULL;
    unsigned char *p=get_prop(wm, c->win, wm->ewmh_atom[_NET_WM_ICON], &n);
    if(!p)
        return NULL;
    
    data=(unsigned long *)p;
    w=data[0], h=data[1], data+=2;
    Imlib_Image image=imlib_create_image(w, h);
    imlib_context_set_image(image);
    imlib_image_set_has_alpha(1);
    /* imlib2和_NET_WM_ICON同樣使用大端字節序，因此不必轉換字節序 */
    DATA32 *image_data=imlib_image_get_data();
    /* 當long大小爲8字節時，以long型數組存儲的特性數據，每個元素的前4字節都是填充0 */
    if(sizeof(long) == 8)
        for(i=0; i<n; i++)
            image_data[i]=data[i]; // 跳過填充字節
    else
        memcpy(image_data, (unsigned char *)data, w*h*4);
    XFree(p);
    imlib_image_put_back_data(image_data);
    return image;
}

static Imlib_Image get_icon_image_from_file(WM *wm, Client *c)
{
    char *filename=find_icon(c->class_hint.res_name, ICON_SIZE, 1, "apps");
    return filename ? imlib_load_image(filename) : NULL;
}

/* 根據圖標名稱、尺寸、縮放比例和規範中context對應的目錄名來搜索圖標文件全名。
 * context_dir的取值參見以下規範中的Directory列：
 * specifications.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html
 * 當context_dir爲空指針時，即爲通配。
 * 以下尋找圖標文件的算法參考《圖標主題規範》(以下簡稱規範，詳見：
 * specifications.freedesktop.org/icon-theme-spec/icon-theme-spec-latest.html) 
 * ，並修正其謬誤，及提高效率。規範對index.theme的格式提出詳細要求，實際上我所
 * 見過的所有這類文件都沒有不必要的空白符。爲簡單起見，本搜索圖標文件的算法也不
 * 考慮其存在多餘空白符。*/

#define ICON_BUF_SIZE (1<<20)
// 目前圖標主題規範只支持這三種格式的圖標
#define ICON_EXT (const char *[]){".png", ".svg", ".xpm"}
// index.theme文件中的目錄段鍵名，即圖標主題規範所說的Per-Directory Keys
#define ICON_THEME_PER_DIR_KEYS (const char *[]){"Size=", "Scale=", "MaxSize=", "MinSize=", "Threshold=", "Type=", "Context="}

static char *find_icon(const char *name, int size, int scale, const char *context_dir)
{
    char *filename=NULL, **base_dirs=get_base_dirs();

    // 規範建議先找基本目錄，然後找hicolor，最後找後備目錄
    if( (filename=find_icon_helper(name, size, scale, base_dirs, CUR_ICON_THEME, context_dir))
        || (filename=find_icon_helper(name, size, scale, base_dirs, "hicolor", context_dir))
        || (filename=lookup_fallback_icon(name, base_dirs)))
    {
        for(char **b=base_dirs; b&&*b; b++)
            free(*b);
        free((void *)base_dirs);
    }
    return filename;
}

static char *find_icon_helper(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir)
{
    char *filename=NULL, *const *b=NULL, *const *p=NULL, *const *backup=NULL;

    if((filename=lookup_icon(name, size, scale, base_dirs, theme, context_dir)))
        return filename;
    // 規範建議在給定主題中找不到匹配的圖標時，遞歸搜索其父主題列表
    for(b=base_dirs; b&&*b; b++, free((void *)backup))
        for(backup=p=get_parent_themes(*b, theme); p&&*p; free(*p++))
            if((filename=find_icon_helper(name, size, scale, b, *p, context_dir)))
                { free((void *)backup); return filename; }
    return NULL;
}

static char *lookup_icon(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir)
{
    int min=INT_MAX, d=0;
    char *filename=NULL, *closest_filename=NULL;
    char *const *b=NULL, *const *s=NULL, *const *backup=NULL;

    // 規範建議先搜索完全匹配的圖標，然後搜索尺寸最接近的圖標
    for(b=base_dirs; b&&*b; b++, free((void *)backup))
        for(backup=s=get_sub_dirs(*b, theme, context_dir); s&&*s; free(*s++))
            for(size_t i=0, n=ARRAY_NUM(ICON_EXT); i<n; i++)
                if(is_accessible(filename=copy_strings(*b, "/", theme, "/",
                    *s, "/", name, ICON_EXT[i], NULL)))
                {
                    if(is_dir_match_size(*b, theme, *s, size, scale))
                        { free((void *)backup); return filename; }
                    if((d=get_dir_size_distance(*b, theme, *s, size, scale)) < min)
                        closest_filename=filename, min=d;
                }
    if(closest_filename)
        return closest_filename;
    return NULL;
}

static char *lookup_fallback_icon(const char *name, char *const *base_dirs)
{
    char *filename=NULL;
    // 規範規定後備圖標路徑爲：基本目錄/圖標名.擴展名
    for(char *const *b=base_dirs; b&&*b; b++)
        for(size_t i=0, n=ARRAY_NUM(ICON_EXT); i<n; i++)
            if((filename=copy_strings(*b, "/", name, ICON_EXT[i], NULL))
                && is_accessible(filename))
                return filename;
    return NULL;
}

static bool is_dir_match_size(const char *base_dir, const char *theme, const char *sub_dir, int size, int scale)
{
    Icon_dir_info info;
    if( !get_dir_info_from_index_theme(base_dir, theme, sub_dir, &info)
        || info.scale != scale)
        return false;
    if(strcmp(info.type, "Fixed") == 0)
        return info.size==size;
    if(strcmp(info.type, "Scaled") == 0)
        return info.min_size<=size && size<=info.max_size;
    if(strcmp(info.type, "Threshold") == 0)
        return info.size-info.threshold<=size && size<=info.size+info.threshold;
    return false;
}

static int get_dir_size_distance(const char *base_dir, const char *theme, const char *sub_dir, int size, int scale)
{
    Icon_dir_info info;
    if(!get_dir_info_from_index_theme(base_dir, theme, sub_dir, &info))
        return 0;
    if(strcmp(info.type, "Fixed") == 0)
        return abs(info.size*info.scale-size*scale);
    if(strcmp(info.type, "Scaled") == 0)
    {
        if(size*scale < info.min_size*info.scale)
            return info.min_size*info.scale-size*scale;
        if(size*scale > info.max_size*info.scale)
            return size*scale-info.max_size*info.scale;
        return 0;
    }
    if(strcmp(info.type, "threshold") == 0)
    {
        if(size*scale < (info.size-info.threshold)*info.scale)
            return info.min_size*info.scale-size*scale;
        if(size*scale > (info.size+info.threshold)*info.scale)
            return size*scale-info.max_size*info.scale;
        return 0;
    }
    return INT_MAX;
}

static char **get_base_dirs(void)
{
    char **dirs=NULL, *home=getenv("HOME"), *pix="/usr/share/pixmaps",
          *xdg=copy_strings(getenv("XDG_DATA_DIRS"), NULL);
    int n=get_spec_char_num(xdg, ':')+4;

    // 規範規定依次搜索如下三個基本目錄：
    // $HOME/.icons、$XDG_DATA_DIRS/icons、/usr/share/pixmaps
    dirs=malloc(n*sizeof(char *));
    dirs[0]=copy_strings(home, "/.icons", NULL);
    dirs[n-2]=copy_strings(pix, NULL);
    dirs[n-1]=NULL;
    for(size_t len=0, i=1; i<n-2; i++, xdg+=len+1)
    {
        xdg[len=strcspn(xdg, ":")]='\0';
        dirs[i]=copy_strings(xdg, "/icons", NULL);
    }
    return dirs;
}

static char **get_sub_dirs(const char *base_dir, const char *theme, const char *context_dir)
{
    return get_list_val_from_index_theme(base_dir, theme, "Directories=", context_dir);
}

static char **get_list_val_from_index_theme(const char *base_dir, const char *theme, const char *key, const char *filter)
{
    char buf[ICON_BUF_SIZE], *val=NULL;
    if(!grep_index_theme(base_dir, theme, key, buf, ICON_BUF_SIZE))
        return NULL;
    if((val=strchr(buf, '=')+1) == NULL)
        return NULL;

    size_t len, i, n=get_spec_char_num(val, ',')+2;
    char **result=malloc(n*sizeof(char *));
    for(len=0, i=0; i<n-1 && *val; val+=len+1)
    {
        val[len=strcspn(val, ",")]='\0';
        if(!filter || strstr(val, filter))
            result[i++]=copy_strings(val, NULL);
    }
    result[i]=NULL;
    return filter ? realloc(result, ++i*sizeof(char *)) : result;
}

static char *grep_index_theme(const char *base_dir, const char *theme, const char *key, char *buf, size_t size)
{
    FILE *fp=open_index_theme(base_dir, theme);
    if(fp == NULL)
        return NULL;

    char *p=NULL;
    while(!feof(fp) && (!fgets(buf, size, fp) || !(p=strstr(buf, key))))
        ;
    fclose(fp);
    if(p == NULL)
        return NULL;

    if((p=strrchr(buf, '\n')))
        *p='\0';
    return buf;
}

static bool get_dir_info_from_index_theme(const char *base_dir, const char *theme, const char *sub_dir, Icon_dir_info *info)
{
    FILE *fp=open_index_theme(base_dir, theme);
    if(fp == NULL)
        return false;

    bool result=false;
    char buf[ICON_BUF_SIZE];
    memset(info, 0, sizeof(Icon_dir_info));
    // 先跳過目錄段標題，標題以'['開頭，以子目錄名過濾時應跳過它
    while( !feof(fp)
        && (!fgets(buf, ICON_BUF_SIZE, fp) || strstr(buf, sub_dir)!=buf+1))
        ;
    while( (result=!feof(fp)) && fgets(buf, ICON_BUF_SIZE, fp)
        && get_icon_dir_info_from_buf(buf, ICON_BUF_SIZE, info))
        ;
    fclose(fp);
    fix_icon_dir_info(info);
    return result;
}

static bool get_icon_dir_info_from_buf(char *buf, size_t size, Icon_dir_info *info)
{
    for(size_t i=0, n=ARRAY_NUM(ICON_THEME_PER_DIR_KEYS); i<n; i++)
    {
        if(strstr(buf, ICON_THEME_PER_DIR_KEYS[i]) == buf)
        {
            char *val=strchr(buf, '=')+1;
            *strrchr(val, '\n')='\0';
            switch(i)
            {
                case 0: info->size=atoi(val); return true;
                case 1: info->scale=atoi(val); return true;
                case 2: info->max_size=atoi(val); return true;
                case 3: info->min_size=atoi(val); return true;
                case 4: info->threshold=atoi(val); return true;
                case 5: strcpy(info->type, val); return true;
                default: return true; // 也就是info->context，目前用不上
            }
        }
    }
    return false;
}

static void fix_icon_dir_info(Icon_dir_info *info)
{
    if(info->scale == 0)
        info->scale=1;
    if(info->max_size==0)
        info->max_size=info->size;
    if(info->min_size == 0)
        info->min_size=info->size;
    if(info->threshold == 0)
        info->threshold=2;
    if(info->type[0] == '\0')
        strcpy(info->type, "Threshold");
}

static FILE *open_index_theme(const char *base_dir, const char *theme)
{
    // 規範約定的主題文件全名爲：基本目錄/主題/index.theme
    char *filename=copy_strings(base_dir, "/", theme, "/index.theme", NULL);
    if(filename == NULL)
        return NULL;

    FILE *fp=fopen(filename, "r");
    free(filename);
    return fp;
}

static size_t get_spec_char_num(const char *str, int ch)
{
    size_t n=0;
    if(str)
        for(const char *p=str; *p; p++)
            if(*p == ch)
                n++;
    return n;
}

static char **get_parent_themes(const char *base_dir, const char *theme)
{
    return get_list_val_from_index_theme(base_dir, theme, "Inherits=", NULL);
}

static bool is_accessible(const char *filename)
{
    if(filename == NULL)
        return false;

    struct stat buf;
    return !stat(filename, &buf);
}

#endif

static void create_icon(WM *wm, Client *c);
static bool have_same_class_icon_client(WM *wm, Client *c);

void iconify(WM *wm, Client *c)
{
    create_icon(wm, c);
    XMapWindow(wm->display, c->icon->win);
    XUnmapWindow(wm->display, c->frame);
    if(c == DESKTOP(wm).cur_focus_client)
    {
        focus_client(wm, wm->cur_desktop, NULL);
        update_frame(wm, wm->cur_desktop, c);
    }
}

static void create_icon(WM *wm, Client *c)
{
    Icon *p=c->icon=malloc_s(sizeof(Icon));
    p->w=p->h=ICON_SIZE;
    p->x=0, p->y=wm->taskbar.h/2-p->h/2-ICON_BORDER_WIDTH;
    p->area_type=c->area_type==ICONIFY_AREA ? DEFAULT_AREA_TYPE : c->area_type;
    c->area_type=ICONIFY_AREA;
    p->win=XCreateSimpleWindow(wm->display, wm->taskbar.icon_area, p->x, p->y,
        p->w, p->h, ICON_BORDER_WIDTH,
        wm->widget_color[NORMAL_BORDER_COLOR].pixel,
        wm->widget_color[ICON_COLOR].pixel);
    XSelectInput(wm->display, c->icon->win, ICON_WIN_EVENT_MASK);
#if USE_IMAGE_ICON
    set_icon_image(wm, c);
#endif
    p->title_text=get_text_prop(wm, c->win, XA_WM_ICON_NAME);
    update_icon_area(wm);
}

void update_icon_area(WM *wm)
{
    unsigned int x=0, w=0;
    for(Client *c=wm->clients->prev; c!=wm->clients; c=c->prev)
    {
        if(is_on_cur_desktop(wm, c) && c->area_type==ICONIFY_AREA)
        {
            Icon *i=c->icon;
            i->w=MIN(get_icon_draw_width(wm, c), ICON_WIN_WIDTH_MAX);
            if(have_same_class_icon_client(wm, c))
            {
                get_string_size(wm, wm->font[TITLE_FONT], i->title_text, &w, NULL);
                i->w=MIN(i->w+w, ICON_WIN_WIDTH_MAX);
                i->is_short_text=false;
            }
            else
                i->is_short_text=true;
            i->x=x;
            x+=i->w+ICONS_SPACE;
            XMoveResizeWindow(wm->display, i->win, i->x, i->y, i->w, i->h); 
        }
    }
}

unsigned int get_icon_draw_width(WM *wm, Client *c)
{
#if USE_IMAGE_ICON
    return ICON_SIZE;
#else
    unsigned int w=0;
    get_string_size(wm, wm->font[ICON_CLASS_FONT], c->class_name, &w, NULL);
    return w;
#endif
}

void draw_icon(WM *wm, Client *c)
{
    Icon *i=c->icon;
    String_format f={{0, 0, i->w, i->h}, CENTER_LEFT, false, 0,
        wm->text_color[CLASS_TEXT_COLOR], CLASS_FONT};
#if USE_IMAGE_ICON
    if(c->image)
        draw_string(wm, i->win, "", &f), draw_icon_image(wm, c);
    else
        draw_string(wm, i->win, c->class_name, &f);
#else
    draw_string(wm, i->win, c->class_name, &f);
#endif
}

static bool have_same_class_icon_client(WM *wm, Client *c)
{
    for(Client *p=wm->clients->next; p!=wm->clients; p=p->next)
        if( p!=c && is_on_cur_desktop(wm, p) && p->area_type==ICONIFY_AREA
            && !strcmp(c->class_name, p->class_name))
            return true;
    return false;
}

void deiconify(WM *wm, Client *c)
{
    if(c)
    {
        XMapWindow(wm->display, c->frame);
        del_icon(wm, c);
        focus_client(wm, wm->cur_desktop, c);
    }
}

void del_icon(WM *wm, Client *c)
{
    XDestroyWindow(wm->display, c->icon->win);
    c->area_type=c->icon->area_type;
    free(c->icon->title_text);
    free(c->icon);
    update_icon_area(wm);
}
