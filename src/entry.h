/* *************************************************************************
 *     entry.h：與entry.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef ENTRY_H
#define ENTRY_H

typedef struct _entry_tag Entry; // 輸入構件

struct _entry_tag
{
    Window win;  // 輸入構件的窗口
    int x, y; // 坐標
    int w, h; // 寬和高
    wchar_t text[BUFSIZ]; // 構件上的文字
    const char *hint; // 構件的提示文字
    size_t cursor_offset; // 光標位置
    XIC xic; // 輸入法句柄
    Strings *(*complete)(Entry *, int *);
};

Entry *create_entry(int x, int y, int w, int h, const char *hint, Strings *(*complete)(Entry *, int *));
void show_entry(Entry *entry);
void update_entry_bg(Entry *entry);
void update_entry_text(Entry *entry);
bool input_for_entry(Entry *entry, XKeyEvent *ke);
void destroy_entry(Entry *entry);
void paste_for_entry(Entry *entry);
Entry *create_cmd_entry(void);

#endif
