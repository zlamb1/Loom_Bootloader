#ifndef LOOM_LIST_H
#define LOOM_LIST_H 1

#define LOOM_LIST_ITERATE(LIST, VAR)                                          \
  for (typeof (LIST) VAR = (LIST); VAR; VAR = VAR->next)

#endif