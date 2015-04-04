#ifndef _LEMON_TABLE_H_
#define _LEMON_TABLE_H_

/*
 * Code for processing tables in the LEMON parser generator.
 */

#include <stdbool.h>
#include "action.h"

struct config;

/* Symbols (terminals and nonterminals) of the grammar are stored
 * in the following:
 */
enum symbol_type { TERMINAL, NONTERMINAL, MULTITERMINAL };

enum e_assoc { LEFT, RIGHT, NONE, UNK };

struct symbol {
  const char *name;        // Name of the symbol
  int index;               // Index number for this symbol
  enum symbol_type type;   // Symbols are all either TERMINALS or NTs
  struct rule *rule;       // Linked list of rules of this (if an NT)
  struct symbol *fallback; // fallback token in case this token doesn't parse
  int prec;                // Precedence if defined (-1 otherwise)
  enum e_assoc assoc;      // Associativity if precedence is defined
  char *firstset;          // First-set for all rules of this symbol
  bool lambda;             // True if NT and can generate an empty string
  int useCnt;              // Number of times used
  char *destructor;        // Code which executes whenever this symbol popped from the stack during error processing
  int destLineno;          // Line number for start of destructor
  char *datatype;          // The data type of information held by this object. Only used if type==NONTERMINAL
  int dtnum; // The data type number.  In the parser, the value stack is a union.  The .yy%d element of this union is
             // the correct data type for this object
  /* The following fields are used by MULTITERMINALs only */
  int nsubsym;            // Number of constituent symbols in the MULTI
  struct symbol **subsym; // Array of constituent symbols
};

/* Each production rule in the grammar is stored in the following
 * structure.
 */
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
  struct rule *nextlhs;   // Next rule with the same LHS
  struct rule *next;      // Next rule in the global list
};

/* Each state of the generated parser's finite state machine
 * is encoded as an instance of the following structure.
 */
struct state {
  struct config *bp;     // The basis configurations for this state
  struct config *cfp;    // All configurations in this set
  int statenum;          // Sequential number for this state
  struct action_list *actions;     // Array of actions for this state
  int nTknAct, nNtAct;   // Number of actions on terminals and nonterminals
  int iTknOfst, iNtOfst; // yy_action[] offset for terminals and nonterms
  int iDflt;             // Default action
};

/* A configuration is a production rule of the grammar together with
 * a mark (dot) showing how much of that rule has been processed so far.
 * Configurations also contain a follow-set which is a list of terminal
 * symbols which are allowed to immediately follow the end of the rule.
 * Every configuration is recorded as an instance of the following:
 */
enum cfgstatus { COMPLETE, INCOMPLETE };
struct config {
  struct rule *rp;       // The rule upon which the configuration is based
  int dot;               // The parse point
  char *fws;             // Follow-set for this configuration only
  struct plink *fplp;    // Follow-set forward propagation links
  struct plink *bplp;    // Follow-set backwards propagation links
  struct state *stp;     // Pointer to state which contains this
  enum cfgstatus status; // used during followset and shift computations
  struct config *next;   // Next configuration in the state
  struct config *bp;     // The next basis configuration
};

/* Routines for handling a strings */
const char *Strsafe(const char *);

void Strsafe_init(void);
int Strsafe_insert(const char *);
const char *Strsafe_find(const char *);

/* Routines for handling symbols of the grammar */
struct symbol *Symbol_new(const char *);
int Symbolcmpp(const void *, const void *);
void Symbol_init(void);
int Symbol_insert(struct symbol *, const char *);
struct symbol *Symbol_find(const char *);
struct symbol *Symbol_Nth(int);
int Symbol_count(void);
struct symbol **Symbol_arrayof(void);

/* Routines to manage the state table */
int Configcmp(const char *, const char *);
struct state *State_new(void);
void State_init(void);
int State_insert(struct state *, struct config *);
struct state *State_find(struct config *);
struct state **State_arrayof(/*  */);

/* Routines used for efficiency in Configlist_add */
void Configtable_init(void);
int Configtable_insert(struct config *);
struct config *Configtable_find(struct config *);
void Configtable_clear(int (*)(struct config *));

#endif //_LEMON_TABLE_H_
