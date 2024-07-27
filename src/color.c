/* *************************************************************************
 *     color.c：實現顏色相關功能。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

/* 界面配色原理：
 *     色相環中紫紅至黃色的半圓區域是暖色，黃綠至紫色的半圓區域是冷色。越靠近
*  紅色越暖，越靠近藍色越冷。暖色系中飽和度較高的顏色稱爲興奮色，冷色系中飽和
*  度較低的顏色稱爲安靜色。暖色會讓用戶產生積極、喜慶、食欲、親近的感受，而冷
*  色會讓用戶產生正義、平靜、安全、理智、高科技的感受。
 * 彩色內容一般都應是可以點擊。標題宜用深灰色，但正文宜用較淺的灰色。
 *     通過利用同色系色相差來區分界面中不同類型的內容，通過色彩強弱對比突出關
 * 鍵內容。互補色是色相環中呈180°的兩種顏色，同類色是同一色相中不同傾向的系列
 * 顏色，對比色是色相環中呈120°~150°的顏色。使用同類色對比的優點是：可以營造
 * 出和諧統一的界面效果；缺點是：如果界面僅有同類色就顯得單調乏味。使用互補色
 * 對比的優點是：讓畫面更具張力，營造出視覺反差，吸引用戶關注；缺點是：若搭配
 * 得不好，容易使界面俗氣或刺眼。將亮度相同、飽和度很高的等量互補色搭配在一起，
 * 可以使界面鮮明且不刺眼。使用飽和度對比的優點是：可以讓界面分清主次，飽和度
 * 越高越引人注目；缺點是：若飽和度過高，尤其是多種高飽和度顏色同時出現，會讓
 * 人看着不舒服，若飽和度過低，則容易讓界面髒亂和沉悶。飽和度對比越強，界面越
 * 明朗且富有生氣，界面衝擊力越強。飽和度越弱，界面衝擊力越弱，界面效果較含蓄，
 * 適合長時間和近距離觀看。前景宜用高飽和度，背景宜用低飽和度，以此來強化視覺
 * 上的前後關系。使用亮度對比，可以展現出色彩的層次感。亮度越高表示越靠前，亮
 * 度越低表示越靠後。
 *     使用顏色面積對比，會讓視覺效果豐富多彩。通常，較耀眼的彩色面積佔比小，
 * 較柔和的彩色面積面積佔比大。
 *     使用動靜對比，可以緩解視覺疲勞。花哨的顏色佔比小，純色佔比高，兩者佔比
 * 應根據界面風格確定。
 *     顏色的心理重量由重到輕依次爲：黑、紅、紫、藍、綠、黃、白。另外，同色相時，
 * 亮度越高，感覺越輕。
 *     主色是界面給用戶留下第一印象的顏色，除黑色和少數深色以外，主色的飽和度和
 * 亮度宜高，但過高會刺眼。當界面中需要提示的內容不止一種時，可用輔助色區分。當
 * 主題色佔比過大時，使用輔助色平衡視覺。當需要區分的內容各類較多，主色和輔助色
 * 滿足不了時，可使用點綴色區分。當有的內容需要特別強調時，也可以使用點綴色。當
 * 主色和輔助時都同屬冷色或暖色時，可使用點綴色平衡。中性色主要用于文字、背景、
 * 分隔線。中性色通常使用主色色相，並提高亮度和降低飽和度，即亮度與飽和度成反比：
 * b=k/s。从HSB拾色器可以明顯感受到，當b=0.1且s=1時，是中性色，代入上式可求得
 * k=0.1。并且在b=0.1/s曲線下方的顏色也是中性色。當s<0.1時，b>1，故此時b應取1。
 *     顏色應用場景：主色約點60%，輔助色約佔30%，點綴色約點10%。不計中性色，除非
 * 純中性色配色。彩色一般用在按鈕、圖標、提示性元素上，中性色一般用在字體、分界線
 * 和背景上。背景與文字保持適當的對比度，不但可以提高文字的辨識度，還可以降低視覺
 * 疲勞。對比度可通過視覺亮度差和色差來計算。視覺亮度=R*0.299+G*0.587+B*0.114。
 * 亮度差=高亮度-低亮度。色差=|R1-R2|+|G1-G2|+|B1-B2|。W3C推薦亮度差超過125、色差
 * 超過500的顏色搭配。
 */

#include <float.h>
#include "config.h"
#include "gwm.h"

enum color_index // *[COLOR_N]數組下標索引，它與構件狀態相關
{
    STATE_NORMAL, 
    STATE_DISABLE, 
    STATE_ACTIVE, 
    STATE_WARN, 
    STATE_HOT, 
    STATE_URGENT, 
    STATE_ATTENT, 
    STATE_CHOSEN, 
    STATE_CURRENT, 
    STATE_DISABLE_CURRENT, 
    STATE_HOT_CHOSEN,
    STATE_LAST=STATE_HOT_CHOSEN
};

#define COLOR_N (STATE_LAST+1)

typedef enum // 顏色主題
{
    DARK_COLOR_THEME, LIGHT_COLOR_THEME,    // 彩色
    DARK_NEUTRAL_THEME, LIGHT_NEUTRAL_THEME,// 中性色
    DARK_GREY_THEME, LIGHT_GREY_THEME,      // 灰色
    LAST_COLOR_THEME=LIGHT_GREY_THEME
} Color_theme;

#define COLOR_THEME_N (LAST_COLOR_THEME+1)

typedef struct // 以下成員的值爲負數時表示該成員的值無法確定
{
    float h; // 色相[0-360）
    float s; // 飽和度[0-1]
    float b; // 亮度[0-1]
} HSB;

#define HSB(h, s, b) (HSB){(h), (s), (b)}

typedef struct
{
    int r; // 紅色[0-255]
    int g; // 綠色[0-255]
    int b; // 藍色[0-255]
} RGB;

#define RGB(r, g, b) (RGB){(r)+0.5, (g)+0.5, (b)+0.5} // 考慮四舍五入

// 以下分別是構件在深色和淺色主題下舒適彩色的飽和度s和亮度b的極值列表：
static const float cozy_s_min[]={0.70, 0.66}, cozy_s_max[]={0.80, 0.72};
static const float cozy_b_min[]={0.64, 0.60}, cozy_b_max[]={0.72, 0.70};

static unsigned long widget_color[COLOR_N]; // 構件顏色
static XftColor text_color[COLOR_N]; // 文本顏色
static unsigned long root_bg_color; // 根窗口背景色

static HSB get_main_hsb(const char *main_color_name);
static void fix_main_hsb_for_color(HSB *hsb);
static void fix_main_hsb_for_neutral(HSB *hsb);
static void fix_main_hsb_for_grey(HSB *hsb);
static float get_neutral_saturation(float brightness);
static void alloc_root_bg_color(HSB hsb);
static HSB get_root_bg_hsb(HSB hsb);
static void alloc_widget_colors(HSB hsb);
static void alloc_widget_color(Widget_state state, HSB hsb);
static RGB get_widget_rgb(Widget_state state, HSB hsb);
static RGB get_widget_rgb_on_color(Widget_state state, HSB hsb);
static RGB get_widget_rgb_on_neutral(Widget_state state, HSB hsb);
static RGB get_widget_rgb_on_grey(Widget_state state, HSB hsb);
static float get_cozy_s(HSB hsb);
static float get_cozy_b(HSB hsb);
static bool is_light_color(HSB hsb);
static XColor alloc_xcolor_by_rgb(RGB rgb);
static XftColor alloc_xftcolor_by_rgb(RGB rgb);
static size_t state_to_index(Widget_state state);
static Color_theme get_color_theme(HSB hsb);
static RGB get_text_rgb(Widget_state state, RGB rgb);
static float get_valid_dh(float h, float dh);
static bool is_valid_hue(float h);
static RGB xcolor_to_rgb(XColor xcolor);
static XColor rgb_to_xcolor(RGB rgb);
static float rgb_to_luminance(RGB rgb);
static HSB rgb_to_hsb(RGB rgb);
static RGB hsb_to_rgb(HSB hsb);

unsigned long get_root_bg_color(void)
{
    return root_bg_color;
}

unsigned long get_widget_color(Widget_state state)
{
    unsigned long alpha=0xff*cfg->widget_opacity;
    unsigned long rgb=widget_color[state_to_index(state)];

    return (rgb & 0x00ffffff) | alpha<<24;
}

void alloc_color(const char *main_color_name)
{
    HSB hsb=get_main_hsb(main_color_name);

    alloc_root_bg_color(hsb);
    alloc_widget_colors(hsb);
}

/* 取得界面主色hsb，即構件處於當前狀態時的背景色，文字和其他狀態下的顏色據此確定 */
static HSB get_main_hsb(const char *main_color_name)
{
    XColor xcolor;
    XParseColor(xinfo.display, xinfo.colormap, main_color_name, &xcolor); 
    HSB hsb=rgb_to_hsb(xcolor_to_rgb(xcolor));

    switch(get_color_theme(hsb))
    {
        case DARK_COLOR_THEME:
        case LIGHT_COLOR_THEME: fix_main_hsb_for_color(&hsb); break;
        case DARK_NEUTRAL_THEME:
        case LIGHT_NEUTRAL_THEME: fix_main_hsb_for_neutral(&hsb); break;
        case DARK_GREY_THEME:
        case LIGHT_GREY_THEME: fix_main_hsb_for_grey(&hsb); break;
    }

    return hsb;
}

static void fix_main_hsb_for_color(HSB *hsb)
{
    int i=is_light_color(*hsb);
    if(isless(hsb->s, cozy_s_min[i]))
        hsb->s=cozy_s_min[i];
    else if(isgreater(hsb->s, cozy_s_max[i]))
        hsb->s=cozy_s_max[i];
    if(isless(hsb->b, cozy_b_min[i]))
        hsb->b=cozy_b_min[i];
    else if(isgreater(hsb->b, cozy_b_max[i]))
        hsb->b=cozy_b_max[i];
}

// 因爲用不同顏色來反映構件狀態，故應預留必要的亮度差來生成有區分度的顏色
static void fix_main_hsb_for_neutral(HSB *hsb)
{
    float db=0.2, bmax=1-db, bmin=db;
    if(isgreater(hsb->b, bmax))
        hsb->b=bmax, hsb->s=get_neutral_saturation(hsb->b);
    else if(isless(hsb->b, bmin))
        hsb->b=bmin, hsb->s=get_neutral_saturation(hsb->b);
}

// 因爲用不同顏色來反映構件狀態，故應預留必要的亮度差來生成有區分度的顏色
static void fix_main_hsb_for_grey(HSB *hsb)
{
    float db=0.2, bmax=1-db, bmin=db;
    if(isgreater(hsb->b, bmax))
        hsb->b=bmax, hsb->s=0;
    else if(isless(hsb->b, bmin))
        hsb->b=bmin, hsb->s=0;
}

static float get_neutral_saturation(float brightness)
{
    /* 反比例系數。中性色的飽和度s和亮度b成反比例：sb=k。
     * k的取值可通過觀察hsb拾色器的顏色變化而憑直覺確定。*/
    float sb_k=0.04;
    return sb_k/brightness;
}

static void alloc_root_bg_color(HSB hsb)
{
    hsb=get_root_bg_hsb(hsb);
    RGB rgb=hsb_to_rgb(hsb);
    XColor xcolor=rgb_to_xcolor(rgb);
    XAllocColor(xinfo.display, xinfo.colormap, &xcolor);
    root_bg_color=xcolor.pixel;
}

/* 桌面背景色宜低飽和度、低亮度 */
static HSB get_root_bg_hsb(HSB hsb)
{
    if(isgreater(hsb.s, 0))
        hsb.s *= 0.25;
    hsb.b *= 0.25;
    
    return hsb;
}

static void alloc_widget_colors(HSB hsb)
{
    Widget_state state[]=
    {
        [STATE_NORMAL]          = {0},
        [STATE_DISABLE]         = {.disable=1},
        [STATE_ACTIVE]          = {.active=1},
        [STATE_WARN]            = {.warn=1},
        [STATE_HOT]             = {.hot=1},
        [STATE_URGENT]          = {.urgent=1},
        [STATE_ATTENT]          = {.attent=1},
        [STATE_CHOSEN]          = {.chosen=1},
        [STATE_CURRENT]         = {.current=1},
        [STATE_DISABLE_CURRENT] = {.disable=1, .current=1},
        [STATE_HOT_CHOSEN]      = {.hot=1, .chosen=1},
    };

    for(int i=0; i<COLOR_N; i++)
        alloc_widget_color(state[i], hsb);
}

static void alloc_widget_color(Widget_state state, HSB hsb)
{
    int i=state_to_index(state);
    RGB rgb=get_widget_rgb(state, hsb);
    widget_color[i]=alloc_xcolor_by_rgb(rgb).pixel;
    rgb=get_text_rgb(state, rgb);
    text_color[i]=alloc_xftcolor_by_rgb(rgb);
}

static RGB get_widget_rgb(Widget_state state, HSB hsb)
{
    switch(get_color_theme(hsb))
    {
        case DARK_COLOR_THEME:
        case LIGHT_COLOR_THEME: return get_widget_rgb_on_color(state, hsb);
        case DARK_NEUTRAL_THEME:
        case LIGHT_NEUTRAL_THEME: return get_widget_rgb_on_neutral(state, hsb);
        case DARK_GREY_THEME:
        case LIGHT_GREY_THEME: return get_widget_rgb_on_grey(state, hsb);
        default: return RGB(0, 0, 0);
    }
}

static RGB get_widget_rgb_on_color(Widget_state state, HSB hsb)
{
    float h=hsb.h, s=hsb.s, b=hsb.b;
    float dh=get_valid_dh(h, -60)/3, db=fminf((1-b)/2, 0.2);
    HSB table[COLOR_N] =
    {
        [STATE_NORMAL]          = {h,      s/2, b/2}, 
        [STATE_DISABLE]         = {h,      s/2, b/2}, 
        [STATE_ACTIVE]          = {h,      1,   1},   
        [STATE_WARN]            = {0,      1,   1}, // 紅色
        [STATE_HOT]             = {h,      s,   b+db},
        [STATE_URGENT]          = {h+dh*2, s,   b},   
        [STATE_ATTENT]          = {h+dh,   s,   b},   
        [STATE_CHOSEN]          = {h+dh*3, s,   b},   
        [STATE_CURRENT]         = {h,      s,   b},   
        [STATE_DISABLE_CURRENT] = {h,      s,   b},   
        [STATE_HOT_CHOSEN]      = {h+dh*3, s,   b+db},
    };

    return hsb_to_rgb(table[state_to_index(state)]);
}

static RGB get_widget_rgb_on_neutral(Widget_state state, HSB hsb)
{
    float h=hsb.h, s=hsb.s, b=hsb.b;
    float dh=get_valid_dh(h, 60)/3, db=fmin((1-b)/2, 0.2);
    float cs=get_cozy_s(hsb), cb=get_cozy_b(hsb);
    HSB table[COLOR_N] =
    {
        [STATE_NORMAL]          = {h,      s/2, b/2}, 
        [STATE_DISABLE]         = {h,      s/2, b/2}, 
        [STATE_ACTIVE]          = {h,      1,   1}, 
        [STATE_WARN]            = {0,      1,   1}, // 紅色
        [STATE_HOT]             = {h,      s,   b+db},
        [STATE_URGENT]          = {h+dh*2, cs,  cb}, 
        [STATE_ATTENT]          = {h+dh,   cs,  cb}, 
        [STATE_CHOSEN]          = {h+dh*3, cs,  cb},
        [STATE_CURRENT]         = {h,      s,   b},   
        [STATE_DISABLE_CURRENT] = {h,      s,   b},   
        [STATE_HOT_CHOSEN]      = {h+dh*3, cs,  1}, 
    };

    return hsb_to_rgb(table[state_to_index(state)]);
}

static RGB get_widget_rgb_on_grey(Widget_state state, HSB hsb)
{
    float h=hsb.h, s=hsb.s, b=hsb.b, db=fmin((1-b)/2, 0.2);
    float cs=get_cozy_s(hsb), cb=get_cozy_b(hsb);
    HSB table[COLOR_N] =
    {
        [STATE_NORMAL]          = {h,   s,  b/2}, 
        [STATE_DISABLE]         = {h,   s,  b/2}, 
        [STATE_ACTIVE]          = {h,   s,  1}, 
        [STATE_WARN]            = {0,   1,  1}, // 紅色
        [STATE_HOT]             = {h,   s,  b+db},
        [STATE_URGENT]          = {175, cs, cb}, 
        [STATE_ATTENT]          = {155, cs, cb}, 
        [STATE_CHOSEN]          = {195, cs, cb},
        [STATE_CURRENT]         = {h,   s,  b},   
        [STATE_DISABLE_CURRENT] = {h,   s,  b},   
        [STATE_HOT_CHOSEN]      = {195, cs, 1}, 
    };

    return hsb_to_rgb(table[state_to_index(state)]);
}

static float get_cozy_s(HSB hsb)
{
    int i=is_light_color(hsb);
    return (cozy_s_min[i]+cozy_s_max[i])/2;
}

static float get_cozy_b(HSB hsb)
{
    int i=is_light_color(hsb);
    return (cozy_b_min[i]+cozy_b_max[i])/2;
}

static bool is_light_color(HSB hsb)
{
    return isgreater(hsb.b, 0.5);
}

static XColor alloc_xcolor_by_rgb(RGB rgb)
{
    XColor xcolor=rgb_to_xcolor(rgb);
    XAllocColor(xinfo.display, xinfo.colormap, &xcolor);
    return xcolor;
}

/* XColor的成員color爲XRenderColor結構體，該結構體的顏色分量均爲16位整數，
 * 其高8位表示真正的RGB分量值 */
static XftColor alloc_xftcolor_by_rgb(RGB rgb)
{
    XftColor xc;
    XRenderColor rc={.red=rgb.r<<8, .green=rgb.g<<8, .blue=rgb.b<<8, .alpha=0xffff};
    XftColorAllocValue(xinfo.display, xinfo.visual, xinfo.colormap, &rc, &xc);
    return xc;
}

static size_t state_to_index(Widget_state state)
{
    if(state.disable) return state.current ? STATE_DISABLE_CURRENT : STATE_DISABLE;
    if(state.active)  return STATE_ACTIVE; 
    if(state.warn)    return STATE_WARN; 
    if(state.hot)     return state.chosen ? STATE_HOT_CHOSEN : STATE_HOT; 
    if(state.urgent)  return STATE_URGENT; 
    if(state.attent)  return STATE_ATTENT; 
    if(state.chosen)  return STATE_CHOSEN; 
    if(state.current) return STATE_CURRENT; 
    return STATE_NORMAL;
}

static Color_theme get_color_theme(HSB hsb)
{
    bool light=is_light_color(hsb);
    if(hsb.h == -1) // 無色相，即灰色
        return light ? LIGHT_GREY_THEME : DARK_GREY_THEME;
    if(islessequal(hsb.s, get_neutral_saturation(hsb.b)))   // 中性色
        return light ? LIGHT_NEUTRAL_THEME : DARK_NEUTRAL_THEME;
    return light ? LIGHT_COLOR_THEME : DARK_COLOR_THEME;    // 彩色
}

static RGB get_text_rgb(Widget_state state, RGB rgb)
{
    float dark_b[COLOR_N]= // 深色主題的顏色亮度列表
    {
        [STATE_NORMAL]          = 0.3,
        [STATE_DISABLE]         = 1.0,
        [STATE_ACTIVE]          = 0.0,
        [STATE_WARN]            = 0.0,
        [STATE_HOT]             = 0.1,
        [STATE_URGENT]          = 0.2,
        [STATE_ATTENT]          = 0.2,
        [STATE_CHOSEN]          = 0.2,
        [STATE_CURRENT]         = 0.0,
        [STATE_DISABLE_CURRENT] = 1.0,
        [STATE_HOT_CHOSEN]      = 0.1,
    };
    float b=dark_b[state_to_index(state)];
    HSB hsb=rgb_to_hsb(rgb);

    hsb.b = isgreater(rgb_to_luminance(rgb), 255/2.0) ? 0 : 1;
    hsb.s = isgreater(hsb.b, 0) ? 0 : -1;
    hsb.h = -1;
    hsb.b = is_light_color(hsb) ? 1-b : b;

    return hsb_to_rgb(hsb);
}

static float get_valid_dh(float h, float dh)
{
    return (is_valid_hue(h+dh) ? dh : -dh);
}

static bool is_valid_hue(float h)
{
    return isgreaterequal(h, 0) && islessequal(h, 360);
}

XftColor get_widget_fg(Widget_state state)
{
    return text_color[state_to_index(state)];
}

/* XColor的顏色分量均爲16位整數，其高8位表示真正的RGB分量值 */
static RGB xcolor_to_rgb(XColor xcolor)
{
    return RGB(xcolor.red>>8, xcolor.green>>8, xcolor.blue>>8);
}

static XColor rgb_to_xcolor(RGB rgb)
{
    return (XColor){.red=rgb.r<<8, .green=rgb.g<<8, .blue=rgb.b<<8};
}

static float rgb_to_luminance(RGB rgb)
{
    return rgb.r*0.299+rgb.g*0.587+rgb.b*0.114;
}

static HSB rgb_to_hsb(RGB rgb)
{
    int r=rgb.r, g=rgb.g, b=rgb.b;
    int max=fmaxf(fmaxf(r, g), b), min=fminf(fminf(r, g), b), delta=max-min;
    float h, s, v=max/255.0;

    if(max == 0) // 意味着RGB各分量均爲0，delta爲0。這種情況表示黑色，色相和飽和度均無法確定，亮度爲0
        return HSB(-1, -1, 0);
    if(delta == 0) // 意味着max==min，即RGB各分量相等。這種情況均表示灰色，色相無法確定，飽和度爲0，亮度爲max
        return HSB(-1, 0, v);

    s=(float)delta/max; // 通過套用公式來計算飽和度

    /* 通過套用公式來計算色相 */
    if(r == max)
        h=(float)(g-b)/delta;
    else if(g == max)
        h=2+(float)(b-r)/delta;
    else
        h=4+(float)(r-g)/delta;
    h = isgreaterequal(h, 0) ? h*60 : h*60+360;

    return HSB(h, s, v);
}

static RGB hsb_to_rgb(HSB hsb)
{
    float h=hsb.h, s=hsb.s, v=hsb.b;

    /* 當色相h等於-1時，表示顏色是灰色。在這種情況下，只有亮度有意義，即RGB分量等於亮度乘以255。*/
    if(h == -1)
        return RGB(v*255, v*255, v*255);

    h/=60; // 計算色相在色相環中的位置
    int i=(int)h; // 計算所在的色相環區段
    float f=h-i; // 計算色相在區段內的偏移值
    float p=v*(1-s); // 計算RGB中的最小分量值
    float q=v*(1-s*f); // 計算RGB中的最大分量值
    float t=v*(1-s*(1-f)); // 計算RGB中的中間分量值

    /* 根據色環區段值選擇對應的RGB值 */
    switch(i)
    {
        case 0: return RGB(v*255, t*255, p*255);
        case 1: return RGB(q*255, v*255, p*255);
        case 2: return RGB(p*255, v*255, t*255);
        case 3: return RGB(p*255, q*255, v*255);
        case 4: return RGB(t*255, p*255, v*255);
        case 5: return RGB(v*255, p*255, q*255);
        default:return RGB(-1, -1, -1);
    }
}
