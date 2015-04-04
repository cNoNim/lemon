#ifndef _LEMON_ACTION_H_
#define _LEMON_ACTION_H_

/*
 * Routines processing parser actions in the LEMON parser generator.
 */

enum action_type {
  SHIFT,
  ACCEPT,
  REDUCE,
  ERROR,
  SSCONFLICT,  // A shift/shift conflict
  SRCONFLICT,  // Was a reduce, but part of a conflict
  RRCONFLICT,  // Was a reduce, but part of a conflict
  SH_RESOLVED, // Was a shift.  Precedence resolved conflict
  RD_RESOLVED, // Was reduce.  Precedence resolved conflict
  NOT_USED     // Deleted by compression
};

/* Every shift or reduce operation is stored as one of the following */
struct action_list {
  struct action *item;
  struct action_list *next;
};

struct action {
  struct symbol *sp; // The look-ahead symbol
  enum action_type type;
  union {
    struct state *stp; // The new state, if a shift
    struct rule *rp;   // The rule, if a reduce
  } x;
};

struct action_list *action_list_insert(struct action *action, struct action_list **list);
void action_list_sort(struct action_list **list);

struct action *make_action(struct symbol *symbol, enum action_type type, void *arg);

#endif //_LEMON_ACTION_H_
