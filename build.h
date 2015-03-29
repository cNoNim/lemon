#ifndef _LEMON_BUILD_H_
#define _LEMON_BUILD_H_

/*
 * Routines to construction the finite state machine for the LEMON
 * parser generator.
 */

struct lemon;

void FindRulePrecedences(struct lemon *);
void FindFirstSets(struct lemon *);
void FindStates(struct lemon *);
void FindLinks(struct lemon *);
void FindFollowSets(struct lemon *);
void FindActions(struct lemon *);

#endif //_LEMON_BUILD_H_
