/* *************************************************************************
 *     list.c：實現雙向循環鏈表相關功能。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include <stddef.h>
#include "list.h"

static void _list_add(List *node, List *prev, List *next);
static bool _list_add_valid(List *node, List *prev, List *next);
static void _list_del(List *node);
static bool _list_del_valid(List *node);

/* 初始化鏈表 */
void list_init(List *list)
{
    list->next=list->prev=list;
}

/* 頭插法插入節點 */
void list_add(List *node, List *head)
{
    _list_add(node, head, head->next);
}

/* 尾插法插入節點 */
void list_add_tail(List *node, List *head)
{
    _list_add(node, head->prev, head);
}

/* 刪除給定的鏈表節點 */
void list_del(List *node)
{
    _list_del(node);
    node->next=node->prev=NULL;
}

/* 在兩個已知鏈表節點（可以相同）之間插入新節點 */
static void _list_add(List *node, List *prev, List *next)
{
    if(!_list_add_valid(node, prev, next))
        return;

    next->prev=node;
    node->next=next;
    node->prev=prev;
    prev->next=node;
}

/* 測試_list_add的參數是否有效 */
static bool _list_add_valid(List *node, List *prev, List *next)
{
    return next->prev==prev && prev->next==next
        && node!=prev && node!=next;
}

/* 通過使prev/next指向對方來刪除表項 */
static void _list_del(List *node)
{
    if(!_list_del_valid(node))
        return;

    List *prev=node->prev, *next=node->next;
    next->prev=prev;
    prev->next=next;
}

/* 測試_list_del的參數是否有效 */
static bool _list_del_valid(List *node)
{
    List *prev=node->prev, *next=node->next;
    return prev->next==node && next->prev==node;
}

/* 把鏈表中first至last之間的段落移動到head之後。
 * 注意：first和last可以相同；head須不在first至last之間；三者須屬同一鏈表。*/
void list_bulk_move(List *head, List *first, List *last)
{
    list_bulk_del(first, last);
    list_bulk_add(head, first, last);
}

/* 把first至last之間的鏈表段落添加到head之後。
 * 注意：first和last可以相同且須屬同一鏈表；head須不在first至last之間。*/
void list_bulk_add(List *head, List *first, List *last)
{
    head->next->prev=last;
    last->next=head->next;

    head->next=first;
    first->prev=head;
}

/* 把鏈表中first至last之間的段落刪除。
 * 注意：first和last可以相同；兩者須屬同一鏈表。*/
void list_bulk_del(List *first, List *last)
{
    first->prev->next=last->next;
    last->next->prev=first->prev;
}

/* 取得鏈表節點數 */
size_t list_count_nodes(const List *list)
{
    size_t n=0;
    list_for_each(p, list)
        n++;
    return n;
}

/* 測試是否爲表頭 */
bool list_is_head(const List *node, const List *list)
{
    return node==list;
}

/* 測試是否爲空表 */
bool list_is_empty(const List *list)
{
    return list==NULL || list->next==list;
}
