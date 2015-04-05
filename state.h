#ifndef _LEMON_STATE_H_
#define _LEMON_STATE_H_

/* Each state of the generated parser's finite state machine
 * is encoded as an instance of the following structure.
 */
struct state {
    struct config_list *basis;   // The basis configurations for this state
    struct config_list *configs; // All configurations in this set
    int statenum;                // Sequential number for this state
    struct action_list *actions; // Array of actions for this state
    int nTknAct, nNtAct;         // Number of actions on terminals and nonterminals
    int iTknOfst, iNtOfst;       // yy_action[] offset for terminals and nonterms
    int iDflt;                   // Default action
};

struct state **array_of_state(unsigned int *size);
struct state *lookup_state(struct config_list *key);
struct state *make_state(struct config_list *key);

#endif //_LEMON_STATE_H_
