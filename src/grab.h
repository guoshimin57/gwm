/* *************************************************************************
 *     grab.h：與grab.c相應的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef GRAB_H
#define GRAB_H

#include "gwm.h"

typedef union // 要綁定的函數的參數類型
{
    char *const *cmd; // 命令字符串
    unsigned int desktop_n; // 虛擬桌面編號，從0開始編號
} Arg;

typedef void (*Func)(XEvent *, Arg); // 要綁定的函數類型

typedef struct // 鍵盤按鍵功能綁定
{
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵
	KeySym keysym; // 要綁定的鍵盤功能轉換鍵
	Func func; // 要綁定的函數
    Arg arg; // 要綁定的函數的參數
} Keybind;

typedef struct // 定位器按鈕功能綁定
{
    Widget_id widget_id; // 要綁定的構件標識
	unsigned int modifier; // 要綁定的鍵盤功能轉換鍵 
    unsigned int button; // 要綁定的定位器按鈕
	Func func; // 要綁定的函數
    Arg arg; // 要綁定的函數的參數
} Buttonbind;

void reg_binds(const Keybind *kbinds, const Buttonbind *bbinds);
const Keybind *get_keybinds(void);
const Buttonbind *get_buttonbinds(void);
void grab_keys(void);
void grab_buttons(Window);
bool grab_pointer(Window win, Pointer_act act);
void create_cursors(void);
void set_cursor(Window win, Pointer_act act);
void free_cursors(void);

#endif
