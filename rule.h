#ifndef _LEMON_RULE_H_
#define _LEMON_RULE_H_

#include <stdbool.h>

/* Each production rule in the grammar is stored in the following
 * structure.
 */
struct rule_list {
    struct rule * item;
    struct rule_list * next;
};

struct rule {
    struct symbol *lhs;     // Left-hand side of the rule
    const char *lhsalias;   // Alias for the LHS (NULL if none)
    int lhsStart;           // True if left-hand side is the start symbol
    int ruleline;           // Line number for the rule
    int nrhs;               // Number of RHS symbols
    struct symbol **rhs;    // The RHS symbols
    const char **rhsalias;  // An alias for each RHS symbol (NULL if none)
    int line;               // Line number at which code begins
    const char *code;       // The code executed when this rule is reduced
    struct symbol *precsym; // Precedence symbol for this rule
    int index;              // An index number for this rule
    bool canReduce;         // True if this rule is ever reduced
};

struct rule_list *rule_list_insert(struct rule *rule, struct rule_list **list);

#endif //_LEMON_RULE_H_
