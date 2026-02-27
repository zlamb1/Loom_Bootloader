#ifndef LOOM_LIST_H
#define LOOM_LIST_H 1

#include "loom/compiler.h"
#include "loom/types.h"

#define LOOM_LIST_ITERATE(LIST, VAR)                                          \
  for (typeof (LIST) VAR = (LIST); VAR; VAR = VAR->next)

typedef struct loom_list_t
{
  struct loom_list_t *prev, *next;
} loom_list_t;

#define LOOM_LIST_HEAD(VARNAME)                                               \
  (loom_list_t) { .prev = &VARNAME, .next = &VARNAME }

#define LOOM_LIST_HEAD_INIT(VAR) VAR = LOOM_LIST_HEAD (VAR)

#define loom_list_for_each(HEAD, VARNAME)                                     \
  for (loom_list_t *_head = (HEAD), *VARNAME = _head->next; VARNAME != _head; \
       VARNAME = VARNAME->next)

#define loom_list_for_each_entry(head, var, member)                           \
  for (loom_list_t *_node = (head)->next;                                     \
       _node != (head)                                                        \
       && (var = loom_container_of (_node, typeof (*var), member), 1);        \
       _node = _node->next)

#define loom_list_for_each_rev(HEAD, VARNAME)                                 \
  for (loom_list_t *_head = (HEAD), *VARNAME = _head->prev; VARNAME != _head; \
       VARNAME = VARNAME->prev)

#define loom_list_for_each_safe(HEAD, VARNAME)                                \
  for (loom_list_t *_head = (HEAD), *VARNAME = _head->next,                   \
                   *next = VARNAME->next;                                     \
       VARNAME != _head; (VARNAME = next, next = VARNAME->next))

#define loom_list_for_each_rev_safe(HEAD, VARNAME)                            \
  for (loom_list_t *_head = (HEAD), *VARNAME = _head->prev,                   \
                   *prev = VARNAME->prev;                                     \
       VARNAME != _head; (VARNAME = prev, prev = VARNAME->prev))

#define loom_list_add loom_list_prepend

void LOOM_EXPORT (loom_list_prepend) (loom_list_t *head, loom_list_t *node);
void LOOM_EXPORT (loom_list_append) (loom_list_t *head, loom_list_t *node);

void LOOM_EXPORT (loom_list_remove) (loom_list_t *node);
void LOOM_EXPORT (loom_list_replace) (loom_list_t *oldnode,
                                      loom_list_t *newnode);

loom_bool_t LOOM_EXPORT (loom_list_is_empty) (loom_list_t *head);

#endif