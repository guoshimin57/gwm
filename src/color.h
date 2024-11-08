/* *************************************************************************
 *     color.h：與color.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
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

#define WIDGET_STATE_NORMAL ((Widget_state){0})

typedef struct // 構件狀態。全0表示普通狀態，即以上狀態以外的狀態。
{
    unsigned int disable : 1;   // 禁用狀態，即此時構件禁止使用
    unsigned int active : 1;    // 激活狀態，即鼠標在構件上按下
    unsigned int warn : 1;      // 警告狀態，即鼠標懸浮於重要構件之上
    unsigned int hot : 1;       // 可用狀態，即鼠標懸浮於構件之上
    unsigned int urgent : 1;    // 緊急狀態，即構件有緊急消息
    unsigned int attent : 1;    // 關注狀態，即構件有需要關注的消息
    unsigned int chosen : 1;    // 選中狀態，即選中了此構件所表示的功能
    unsigned int unfocused : 1; // 失去焦點狀態，即可接收輸入的構件失去了輸入焦點
} Widget_state;

void alloc_color(const char *main_color_name);
unsigned long get_widget_color(Widget_state state);
XftColor get_widget_fg(Widget_state state);
unsigned long get_root_bg_color(void);

#endif
