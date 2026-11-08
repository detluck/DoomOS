/*
 * kernel/include/lib/list.h
 *
 * PURPOSE:
 * Intrusive circular doubly linked list (same design as Linux's list_head).
 * Header-only: every function is static inline, so there is no list.c.
 *
 * HOW IT WORKS:
 * - struct list_head is just { next, prev }. You embed one inside your own
 *   struct for every list that struct can be on.
 * - A list head is a standalone list_head that belongs to no object. It marks
 *   the start and end of the circle. An empty list points at itself.
 * - Functions work with list_head pointers (&obj->link). The *_entry macros
 *   convert back to your struct with container_of().
 *
 * USAGE:
 * 1. Embed a link in your struct and declare a head for the list:
 *
 *    struct slab {
 *        u16 in_use;
 *        struct list_head link;
 *    };
 *    static LIST_HEAD(partial);              // or INIT_LIST_HEAD(&x) at runtime
 *
 * 2. Add and remove (pass &obj->link, not obj):
 *
 *    list_add(&s->link, &partial);           // at the front
 *    list_add_tail(&s->link, &partial);      // at the back
 *    list_del(&s->link);                     // remove from its list
 *    list_move(&s->link, &full);             // move to another list
 *
 * 3. Query and walk (the last argument is the field NAME, not an expression):
 *
 *    if (!list_empty(&partial)) {
 *        struct slab *s = list_first_entry(&partial, struct slab, link);
 *    }
 *
 *    struct slab *s;
 *    list_for_each_entry(s, &partial, link) { ... }          // read-only walk
 *
 *    struct slab *s, *tmp;
 *    list_for_each_entry_safe(s, tmp, &partial, link) { ... } // body may delete/move s
 *
 * NOTES:
 * - list_del() sets the removed entry's pointers to NULL. Never use an entry
 *   after deleting it unless you re-add it.
 * - Not thread-safe. Protect a list with a lock or disabled interrupts once
 *   interrupt handlers or other CPUs touch it.
 */

#pragma once

#include "types.h"
#include "defs.h"
#include <stddef.h>

/*
 * List head structure
 *
 * Both list_head and list entries use this structure.
 * An empty list has head->next == head->prev == head.
 */
struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

// static initializer for list head
// clang-format off
#define LIST_HEAD_INIT(name) { &(name), &(name) }
// clang-format on

// declare and initialize a list head
#define LIST_HEAD(name) struct list_head name = LIST_HEAD_INIT(name)

// initialize a list head at runtime
static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

// internal helper to insert new entry between two neighbors
static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next)
{
    prev->next = new;
    new->prev = prev;
    new->next = next;
    next->prev = new;
}

// adds new entry after the specified head
static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

// adds new entry before the sprcified head
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

// internal helper to deleate the entry from the list
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    prev->next = next;
    next->prev = prev;
}

// deletes the entry from the list and sets it to NULL
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

// checks if the list is empty(every empty list points to itself)
static inline bool list_empty(struct list_head *list)
{
    return list->next == list;
}

// replace old entry by new one
static inline void list_replace(struct list_head *old, struct list_head *new)
{
    new->prev = old->prev;
    new->prev->next = new;
    new->next = old->next;
    new->next->prev = new;
}

// moves list entry to the front of another list
static inline void list_move(struct list_head *entry, struct list_head *new_head)
{
    __list_del(entry->prev, entry->next);
    list_add(entry, new_head);
}

// moves list entry to the back of another list
static inline void list_move_tail(struct list_head *entry, struct list_head *new_head)
{
    __list_del(entry->prev, entry->next);
    list_add_tail(entry, new_head);
}

/*
 * explanation of the arguments for all functions down below
 * @pos: the type * to cursor
 * @ptr: the &struct list_head pointer
 * @type: the type of the struct this is embedded in
 * @member: the name of the list_head within the struct
 * @tmp: temporary storage
 * @head of the list you want to loop in
 */

// get the struct containing this list entry
#define list_entry(ptr, type, member) container_of(ptr, type, member)

// get the first element from the list
#define list_first_entry(ptr, type, member) container_of((ptr)->next, type, member)

// get the last element from the list
#define list_last_entry(ptr, type, member) container_of((ptr)->prev, type, member)

// get the next entry from the list
#define list_next_entry(pos, member) list_entry((pos)->member.next, typeof(*(pos)), member)

// get the previous entry from the list
#define list_prev_entry(pos, member) list_entry((pos)->member.prev, typeof(*(pos)), member)

// loop each entry in the list
#define list_for_each_entry(pos, head, member) \
    for (pos = list_first_entry(head, typeof(*(pos)), member); &pos->member != (head); \
         pos = list_next_entry(pos, member))

// loop each entry safe against removal
#define list_for_each_entry_safe(pos, tmp, head, member) \
    for (pos = list_first_entry(head, typeof(*pos), member), tmp = list_next_entry(pos, member); \
         &pos->member != (head); pos = tmp, tmp = list_next_entry(tmp, member))
