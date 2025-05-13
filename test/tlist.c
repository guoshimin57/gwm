/* *************************************************************************
 *     tlist.c：對list模塊進行單元測試。
 *     版權 (C) 2025 gsm <406643764@qq.com>
 *     本程序為自由軟件：你可以依據自由軟件基金會所發布的第三版或更高版本的
 * GNU通用公共許可證重新發布、修改本程序。
 *     雖然基于使用目的而發布本程序，但不負任何擔保責任，亦不包含適銷性或特
 * 定目標之適用性的暗示性擔保。詳見GNU通用公共許可證。
 *     你應該已經收到一份附隨此程序的GNU通用公共許可證副本。否則，請參閱
 * <http://www.gnu.org/licenses/>。
 * ************************************************************************/

#include "../src/list.c"
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define LIST_N_MIN 3
#define LIST_N_MAX 10

typedef struct
{
    size_t n;
    List list;
} Tlist;

static size_t get_rand_list_n(void);
static size_t get_rand_gap(size_t list_n);
static void init_entrys(Tlist entrys[], size_t n);
static void free_list(List *list);
static void free_tlist(Tlist *tlist);
static bool is_tlist_valid(const Tlist *tlist);
static void test_list_func(Tlist *tlist, Tlist entrys[], size_t n);
static void test_list_wrap_func_macro(Tlist *tlist, Tlist entrys[], size_t n);
static void test_list_org_marco(Tlist *tlist, Tlist entrys[], size_t n);
static void test_list_wrap_macro(Tlist *tlist, Tlist entrys[], size_t n);

int main(void)
{
    size_t n=get_rand_list_n();
    Tlist tlist, entrys[n];

    init_entrys(entrys, n);

    test_list_func(&tlist, entrys, n);
    test_list_wrap_func_macro(&tlist, entrys, n);
    test_list_org_marco(&tlist, entrys, n);
    test_list_wrap_macro(&tlist, entrys, n);

    return 0;
}

static size_t get_rand_list_n(void)
{
    size_t n=0;

    srand(time(NULL));
    n=rand()%(LIST_N_MAX+1);

    if(n < LIST_N_MIN)
        n=LIST_N_MIN;

    return n;
}

static size_t get_rand_gap(size_t list_n)
{
    srand(time(NULL));

    size_t gap=rand()%list_n;

    return gap>=1 ? gap : 1;
}

static void init_entrys(Tlist entrys[], size_t n)
{
    for(size_t i=0; i<n; i++)
        entrys[i].n=i;
}

static void free_list(List *list)
{
    list_for_each_entry_safe(Tlist, entry, list, list)
        list_del(&entry->list);
    assert(list_count_nodes(list) == 0);
}

static void free_tlist(Tlist *tlist)
{
    LIST_FOR_EACH_SAFE(Tlist, entry, tlist)
        LIST_DEL(entry);
    assert(LIST_COUNT(tlist) == 0);
}

static bool is_tlist_valid(const Tlist *tlist)
{
    if(!tlist)
        return false;

    size_t n=0;
    const List *list=&tlist->list;
    if(!list || !list_is_head(list, list) || !LIST_IS_HEAD(tlist, tlist))
        return false;

    for(const List *p=list->next; p!=list; p=p->next, n++)
        if(!p->next || !p->prev || p->next->prev!=p || p->prev->next!=p
            || list_is_head(p, list) || list_is_empty(list))
            return false;

    if(list->next == list)
        return list_is_empty(list) && list_count_nodes(list)==0
            && LIST_IS_EMPTY(tlist) && LIST_COUNT(tlist)==0;

    return !list_is_empty(list) && list_count_nodes(list)==n
        && !LIST_IS_EMPTY(tlist) && LIST_COUNT(tlist)==n;
}

static void test_list_func(Tlist *tlist, Tlist entrys[], size_t n)
{
    size_t gap=get_rand_gap(n);
    List *list=&tlist->list, *node=NULL, *head=NULL;

    list_init(list);
    assert(is_tlist_valid(tlist));

    for(size_t i=0; i<n; i++)
    {
        node=&entrys[i].list;

        assert(_list_add_valid(node, list, list->next));
        list_add(node, list), assert(is_tlist_valid(tlist));
        assert(_list_del_valid(node));
        list_del(node), assert(is_tlist_valid(tlist));
        _list_add(node, list, list->next), assert(is_tlist_valid(tlist));
        _list_del(node), assert(is_tlist_valid(tlist));
        list_add_tail(node, list), assert(is_tlist_valid(tlist));

        head=node->prev;
        list_bulk_move(list, node, node), assert(is_tlist_valid(tlist));
        list_bulk_del(node, node), assert(is_tlist_valid(tlist));
        list_bulk_add(head, node, node), assert(is_tlist_valid(tlist));

        if(i >= gap)
        {
            head = i>gap ? &entrys[i-gap-1].list : list;
            list_bulk_move(list, &entrys[i-gap].list, node);
            assert(is_tlist_valid(tlist));
            list_bulk_del(&entrys[i-gap].list, node);
            assert(is_tlist_valid(tlist));
            list_bulk_add(head, &entrys[i-gap].list, node);
            assert(is_tlist_valid(tlist));
        }
    }

    free_tlist(tlist);
}

static void test_list_wrap_func_macro(Tlist *tlist, Tlist entrys[], size_t n)
{
    size_t gap=get_rand_gap(n);
    Tlist *head=NULL;

    LIST_INIT(tlist);
    assert(is_tlist_valid(tlist));

    for(size_t i=0; i<n; i++)
    {
        LIST_ADD(entrys+i, tlist), assert(is_tlist_valid(tlist));
        LIST_DEL(entrys+i), assert(is_tlist_valid(tlist));
        LIST_ADD_TAIL(entrys+i, tlist), assert(is_tlist_valid(tlist));

        head=LIST_PREV(Tlist, entrys+i);
        LIST_BULK_MOVE(tlist, entrys+i, entrys+i), assert(is_tlist_valid(tlist));
        LIST_BULK_DEL(entrys+i, entrys+i), assert(is_tlist_valid(tlist));
        LIST_BULK_ADD(head, entrys+i, entrys+i), assert(is_tlist_valid(tlist));

        if(i >= gap)
        {
            head = i>gap ? entrys+i-gap-1 : tlist;
            LIST_BULK_MOVE(tlist, entrys+i-gap, entrys+i);
            assert(is_tlist_valid(tlist));
            LIST_BULK_DEL(entrys+i-gap, entrys+i);
            assert(is_tlist_valid(tlist));
            LIST_BULK_ADD(head, entrys+i-gap, entrys+i);
            assert(is_tlist_valid(tlist));
        }
    }
}

static void test_list_org_marco(Tlist *tlist, Tlist entrys[], size_t n)
{
    size_t i=0;
    Tlist *p=NULL;

    list_init(&tlist->list);
    assert(is_tlist_valid(tlist));

    for(i=0; i<n; i++)
    {
        list_add_tail(&entrys[i].list, &tlist->list);
        assert(container_of(&entrys[i].list, Tlist, list) == entrys+i);
        assert(list_entry(&entrys[i].list, Tlist, list) == entrys+i);
        assert(!list_entry_is_head(entrys+i, &tlist->list, list));
        assert(list_first_entry(&tlist->list, Tlist, list) == entrys);
        assert(list_last_entry(&tlist->list, Tlist, list) == entrys+i);
        assert(list_next_entry(entrys+i, Tlist, list) == tlist);
        assert(list_prev_entry(entrys+i, Tlist, list) == (i>0 ? entrys+i-1 : tlist));
        assert(is_tlist_valid(tlist));
    }

    i=0;
    list_for_each(node, &tlist->list)
        assert(++i && !list_is_head(node, &tlist->list));
    assert(i == n);

    i=0;
    list_for_each_entry(Tlist, entrys, &tlist->list, list)
        assert(entrys->n == i++);

    i=0;
    list_for_each_entry_reverse(Tlist, entrys, &tlist->list, list)
        assert(entrys->n == n-++i);

    i=1;
    p=entrys+i;
    list_for_each_entry_continue(Tlist, p, &tlist->list, list)
        assert(p->n == ++i);

    i=n-2;
    p=entrys+i;
    list_for_each_entry_continue_reverse(Tlist, p, &tlist->list, list)
        assert(p->n == --i);

    i=1;
    p=entrys+i;
    list_for_each_entry_from(Tlist, p, &tlist->list, list)
        assert(p->n == i++);

    i=n-2;
    p=entrys+i;
    list_for_each_entry_from_reverse(Tlist, p, &tlist->list, list)
        assert(p->n == i--);

    free_list(&tlist->list);
}

static void test_list_wrap_macro(Tlist *tlist, Tlist entrys[], size_t n)
{
    size_t i=0;
    Tlist *p=NULL;

    LIST_INIT(tlist); // 鏈表狀態：只有tlist

    for(i=0; i<n; i++)
    {
        entrys[i].n=i;
        LIST_ADD_TAIL(entrys+i, tlist);
        assert(LIST_FIRST(Tlist, tlist) == entrys);
        assert(LIST_LAST(Tlist, tlist) == entrys+i);
        assert(LIST_NEXT(Tlist, entrys+i) == tlist);
        assert(LIST_PREV(Tlist, entrys) == tlist);
        assert(is_tlist_valid(tlist));
    }

    i=0;
    LIST_FOR_EACH(Tlist, entrys, tlist)
        assert(entrys->n == i++);

    i=0;
    LIST_FOR_EACH_REVERSE(Tlist, entrys, tlist)
        assert(entrys->n == n-++i);

    i=1;
    p=entrys+i;
    LIST_FOR_EACH_CONTINUE(Tlist, p, tlist)
        assert(p->n == ++i);

    i=n-2;
    p=entrys+i;
    LIST_FOR_EACH_CONTINUE_REVERSE(Tlist, p, tlist)
        assert(p->n == --i);

    i=1;
    p=entrys+i;
    LIST_FOR_EACH_FROM(Tlist, p, tlist)
        assert(p->n == i++);

    i=n-2;
    p=entrys+i;
    LIST_FOR_EACH_FROM_REVERSE(Tlist, p, tlist)
        assert(p->n == i--);

    LIST_FOR_EACH_SAFE(Tlist, entrys, tlist)
        if(entrys->n == 1)
            LIST_DEL(entrys);
    assert(LIST_COUNT(tlist) == n-1);

    free_tlist(tlist);
}
