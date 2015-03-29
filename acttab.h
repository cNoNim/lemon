#ifndef _LEMON_ACTTAB_H_
#define _LEMON_ACTTAB_H_

/*
 * This module implements routines use to construct the yy_action[] table.
 */

/*
 * The state of the yy_action table under construction is an instance of
 * the following structure.
 *
 * The yy_action table maps the pair (state_number, lookahead) into an
 * action_number.  The table is an array of integers pairs.  The state_number
 * determines an initial offset into the yy_action array.  The lookahead
 * value is then added to this initial offset to get an index X into the
 * yy_action array. If the aAction[X].lookahead equals the value of the
 * of the lookahead input, then the value of the action_number output is
 * aAction[X].action.  If the lookaheads do not match then the
 * default action for the state_number is returned.
 *
 * All actions associated with a single state_number are first entered
 * into aLookahead[] using multiple calls to acttab_action().  Then the
 * actions for that single state_number are placed into the aAction[]
 * array with a single call to acttab_insert().  The acttab_insert() call
 * also resets the aLookahead[] array in preparation for the next
 * state number.
 */
struct lookahead_action {
  int lookahead; // Value of the lookahead token
  int action;    // Action to take on the given lookahead
};

struct acttab {
  int nAction;                      // Number of used slots in aAction[]
  int nActionAlloc;                 // Slots allocated for aAction[]
  struct lookahead_action *aAction, // The yy_action[] table under construction
      *aLookahead;                  // A single new transaction set
  int mnLookahead;                  // Minimum aLookahead[].lookahead
  int mnAction;                     // Action associated with mnLookahead
  int mxLookahead;                  // Maximum aLookahead[].lookahead
  int nLookahead;                   // Used slots in aLookahead[]
  int nLookaheadAlloc;              // Slots allocated in aLookahead[]
};

/* Return the number of entries in the yy_action table */
#define acttab_size(X) ((X)->nAction)

/* The value for the N-th entry in yy_action */
#define acttab_yyaction(X, N) ((X)->aAction[N].action)

/* The value for the N-th entry in yy_lookahead */
#define acttab_yylookahead(X, N) ((X)->aAction[N].lookahead)

struct acttab *acttab_alloc(void);
void acttab_free(struct acttab *);
void acttab_action(struct acttab *, int, int);
int acttab_insert(struct acttab *);

#endif //_LEMON_ACTTAB_H_
