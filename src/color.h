/* *************************************************************************
 *     color.h：與color.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef COLOR_H 
#define COLOR_H 

#include <X11/Xft/Xft.h>

typedef enum // 顏色號
{
    COLOR_NORMAL, 
    COLOR_ACTIVE, 
    COLOR_WARN, 
    COLOR_HOT, 
    COLOR_URGENT, 
    COLOR_ATTENT, 
    COLOR_CHOSEN, 
    COLOR_UNFOCUSED, 
    COLOR_HOT_CHOSEN,
    COLOR_LAST=COLOR_HOT_CHOSEN
} Color_id;

unsigned long get_root_color(void);
unsigned long find_widget_color(Color_id cid);
void alloc_color(const char *main_color_name);
XftColor find_text_color(Color_id cid);

#endif
