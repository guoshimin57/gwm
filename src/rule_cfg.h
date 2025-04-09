/* *************************************************************************
 *     rule_cfg.h：配置規則的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef RULE_CFG_H
#define RULE_CFG_H

#include "gwm.h"

/* 功能：設置窗口管理器規則。
 * 說明：Rule的定義詳見gwm.h。可通過xprop命令查看客戶程序類型和客戶程序名稱、標題。其結果表示爲：
 *     WM_CLASS(STRING) = "客戶程序名稱", "客戶程序類型"
 *     WM_NAME(STRING) = "標題"
 *     _NET_WM_NAME(UTF8_STRING) = "標題"
 * 當客戶程序類型和客戶程序名稱、標題取NULL或"*"時，表示匹配任何字符串。
 * 一個窗口可以歸屬多個桌面，桌面從0開始編號，桌面n的掩碼計算公式：1<<n。
 * 譬如，桌面0的掩碼是1<<0，即1；桌面1的掩碼是1<<1，即2；1&2即3表示窗口歸屬桌面0和1。
 * 若掩碼爲0，表示窗口歸屬默認桌面。
 */
static const Rule rules[] =
{
    /* 客戶程序類型        客戶程序名稱          標題   客戶程序的類型別名 窗口放置位置  桌面掩碼 */
    {"QQ",                 "qq",                 "QQ",         "QQ",       FIXED_AREA,      0},
    {"explorer.exe",       "explorer.exe",       "*",          NULL,       ABOVE_LAYER,     0},
    {"Thunder.exe",        "Thunder.exe",        "*",          NULL,       ABOVE_LAYER,     0},
    {"firefox",            "Toolkit",            "*",          NULL,       MAIN_AREA,       0},
    {"Google-chrome",      "google-chrome",      "*",          "chrome",   ANY_PLACE,       0},
    {"Org.gnome.Nautilus", "org.gnome.Nautilus", "*",          "Nautilus", ANY_PLACE,       0},
    {0} // 哨兵值，表示結束，切勿刪改之
};

#endif
