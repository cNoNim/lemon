#include "action.h"
#include "error.h"
#include "msort.h"
#include "rule.h"
#include "symbol.h"

#include <stdio.h>
#include <stdlib.h>

static void *action_list_get_next(void const *list);
static void action_list_set_next(void *list, void *next);
static int action_list_compare(void const *left, void const *right);

struct action_list *
action_list_insert(struct action *action, struct action_list **list) {
  struct action_list *new_list = (struct action_list *)malloc(sizeof(struct action_list));
  new_list->item = action;
  new_list->next = *list;
  *list = new_list;
  return new_list;
}

void
action_list_sort(struct action_list **list) {
  *list = mergesort(*list, action_list_get_next, action_list_set_next, action_list_compare);
}

struct action *
make_action(struct symbol *symbol, enum action_type type, void *arg) {
  struct action *new_action = malloc(sizeof(struct action));
  MemoryCheck(new_action);
  new_action->sp = symbol;
  new_action->type = type;
  if (type == SHIFT)
    new_action->x.stp = (struct state *)arg;
  else
    new_action->x.rp = (struct rule *)arg;
}

static void *
action_list_get_next(void const *list) {
  return ((struct action_list const *)list)->next;
}

static void
action_list_set_next(void *list, void *next) {
  ((struct action_list *)list)->next = next;
}

static int
action_list_compare(void const *left, void const *right) {
  struct action *la = ((struct action_list *)left)->item;
  struct action *ra = ((struct action_list *)right)->item;
  int rc = la->sp->index - ra->sp->index;
  if (rc == 0)
    rc = (int)la->type - (int)ra->type;
  if (rc == 0 && la->type == REDUCE)
    rc = la->x.rp->index - ra->x.rp->index;
  if (rc == 0)
    rc = (int)(ra - la);
  return rc;
}
