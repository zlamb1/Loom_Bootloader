#ifndef LOOM_LIST_H
#define LOOM_LIST_H 1

#include "loom/assert.h"
#include "loom/compiler.h"
#include "loom/types.h"

#define LOOM_LIST_ITERATE(LIST, VAR)                                          \
  for (typeof (LIST) VAR = (LIST); VAR; VAR = VAR->next)

typedef struct loom_list
{
  struct loom_list *prev, *next;
} loom_list;

#define LOOM_LIST_HEAD(VARNAME)                                               \
  (loom_list) { .prev = &VARNAME, .next = &VARNAME }

#define LOOM_LIST_HEAD_INIT(VAR) VAR = LOOM_LIST_HEAD (VAR)

#define loom_list_for_each(HEAD, VARNAME)                                     \
  for (loom_list *_head = (HEAD), *VARNAME = _head->next; VARNAME != _head;   \
       VARNAME = VARNAME->next)

#define loom_list_for_each_entry(head, var, member)                           \
  for (loom_list *_node = (head)->next;                                       \
       _node != (head)                                                        \
       && (var = container_of (_node, typeof (*var), member), 1);             \
       _node = _node->next)

#define loom_list_for_each_rev(HEAD, VARNAME)                                 \
  for (loom_list *_head = (HEAD), *VARNAME = _head->prev; VARNAME != _head;   \
       VARNAME = VARNAME->prev)

#define loom_list_for_each_safe(HEAD, VARNAME)                                \
  for (loom_list *_head = (HEAD), *VARNAME = _head->next,                     \
                 *next = VARNAME->next;                                       \
       VARNAME != _head; (VARNAME = next, next = VARNAME->next))

#define loom_list_for_each_rev_safe(HEAD, VARNAME)                            \
  for (loom_list *_head = (HEAD), *VARNAME = _head->prev,                     \
                 *prev = VARNAME->prev;                                       \
       VARNAME != _head; (VARNAME = prev, prev = VARNAME->prev))

#define loomListAdd loomListPrepend

#define loomListAddByField(head, elem, field)                                 \
  do                                                                          \
    {                                                                         \
      loomAssert (elem != NULL);                                              \
      loomListAdd (head, &elem->field);                                       \
    }                                                                         \
  while (0)

#define loom_list_remove_by_field(elem, field)                                \
  do                                                                          \
    {                                                                         \
      loomAssert (elem != NULL);                                              \
      loomListRemove (&elem->field);                                          \
    }                                                                         \
  while (0)

void export (loomListPrepend) (loom_list *head, loom_list *node);
void export (loomListAppend) (loom_list *head, loom_list *node);

void export (loomListRemove) (loom_list *node);
void export (loomListReplace) (loom_list *oldnode, loom_list *newnode);

bool export (loomListIsEmpty) (loom_list *head);

#endif