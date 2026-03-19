#include "loom/list.h"
#include "loom/assert.h"
#include "loom/types.h"

void
loomListPrepend (loom_list *head, loom_list *node)
{
  loomAssert (head != NULL);
  loomAssert (head->prev != NULL);
  loomAssert (head->next != NULL);
  loomAssert (node != NULL);

  node->prev = head;
  node->next = head->next;
  head->next->prev = node;
  head->next = node;
}

void
loomListAppend (loom_list *head, loom_list *node)
{
  loomAssert (head != NULL);
  loomAssert (head->prev != NULL);
  loomAssert (head->next != NULL);
  loomAssert (node != NULL);

  node->prev = head->prev;
  node->next = head;
  head->prev->next = node;
  head->prev = node;
}

void
loomListRemove (loom_list *node)
{
  loomAssert (node != NULL);
  loomAssert (node->prev != NULL);
  loomAssert (node->next != NULL);

  node->prev->next = node->next;
  node->next->prev = node->prev;

  node->prev = node->next = node;
}

void
loomListReplace (loom_list *oldnode, loom_list *newnode)
{
  loomAssert (oldnode != NULL);
  loomAssert (oldnode->prev != NULL);
  loomAssert (oldnode->next != NULL);

  loomAssert (newnode != NULL);

  newnode->prev = oldnode->prev;
  newnode->next = oldnode->next;

  oldnode->prev->next = newnode;
  oldnode->next->prev = newnode;

  oldnode->prev = oldnode->next = oldnode;
}

bool
loomListIsEmpty (loom_list *head)
{
  loomAssert (head != NULL);
  loomAssert (head->prev != NULL);
  loomAssert (head->next != NULL);

  if (head->prev == head)
    {
      loomAssert (head->next == head);
      return true;
    }

  return false;
}