#ifndef _LEMON_CONFIG_H_
#define _LEMON_CONFIG_H_

/* A configuration is a production rule of the grammar together with
 * a mark (dot) showing how much of that rule has been processed so far.
 * Configurations also contain a follow-set which is a list of terminal
 * symbols which are allowed to immediately follow the end of the rule.
 * Every configuration is recorded as an instance of the following:
 */
struct config_list {
  struct config *item;
  struct config_list *next;
};

enum cfgstatus { COMPLETE, INCOMPLETE };
struct config {
  struct rule *rule;     // The rule upon which the configuration is based
  unsigned int position; // The parse point
  char *fws;             // Follow-set for this configuration only
  struct config_list *fplp;    // Follow-set forward propagation links
  struct config_list *bplp;    // Follow-set backwards propagation links
  struct state *stp;     // Pointer to state which contains this
  enum cfgstatus status; // used during followset and shift computations
};

struct config_list *config_list_insert(struct config *config, struct config_list **list);
struct config_list *config_list_copy(struct config_list **dest, struct config_list *src);
void clear_config_list(struct config_list *list);
void config_list_sort(struct config_list **list);
struct config *make_config(struct rule *rule, unsigned int position);

struct config_list *config_list_closure(struct config_list *list);

#endif //_LEMON_CONFIG_H_
