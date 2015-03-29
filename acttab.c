#include "acttab.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Free all memory associated with the given acttab */
void
acttab_free(struct acttab *p) {
  free(p->aAction);
  free(p->aLookahead);
  free(p);
}

/* Allocate a new acttab structure */
struct acttab *
acttab_alloc(void) {
  struct acttab *p = (struct acttab *)calloc(1, sizeof(*p));
  if (p == 0) {
    fprintf(stderr, "Unable to allocate memory for a new acttab.");
    exit(1);
  }
  memset(p, 0, sizeof(*p));
  return p;
}

/* Add a new action to the current transaction set.
 *
 * This routine is called once for each lookahead for a particular
 * state.
 */
void
acttab_action(struct acttab *p, int lookahead, int action) {
  if (p->nLookahead >= p->nLookaheadAlloc) {
    p->nLookaheadAlloc += 25;
    p->aLookahead = (struct lookahead_action *)realloc(p->aLookahead, sizeof(p->aLookahead[0]) * p->nLookaheadAlloc);
    if (p->aLookahead == 0) {
      fprintf(stderr, "malloc failed\n");
      exit(1);
    }
  }
  if (p->nLookahead == 0) {
    p->mxLookahead = lookahead;
    p->mnLookahead = lookahead;
    p->mnAction = action;
  } else {
    if (p->mxLookahead < lookahead)
      p->mxLookahead = lookahead;
    if (p->mnLookahead > lookahead) {
      p->mnLookahead = lookahead;
      p->mnAction = action;
    }
  }
  p->aLookahead[p->nLookahead].lookahead = lookahead;
  p->aLookahead[p->nLookahead].action = action;
  p->nLookahead++;
}

/* Add the transaction set built up with prior calls to acttab_action()
 * into the current action table.  Then reset the transaction set back
 * to an empty set in preparation for a new round of acttab_action() calls.
 *
 * Return the offset into the action table of the new transaction.
 */
int
acttab_insert(struct acttab *p) {
  int i, j, k, n;
  assert(p->nLookahead > 0);

  /* Make sure we have enough space to hold the expanded action table
   * in the worst case.  The worst case occurs if the transaction set
   * must be appended to the current action table
   */
  n = p->mxLookahead + 1;
  if (p->nAction + n >= p->nActionAlloc) {
    int oldAlloc = p->nActionAlloc;
    p->nActionAlloc = p->nAction + n + p->nActionAlloc + 20;
    p->aAction = (struct lookahead_action *)realloc(p->aAction, sizeof(p->aAction[0]) * p->nActionAlloc);
    if (p->aAction == 0) {
      fprintf(stderr, "malloc failed\n");
      exit(1);
    }
    for (i = oldAlloc; i < p->nActionAlloc; i++) {
      p->aAction[i].lookahead = -1;
      p->aAction[i].action = -1;
    }
  }

  /* Scan the existing action table looking for an offset that is a
   * duplicate of the current transaction set.  Fall out of the loop
   * if and when the duplicate is found.
   *
   * i is the index in p->aAction[] where p->mnLookahead is inserted.
   */
  for (i = p->nAction - 1; i >= 0; i--) {
    if (p->aAction[i].lookahead == p->mnLookahead) {
      /* All lookaheads and actions in the aLookahead[] transaction
       * must match against the candidate aAction[i] entry.
       */
      if (p->aAction[i].action != p->mnAction)
        continue;
      for (j = 0; j < p->nLookahead; j++) {
        k = p->aLookahead[j].lookahead - p->mnLookahead + i;
        if (k < 0 || k >= p->nAction)
          break;
        if (p->aLookahead[j].lookahead != p->aAction[k].lookahead)
          break;
        if (p->aLookahead[j].action != p->aAction[k].action)
          break;
      }
      if (j < p->nLookahead)
        continue;

      /* No possible lookahead value that is not in the aLookahead[]
       * transaction is allowed to match aAction[i]
       */
      n = 0;
      for (j = 0; j < p->nAction; j++) {
        if (p->aAction[j].lookahead < 0)
          continue;
        if (p->aAction[j].lookahead == j + p->mnLookahead - i)
          n++;
      }
      if (n == p->nLookahead) {
        break; // An exact match is found at offset i
      }
    }
  }

  /* If no existing offsets exactly match the current transaction, find an
   * an empty offset in the aAction[] table in which we can add the
   * aLookahead[] transaction.
   */
  if (i < 0) {
    /* Look for holes in the aAction[] table that fit the current
     * aLookahead[] transaction.  Leave i set to the offset of the hole.
     * If no holes are found, i is left at p->nAction, which means the
     * transaction will be appended.
     */
    for (i = 0; i < p->nActionAlloc - p->mxLookahead; i++) {
      if (p->aAction[i].lookahead < 0) {
        for (j = 0; j < p->nLookahead; j++) {
          k = p->aLookahead[j].lookahead - p->mnLookahead + i;
          if (k < 0)
            break;
          if (p->aAction[k].lookahead >= 0)
            break;
        }
        if (j < p->nLookahead)
          continue;
        for (j = 0; j < p->nAction; j++) {
          if (p->aAction[j].lookahead == j + p->mnLookahead - i)
            break;
        }
        if (j == p->nAction) {
          break; // Fits in empty slots
        }
      }
    }
  }
  /* Insert transaction set at index i. */
  for (j = 0; j < p->nLookahead; j++) {
    k = p->aLookahead[j].lookahead - p->mnLookahead + i;
    p->aAction[k] = p->aLookahead[j];
    if (k >= p->nAction)
      p->nAction = k + 1;
  }
  p->nLookahead = 0;

  /* Return the offset that is added to the lookahead in order to get the
   * index into yy_action of the action
   */
  return i - p->mnLookahead;
}
