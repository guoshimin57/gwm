/* *************************************************************************
 *     list.h：與list.c相應的實現雙向鏈表操作的頭文件。
 *     版權 (C) 2020-2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#ifndef LIST_H
#define LIST_H

#include <stdbool.h>
#include <stddef.h>

/* ============================ 總說明 =====================================
 * 本實現參考Linux內核雙向循環鏈表的實現：把鏈表設計成只有指針域，需要使用
 * 鏈表的數據結構只需要把這個鏈表嵌入自己的結構體中，即可使用本實現所提供的
 * 鏈表接口。以下如無特殊說明，成員就是指這種只有指針域的鏈表，容器結構體就
 * 是指這種鏈表所嵌入的結構體。因Linux內核鏈表有時用entry表示鏈表，有時又用
 * entry表示鏈表節點所對應的容器結構體。故了爲清晰起見，本實現統一使用entry
 * 表示鏈表節點對應的容器結構體，簡稱爲表項；當小寫標識符不含entry時，指的是
 * 鏈表節點，簡稱爲鏈表節點或節點。另外，Linux內核有時用head表示鏈表頭，有時
 * 又用head表示某個節點之前的節點。本實現統一使用list表示鏈表頭。雖然容器結
 * 構體在嚴格意義上不算鏈表節點，但因爲內嵌了這種List節點，所以我還是稱之爲
 * 鏈表節點，相應的節點系列稱之爲鏈表。爲了區別起見，以下我稱之爲容器鏈表節點
 * 、容器鏈表，其表頭用clist表示。以下全大寫的宏表示操作對象是容器鏈表，並且
 * 容器鏈表節點的List成員名爲list。這些全大寫宏更易於理解和書寫，建議使用本
 * 模塊的調用者只使用這些全大寫的宏。
 * ====================================================================== */

/* 雙向循環鏈表結構體。把它嵌入其他結構體時，即可使用這裏的提供的接口。 */
typedef struct list_tag
{
    struct list_tag *prev;
    struct list_tag *next;
} List;

/* 功能：正向遍歷鏈表節點
 * node：用於遍歷鏈表節點的List*類型指針
 * list：List*類型鏈表頭
 */
#define list_for_each(node, list) \
    for(List *node=(list)->next; !list_is_head(node, (list)); node=node->next)

/* 功能：把結構體成員指針強制轉換爲包含該成員的結構體指針
 * ptr：結構體成員指針
 * type：ptr指向的成員所屬的結構體的類型
 * member：ptr指向的成員在結構體內的名字
 */
#define container_of(ptr, type, member) \
    ((type *)((char *)ptr-offsetof(type, member)))

/* 功能：獲取容器結構體表項
 * plist：List*類型的成員指針
 * type：容器結構體類型
 * member：List類型的成員在容器結構體內的名字
 */
#define list_entry(plist, type, member) \
    container_of(plist, type, member)

/* 功能：獲取鏈表第一個節點對應的容器結體指針
 * list：List*類型的成員指針，爲鏈表頭
 * type：容器結構體類型
 * member：List類型的成員在容器結構體內的名字
 */
#define list_first_entry(list, type, member) \
    list_entry((list)->next, type, member)

/* 功能：獲取鏈表最後一個節點對應的容器結體指針
 * list：List*類型的成員指針，爲鏈表頭
 * type：容器結構體類型
 * member：List類型的成員在容器結構體內的名字
 */
#define list_last_entry(list, type, member) \
    list_entry((list)->prev, type, member)

/* 功能：獲取鏈表下一個節點對應的容器結體指針
 * entry：鏈表當前節點對應的type*類型的容器結構體指針
 * type：容器結構體類型
 * member：List類型的成員在容器結構體內的名字
 */
#define list_next_entry(entry, type, member) \
    list_entry((entry)->member.next, type, member)

/* 功能：獲取鏈表上一個節點對應的容器結體指針
 * entry：鏈表當前節點對應的type*類型的容器結構體指針
 * type：容器結構體類型
 * member：List類型的成員在容器結構體內的名字
 */
#define list_prev_entry(entry, type, member) \
    list_entry((entry)->member.prev, type, member)

/* 功能：測試是否爲鏈表頭節點對應的容器結構體指針
 * entry：待測試的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_entry_is_head(entry, list, member) \
    list_is_head(&(entry)->member, (list))

/* 功能：正向遍歷鏈表項
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry(type, entry, list, member) \
    for(type *entry=list_first_entry(list, type, member); \
        !list_entry_is_head(entry, list, member); \
        entry=list_next_entry(entry, type, member))

/* 功能：正向遍歷鏈表（可以安全地刪除表項）
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry_safe(type, entry, list, member) \
    for(type *entry=list_first_entry(list, type, member), \
        *_n=list_next_entry(entry, type, member); \
        !list_entry_is_head(entry, list, member); \
        entry=_n, _n=list_next_entry(_n, type, member))

/* 功能：逆向遍歷鏈表項
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry_reverse(type, entry, list, member) \
    for(type *entry=list_last_entry(list, type, member); \
        !list_entry_is_head(entry, list, member); \
        entry=list_prev_entry(entry, type, member))

/* 功能：繼續正向遍歷鏈表項
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry_continue(type, entry, list, member) \
    for(entry=list_next_entry(entry, type, member); \
        !list_entry_is_head(entry, list, member); \
        entry=list_next_entry(entry, type, member))

/* 功能：繼續逆向遍歷鏈表項
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry_continue_reverse(type, entry, list, member) \
    for(entry=list_prev_entry(entry, type, member); \
        !list_entry_is_head(entry, list, member); \
        entry=list_prev_entry(entry, type, member))

/* 功能：從指定表項開始正向遍歷鏈表項
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry_from(type, entry, list, member) \
    for(; !list_entry_is_head(entry, list, member); \
        entry=list_next_entry(entry, type, member))

/* 功能：從指定表項開始逆向遍歷鏈表項
 * type：容器結構體類型
 * entry：用於遍歷鏈表項的type*類型的容器結構體指針
 * list：List*類型的成員指針，爲鏈表頭
 * member：List類型的成員在容器結構體內的名字
 */
#define list_for_each_entry_from_reverse(type, entry, list, member) \
    for(; !list_entry_is_head(entry, list, member); \
        entry=list_prev_entry(entry, type, member))

void list_init(List *list);
void list_add(List *node, List *head);
void list_add_tail(List *node, List *head);
void list_del(List *node);
void list_bulk_move(List *head, List *first, List *last);
void list_bulk_add(List *head, List *first, List *last);
void list_bulk_del(List *first, List *last);
size_t list_count_nodes(const List *list);
bool list_is_head(const List *node, const List *list);
bool list_is_empty(const List *list);

#define LIST_FIRST(type, clist) list_first_entry(&(clist)->list, type, list)
#define LIST_LAST(type, clist) list_last_entry(&(clist)->list, type, list)
#define LIST_NEXT(type, entry) list_next_entry(entry, type, list)
#define LIST_PREV(type, entry) list_prev_entry(entry, type, list)
#define LIST_FOR_EACH(type, entry, clist) \
    list_for_each_entry(type, entry, &(clist)->list, list)
#define LIST_FOR_EACH_SAFE(type, entry, clist) \
    list_for_each_entry_safe(type, entry, &(clist)->list, list)
#define LIST_FOR_EACH_REVERSE(type, entry, clist) \
    list_for_each_entry_reverse(type, entry, &(clist)->list, list)
#define LIST_FOR_EACH_CONTINUE(type, entry, clist) \
    list_for_each_entry_continue(type, entry, &(clist)->list, list)
#define LIST_FOR_EACH_CONTINUE_REVERSE(type, entry, clist) \
    list_for_each_entry_continue_reverse(type, entry, &(clist)->list, list)
#define LIST_FOR_EACH_FROM(type, entry, clist) \
    list_for_each_entry_from(type, entry, &(clist)->list, list)
#define LIST_FOR_EACH_FROM_REVERSE(type, entry, clist) \
    list_for_each_entry_from_reverse(type, entry, &(clist)->list, list)

#define LIST_INIT(head) list_init(&(head)->list)
#define LIST_ADD(entry, head) list_add(&(entry)->list, &(head)->list)
#define LIST_ADD_TAIL(entry, head) list_add_tail(&(entry)->list, &(head)->list)
#define LIST_DEL(entry) list_del(&(entry)->list)
#define LIST_BULK_MOVE(head, first, last) \
    list_bulk_move(&(head)->list, &(first)->list, &(last)->list)
#define LIST_BULK_ADD(head, first, last) \
    list_bulk_add(&(head)->list, &(first)->list, &(last)->list)
#define LIST_BULK_DEL(first, last) list_bulk_del(&(first)->list, &(last)->list)
#define LIST_COUNT(clist) list_count_nodes(&(clist)->list)
#define LIST_IS_HEAD(entry, clist) list_is_head(&(entry)->list, &(clist)->list)
#define LIST_IS_EMPTY(clist) (clist==NULL || list_is_empty(&(clist)->list))

#endif
