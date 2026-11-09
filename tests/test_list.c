#include <assert.h>
#include <stdio.h>
#include "lib/list.h"

struct test_node {
    int id;
    struct list_head link;
};

/* Forward walk with the iterator macro and backward walk via ->prev,
 * so a wrong prev pointer can't hide behind correct next pointers. */
static bool check_order(struct list_head *h, const int *expect, int count)
{
    int i = 0;
    struct test_node *p;

    list_for_each_entry(p, h, link)
    {
        if (i >= count || p->id != expect[i]) {
            return false;
        }
        i++;
    }
    if (i != count) {
        return false;
    }

    i = count;
    for (struct list_head *pos = h->prev; pos != h; pos = pos->prev) {
        i--;
        if (i < 0 || list_entry(pos, struct test_node, link)->id != expect[i]) {
            return false;
        }
    }
    return i == 0;
}

#define CHECK_ORDER(h, ...) \
    do { \
        const int expected[] = {__VA_ARGS__}; \
        assert(check_order((h), expected, (int) (sizeof(expected) / sizeof(expected[0])))); \
    } while (0)

static void test_init(void)
{
    LIST_HEAD(h);
    assert(list_empty(&h));
    assert(h.next == &h && h.prev == &h);

    struct list_head rt;
    INIT_LIST_HEAD(&rt);
    assert(list_empty(&rt));
    assert(rt.next == &rt && rt.prev == &rt);
    printf("[PASS] init\n");
}

static void test_add_single(void)
{
    struct test_node a = {.id = 1};
    LIST_HEAD(h);

    list_add(&a.link, &h);
    assert(!list_empty(&h));
    assert(h.next == &a.link && h.prev == &a.link);
    assert(a.link.next == &h && a.link.prev == &h);
    printf("[PASS] add single\n");
}

static void test_add_stack(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);

    list_add(&a.link, &h);
    list_add(&b.link, &h);
    list_add(&c.link, &h);
    CHECK_ORDER(&h, 3, 2, 1);
    printf("[PASS] list_add stack\n");
}

static void test_add_tail_queue(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);

    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);
    list_add_tail(&c.link, &h);
    CHECK_ORDER(&h, 1, 2, 3);
    printf("[PASS] list_add_tail\n");
}

static void test_del_middle(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);
    list_add_tail(&c.link, &h);

    list_del(&b.link);
    CHECK_ORDER(&h, 1, 3);
    assert(a.link.next == &c.link && c.link.prev == &a.link);
    printf("[PASS] list_del middle\n");
}

static void test_del_first_last(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);
    list_add_tail(&c.link, &h);

    list_del(&a.link);
    CHECK_ORDER(&h, 2, 3);
    list_del(&c.link);
    CHECK_ORDER(&h, 2);
    list_del(&b.link);
    assert(list_empty(&h));
    printf("[PASS] list_del first last\n");
}

static void test_del_poison(void)
{
    struct test_node a = {.id = 1};
    LIST_HEAD(h);
    list_add(&a.link, &h);

    list_del(&a.link);
    assert(a.link.next == NULL && a.link.prev == NULL);
    printf("[PASS] list_del entry is NULL\n");
}

static void test_readd(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2};
    LIST_HEAD(h);
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);

    list_del(&a.link);
    list_add_tail(&a.link, &h);
    CHECK_ORDER(&h, 2, 1);
    printf("[PASS] re-add\n");
}

static void test_move(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2};
    LIST_HEAD(h);
    LIST_HEAD(h2);
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);

    list_move(&a.link, &h2);
    CHECK_ORDER(&h, 2);
    CHECK_ORDER(&h2, 1);

    list_move(&a.link, &h);
    CHECK_ORDER(&h, 1, 2);
    assert(list_empty(&h2));

    list_move_tail(&a.link, &h2);
    CHECK_ORDER(&h, 2);
    CHECK_ORDER(&h2, 1);
    printf("[PASS] list_move/list_move_tail\n");
}

static void test_move_same_list(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2};
    LIST_HEAD(h);
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);

    list_move(&b.link, &h);
    CHECK_ORDER(&h, 2, 1);
    list_move_tail(&b.link, &h);
    CHECK_ORDER(&h, 1, 2);
    printf("[PASS] list_move same list\n");
}

static void test_iterate_empty(void)
{
    LIST_HEAD(h);
    struct test_node *p, *tmp;
    int visited = 0;

    list_for_each_entry(p, &h, link)
    {
        visited++;
    }
    list_for_each_entry_safe(p, tmp, &h, link)
    {
        visited++;
    }
    assert(visited == 0);
    printf("[PASS] iterate empty\n");
}

static void test_safe_delete(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);
    struct test_node *p, *tmp;
    int visited = 0;
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);
    list_add_tail(&c.link, &h);

    list_for_each_entry_safe(p, tmp, &h, link)
    {
        visited++;
        list_del(&p->link);
    }
    assert(visited == 3);
    assert(list_empty(&h));
    printf("[PASS] for_each_entry_safe deleate\n");
}

static void test_safe_move(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);
    LIST_HEAD(h2);
    struct test_node *p, *tmp;
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);
    list_add_tail(&c.link, &h);

    list_for_each_entry_safe(p, tmp, &h, link)
    {
        if (p->id != 2) {
            list_move_tail(&p->link, &h2);
        }
    }
    CHECK_ORDER(&h, 2);
    CHECK_ORDER(&h2, 1, 3);
    printf("[PASS] for_each_entry_safe move\n");
}

static void test_first_last_entry(void)
{
    struct test_node a = {.id = 1}, b = {.id = 2}, c = {.id = 3};
    LIST_HEAD(h);
    list_add_tail(&a.link, &h);
    list_add_tail(&b.link, &h);
    list_add_tail(&c.link, &h);

    assert(list_first_entry(&h, struct test_node, link) == &a);
    assert(list_last_entry(&h, struct test_node, link) == &c);
    assert(list_first_entry(&h, struct test_node, link)->id == 1);
    assert(list_last_entry(&h, struct test_node, link)->id == 3);
    printf("[PASS] first/last_entry\n");
}

int main(void)
{
    printf("--- Running List Unit Tests ---\n");
    test_init();
    test_add_single();
    test_add_stack();
    test_add_tail_queue();
    test_del_middle();
    test_del_first_last();
    test_del_poison();
    test_readd();
    test_move();
    test_move_same_list();
    test_iterate_empty();
    test_safe_delete();
    test_safe_move();
    test_first_last_entry();
    printf("--- All 14 Unit Tests Passed! ---\n");
    return 0;
}
