/* *************************************************************************
 *     image.c：實現與圖標映像相關的功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "gwm.h"
#include "list.h"

typedef struct image_node_tag
{
    char *name;
    Imlib_Image image;
    List list;
} Image_node;

typedef struct // 存儲圖標主題規範所說的Per-Directory Keys的結構
{
    int size, scale, max_size, min_size, threshold;
    char type[10]; // char context[32]; 目前用不上
} Icon_dir_info;

static Image_node *create_image_node(const char *name, Imlib_Image image);
static void free_image_node(Image_node *node);
static void reg_image(const char *name, Imlib_Image image);
static Imlib_Image search_icon_image(const char *name);
static Imlib_Image create_icon_image(Window win, const char *name, int size, const char *theme);
static Imlib_Image create_icon_image_from_hint(Window win);
static Imlib_Image create_icon_image_from_prop(Window win);
static Imlib_Image create_icon_image_from_file(const char *name, int size, const char *theme);
static char *find_icon(const char *name, int size, int scale, const char *theme, const char *context_dir);
static char *find_icon_helper(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir);
static char *lookup_icon(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir);
static bool get_closest_icon(const char *name, int size, int scale, const char *base_dir, const char *sub_dir, const char *theme, char **closest, int *min_distance);
static char *lookup_fallback_icon(const char *name, char *const *base_dirs);
static bool is_dir_match_size(const char *base_dir, const char *theme, const char *sub_dir, int size, int scale);
static int get_dir_size_distance(const char *base_dir, const char *theme, const char *sub_dir, int size, int scale);
static char **get_base_dirs(void);
static char **get_sub_dirs(const char *base_dir, const char *theme, const char *context_dir);
static char **get_list_val_from_index_theme(const char *base_dir, const char *theme, const char *key, const char *filter);
static char *grep_index_theme(const char *base_dir, const char *theme, const char *key, char *buf, size_t size);
static bool get_dir_info_from_index_theme(const char *base_dir, const char *theme, const char *sub_dir, Icon_dir_info *info);
static bool get_icon_dir_info_from_buf(char *buf, Icon_dir_info *info);
static void fix_icon_dir_info(Icon_dir_info *info);
static FILE *open_index_theme(const char *base_dir, const char *theme);
static size_t get_spec_char_num(const char *str, int ch);
static char **get_parent_themes(const char *base_dir, const char *theme);

static Image_node *image_list=NULL;

void free_all_images(void)
{
    if(!image_list)
        return;

    list_for_each_entry_safe(Image_node, p, &image_list->list, list)
        free_image_node(p);
    vfree(image_list);
}

void free_image(Imlib_Image image)
{
    if(!image_list)
        return;

    list_for_each_entry_safe(Image_node, p, &image_list->list, list)
        if(p->image == image)
            free_image_node(p);
}

static void free_image_node(Image_node *node)
{
    vfree(node->name);
    imlib_context_set_image(node->image);
    imlib_free_image();
    list_del(&node->list);
    vfree(node);
}

static void reg_image(const char *name, Imlib_Image image)
{
    if(!image_list)
    {
        image_list=create_image_node(NULL, NULL);
        list_init(&image_list->list);
    }
    Image_node *p=create_image_node(name, image);
    list_add(&p->list, &image_list->list);
}

static Image_node *create_image_node(const char *name, Imlib_Image image)
{
    Image_node *p=malloc_s(sizeof(Image_node));
    p->name=copy_string(name);
    p->image=image;
    return p;
}

void draw_image(Imlib_Image image, Drawable d, int x, int y, int w, int h)
{
    XClearArea(xinfo.display, d, x, y, w, h, False); 
    set_visual_for_imlib(d);
    imlib_context_set_image(image);
    imlib_context_set_drawable(d);   
    imlib_render_image_on_drawable_at_size(x, y, w, h);
}

Imlib_Image get_icon_image(Window win, const char *name, int size, const char *theme)
{
    Imlib_Image image=search_icon_image(name);

    return image ? image : create_icon_image(win, name, size, theme);
}

static Imlib_Image search_icon_image(const char *name)
{
    if(!image_list)
        return NULL;

    list_for_each_entry(Image_node, p, &image_list->list, list)
        if(strcmp(p->name, name) == 0)
            return p->image;

    return NULL;
}

static Imlib_Image create_icon_image(Window win, const char *name, int size, const char *theme)
{
    Imlib_Image image=NULL;

    /* 根據加載效率依次嘗試 */
    if( (image=create_icon_image_from_hint(win))
        || (image=create_icon_image_from_prop(win))
        || (image=create_icon_image_from_file(name, size, theme)))
    {
        reg_image(name, image);
        return image;
    }

    return NULL;
}

static Imlib_Image create_icon_image_from_hint(Window win)
{
    XWMHints *hint=XGetWMHints(xinfo.display, win);
    if(!hint || !(hint->flags & IconPixmapHint))
        return NULL;

    int w, h;
    Pixmap pixmap=hint->icon_pixmap, mask=hint->icon_mask;
    XFree(hint);
    if(!get_geometry(pixmap, NULL, NULL, &w, &h, NULL, NULL))
        return NULL;
    imlib_context_set_drawable(pixmap);   
    return imlib_create_image_from_drawable(mask, 0, 0, w, h, 0);
}

static Imlib_Image create_icon_image_from_prop(Window win)
{
    CARD32 *data=get_net_wm_icon(win);
    if(!data)
        return NULL;
    
    CARD32 w=data[0], h=data[1], size=w*h, i;
    Imlib_Image image=imlib_create_image(w, h);
    imlib_context_set_image(image);
    imlib_image_set_has_alpha(1);
    /* imlib2和_NET_WM_ICON同樣使用大端字節序，因此不必轉換字節序 */
    DATA32 *image_data=imlib_image_get_data();
    for(i=0; i<size; i++)
        image_data[i]=data[i+2];
    vfree(data);
    imlib_image_put_back_data(image_data);
    return image;
}

static Imlib_Image create_icon_image_from_file(const char *name, int size, const char *theme)
{
    if(!name || size<=0 || !theme)
        return NULL;

    char *fn=find_icon(name, size, 1, theme, "apps");
    return fn ? imlib_load_image(fn) : NULL;
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

static char *find_icon(const char *name, int size, int scale, const char *theme, const char *context_dir)
{
    char *filename=NULL, **base_dirs=get_base_dirs();

    // 規範建議先找基本目錄，然後找hicolor，最後找後備目錄
    if( (filename=find_icon_helper(name, size, scale, base_dirs, theme, context_dir))
        || (filename=find_icon_helper(name, size, scale, base_dirs, "hicolor", context_dir))
        || (filename=lookup_fallback_icon(name, base_dirs)))
    {
        for(char **b=base_dirs; b&&*b; b++)
            vfree(*b);
        vfree(base_dirs);
    }
    return filename;
}

static char *find_icon_helper(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir)
{
    char *filename=NULL;

    if((filename=lookup_icon(name, size, scale, base_dirs, theme, context_dir)))
        return filename;

    // 規範建議在給定主題中找不到匹配的圖標時，遞歸搜索其父主題列表
    for(char *const *b=base_dirs; b&&*b; b++)
    {
        char **parent=get_parent_themes(*b, theme);
        for(char **p=parent; p&&*p; p++)
            if((filename=find_icon_helper(name, size, scale, b, *p, context_dir)))
                { vfree(parent); return filename; }
        vfree(parent);
    }

    return NULL;
}

static char *lookup_icon(const char *name, int size, int scale, char *const *base_dirs, const char *theme, const char *context_dir)
{
    int min=INT_MAX;
    char *closest=NULL;

    for(char *const *b=base_dirs; b&&*b; b++)
    {
        char **sub_dirs=get_sub_dirs(*b, theme, context_dir);
        for(char **s=sub_dirs; s&&*s; s++)
            if(get_closest_icon(name, size, scale, *b, *s, theme, &closest, &min))
                { vfree(sub_dirs); return closest; }
        vfree(sub_dirs);
    }
    return closest;
}

/* 規範建議先搜索完全匹配的圖標，然後搜索尺寸最接近的圖標。
 * 當直接找到完全匹配的圖標時返回真值，否則返回假值。 */
static bool get_closest_icon(const char *name, int size, int scale, const char *base_dir, const char *sub_dir, const char *theme, char **closest, int *min_distance)
{
    char *f=NULL;
    for(size_t i=0; i<ARRAY_NUM(ICON_EXT); i++)
    {
        f=copy_strings(base_dir, "/", theme, "/", sub_dir, "/", name, ICON_EXT[i], NULL);
        if(is_accessible(f))
        {
            if(is_dir_match_size(base_dir, theme, sub_dir, size, scale))
                return (*closest=f);

            int d=get_dir_size_distance(base_dir, theme, sub_dir, size, scale);
            if(d < *min_distance)
                *closest=f, *min_distance=d;
        }
        vfree(f);
    }

    return false;
}

static char *lookup_fallback_icon(const char *name, char *const *base_dirs)
{
    for(char *const *b=base_dirs; b&&*b; b++)
    {
        for(size_t i=0; i<ARRAY_NUM(ICON_EXT); i++)
        {
            // 規範規定後備圖標路徑爲：基本目錄/圖標名.擴展名
            char *filename=copy_strings(*b, "/", name, ICON_EXT[i], NULL);
            if(is_accessible(filename))
                return filename;
            vfree(filename);
        }
    }

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
         *x=getenv("XDG_DATA_DIRS"), // 注意環境變量可能未設置
         *xdg=copy_strings(x ? x : "/usr/share:/usr/local/share", NULL);
    size_t n=get_spec_char_num(xdg, ':')+4;

    // 規範規定依次搜索如下三個基本目錄：
    // $HOME/.icons、$XDG_DATA_DIRS/icons、/usr/share/pixmaps
    dirs=malloc_s(n*sizeof(char *));
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
    char **result=malloc_s(n*sizeof(char *));
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
        && get_icon_dir_info_from_buf(buf, info))
        ;
    fclose(fp);
    fix_icon_dir_info(info);
    return result;
}

static bool get_icon_dir_info_from_buf(char *buf, Icon_dir_info *info)
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
    vfree(filename);
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
