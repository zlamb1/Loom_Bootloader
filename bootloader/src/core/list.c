#include "loom/list.h"
#include "loom/assert.h"
#include "loom/types.h"

void
loom_list_prepend (loom_list_t *head, loom_list_t *node)
{
  loom_assert (head != NULL);
  loom_assert (head->prev != NULL);
  loom_assert (head->next != NULL);

  loom_assert (node != NULL);

  node->prev = head;
  node->next = head->next;
  head->next->prev = node;
  head->next = node;
}

void
loom_list_append (loom_list_t *head, loom_list_t *node)
{
  loom_assert (head != NULL);
  loom_assert (head->prev != NULL);
  loom_assert (head->next != NULL);

  loom_assert (node != NULL);

  node->prev = head->prev;
  node->next = head;
  head->prev->next = node;
  head->prev = node;
}

void
loom_list_remove (loom_list_t *node)
{
  loom_assert (node != NULL);
  loom_assert (node->prev != NULL);
  loom_assert (node->next != NULL);

  node->prev->next = node->next;
  node->next->prev = node->prev;

  node->prev = node->next = node;
}

void
loom_list_replace (loom_list_t *oldnode, loom_list_t *newnode)
{
  loom_assert (oldnode != NULL);
  loom_assert (oldnode->prev != NULL);
  loom_assert (oldnode->next != NULL);

  loom_assert (newnode != NULL);

  newnode->prev = oldnode->prev;
  newnode->next = oldnode->next;

  oldnode->prev->next = newnode;
  oldnode->next->prev = newnode;

  oldnode->prev = oldnode->next = oldnode;
}

loom_bool_t
loom_list_is_empty (loom_list_t *head)
{
  loom_assert (head != NULL);
  loom_assert (head->prev != NULL);
  loom_assert (head->next != NULL);

  if (head->prev == head)
    {
      loom_assert (head->next == head);
      return 1;
    }

  return 0;
}