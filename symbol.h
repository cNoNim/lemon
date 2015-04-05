#ifndef _LEMON_SYMBOL_H_
#define _LEMON_SYMBOL_H_

#include <stdbool.h>

/* Symbols (terminals and nonterminals) of the grammar are stored
 * in the following:
 */
enum symbol_type { TERMINAL, NONTERMINAL, MULTITERMINAL };

enum e_assoc { UNKNOWN = -1, NONE, LEFT, RIGHT };

struct symbol {
    const char *name;        // Name of the symbol
    int index;               // Index number for this symbol
    enum symbol_type type;   // Symbols are all either TERMINALS or NTs
    struct rule_list *rules; // Linked list of rules of this (if an NT)
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

struct symbol **array_of_symbol(unsigned int *size);
struct symbol *lookup_symbol(char const *key);
struct symbol *make_symbol(char const *key);

#endif //_LEMON_SYMBOL_H_
