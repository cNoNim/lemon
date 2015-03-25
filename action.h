#ifndef _LEMON_ACTION_H_
#define _LEMON_ACTION_H_

struct rule;
struct state;
struct symbol;

enum e_action {
    SHIFT,
    ACCEPT,
    REDUCE,
    ERROR,
    SSCONFLICT,              /* A shift/shift conflict */
    SRCONFLICT,              /* Was a reduce, but part of a conflict */
    RRCONFLICT,              /* Was a reduce, but part of a conflict */
    SH_RESOLVED,             /* Was a shift.  Precedence resolved conflict */
    RD_RESOLVED,             /* Was reduce.  Precedence resolved conflict */
    NOT_USED                 /* Deleted by compression */
};

/* Every shift or reduce operation is stored as one of the following */
struct action {
    struct symbol *sp;       /* The look-ahead symbol */
    enum e_action type;
    union {
        struct state *stp;     /* The new state, if a shift */
        struct rule *rp;       /* The rule, if a reduce */
    } x;
    struct action *next;     /* Next action for this state */
    struct action *collide;  /* Next action with the same hash */
};

void Action_add(struct action **, enum e_action, struct symbol *, char *);
struct action *Action_new(void);
struct action *Action_sort(struct action *);

#endif //_LEMON_ACTION_H_
