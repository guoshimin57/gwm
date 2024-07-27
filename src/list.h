/* *************************************************************************
 *     list.h：與list.c相應的頭文件。
 *     版權 (C) 2020-2024 gsm <406643764@qq.com>
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
 * 表示鏈表節點對應的容器結構體，簡稱爲表項；當標識符不含entry時，指的是鏈表
 * 節點，簡稱爲鏈表節點或節點。另外，Linux內核有時用head表示鏈表頭，有時又用
 * head表示某個節點之前的節點。本實現統一使用list表示鏈表頭。
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
 * list：List*類型的成員指針
 * type：容器結構體類型
 * member：List類型的成員在容器結構體內的名字
 */
#define list_entry(list, type, member) \
    container_of(list, type, member)

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
        *n=list_next_entry(entry, type, member); \
        !list_entry_is_head(entry, list, member); \
        entry=n, n=list_next_entry(n, type, member))

void list_init(List *list);
void list_add(List *node, List *head);
void list_add_tail(List *node, List *head);
void list_del(List *node);
size_t list_count_nodes(const List *list);
bool list_is_head(const List *node, const List *list);
bool list_is_empty(const List *list);

#endif
