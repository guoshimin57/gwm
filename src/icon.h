/* *************************************************************************
 *     icon.h：與icon.c相應的頭文件。
 *     版權 (C) 2020-2023 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef ICON_H
#define ICON_H

struct icon_tag // 縮微窗口相關信息
{
    Window win; // 縮微窗口
    int x, y, w, h; // 無邊框時縮微窗口的坐標、尺寸
    Area_type area_type; // 窗口微縮之前的區域類型
    bool is_short_text; // 是否只爲縮微窗口顯示簡短的文字
    char *title_text; // 縮微窗口標題文字，即XA_WM_ICON_NAME，理論上應比XA_WM_NAME簡短，實際上很多客戶窗口的都是與它一模一樣。
};

void iconify(WM *wm, Client *c);
void update_icon_area(WM *wm);
int get_icon_draw_width(WM *wm, Client *c);
void draw_icon(WM *wm, Client *c);
void deiconify(WM *wm, Client *c);
void del_icon(WM *wm, Client *c);
void iconify_all_clients(WM *wm);
void deiconify_all_clients(WM *wm);

#endif
