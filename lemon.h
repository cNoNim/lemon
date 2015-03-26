#ifndef _LEMON_STRUCT_H_
#define _LEMON_STRUCT_H_

/*
 * Principal data structures for the LEMON parser generator.
 */

struct state;
struct rule;
struct symbol;

/* The state vector for the entire parser generator is recorded as
 * follows.  (LEMON uses no global variables and makes little use of
 * static variables.  Fields in the following structure can be thought
 * of as begin global variables in the program.)
 */
struct lemon {
  struct state **sorted;   // Table of states sorted by state number
  struct rule *rule;       // List of all rules
  int nstate;              // Number of states
  int nrule;               // Number of rules
  int nsymbol;             // Number of terminal and nonterminal symbols
  int nterminal;           // Number of terminal symbols
  struct symbol **symbols; // Sorted array of pointers to symbols
  int errorcnt;            // Number of errors
  struct symbol *errsym;   // The error symbol
  struct symbol *wildcard; // Token that matches anything
  char *name;              // Name of the generated parser
  char *arg;               // Declaration of the 3th argument to parser
  char *tokentype;         // Type of terminal symbols in the parser stack
  char *vartype;           // The default type of non-terminal symbols
  char *start;             // Name of the start symbol for the grammar
  char *stacksize;         // Size of the parser stack
  char *include;           // Code to put at the start of the C file
  char *error;             // Code to execute when an error is seen
  char *overflow;          // Code to execute on a stack overflow
  char *failure;           // Code to execute on parser failure
  char *accept;            // Code to execute when the parser excepts
  char *extracode;         // Code appended to the generated file
  char *tokendest;         // Code to execute to destroy token data
  char *vardest;           // Code for the default non-terminal destructor
  char *filename;          // Name of the input file
  char *outname;           // Name of the current output file
  char *tokenprefix;       // A prefix added to token names in the .h file
  int nconflict;           // Number of parsing conflicts
  int tablesize;           // Size of the parse tables
  int basisflag;           // Print only basis configurations
  int has_fallback;        // True if any %fallback is seen in the grammar
  int nolinenosflag;       // True if #line statements should not be printed
  char *argv0;             // Name of the program
};

#endif //_LEMON_STRUCT_H_
