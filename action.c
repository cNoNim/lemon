#include "action.h"
#include "msort.h"
#include "table.h"

#include <stdio.h>
#include <stdlib.h>

static int actioncmp(struct action *, struct action *);

/* Allocate a new parser action */
struct action *
Action_new(void) {
  static struct action *freelist = 0;
  struct action *newaction;

  if (freelist == 0) {
    int i;
    size_t amt = 100;
    freelist = (struct action *)calloc(amt, sizeof(struct action));
    if (freelist == 0) {
      fprintf(stderr, "Unable to allocate memory for a new parser action.");
      exit(1);
    }
    for (i = 0; i < amt - 1; i++)
      freelist[i].next = &freelist[i + 1];
    freelist[amt - 1].next = 0;
  }
  newaction = freelist;
  freelist = freelist->next;
  return newaction;
}

void
Action_add(struct action **app, enum e_action type, struct symbol *sp, char *arg) {
  struct action *newaction;
  newaction = Action_new();
  newaction->next = *app;
  *app = newaction;
  newaction->type = type;
  newaction->sp = sp;
  if (type == SHIFT) {
    newaction->x.stp = (struct state *)arg;
  } else {
    newaction->x.rp = (struct rule *)arg;
  }
}

/* Sort parser actions */
struct action *
Action_sort(struct action *ap) {
  ap = (struct action *)msort((char *)ap, (char **)&ap->next, (int (*)(const char *, const char *))actioncmp);
  return ap;
}

/* Compare two actions for sorting purposes.  Return negative, zero, or
 * positive if the first action is less than, equal to, or greater than
 * the first
 */
static int
actioncmp(struct action *ap1, struct action *ap2) {
  int rc;
  rc = ap1->sp->index - ap2->sp->index;
  if (rc == 0) {
    rc = (int)ap1->type - (int)ap2->type;
  }
  if (rc == 0 && ap1->type == REDUCE) {
    rc = ap1->x.rp->index - ap2->x.rp->index;
  }
  if (rc == 0) {
    rc = (int)(ap2 - ap1);
  }
  return rc;
}
