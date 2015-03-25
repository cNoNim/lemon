#ifndef _LEMON_CONFIGLIST_H_
#define _LEMON_CONFIGLIST_H_

struct lemon;
struct plink;
struct rule;
struct state;

/* A configuration is a production rule of the grammar together with
** a mark (dot) showing how much of that rule has been processed so far.
** Configurations also contain a follow-set which is a list of terminal
** symbols which are allowed to immediately follow the end of the rule.
** Every configuration is recorded as an instance of the following: */
enum cfgstatus {
    COMPLETE,
    INCOMPLETE
};
struct config {
    struct rule *rp;         /* The rule upon which the configuration is based */
    int dot;                 /* The parse point */
    char *fws;               /* Follow-set for this configuration only */
    struct plink *fplp;      /* Follow-set forward propagation links */
    struct plink *bplp;      /* Follow-set backwards propagation links */
    struct state *stp;       /* Pointer to state which contains this */
    enum cfgstatus status;   /* used during followset and shift computations */
    struct config *next;     /* Next configuration in the state */
    struct config *bp;       /* The next basis configuration */
};

void Configlist_init(void);
struct config *Configlist_add(struct rule *, int);
struct config *Configlist_addbasis(struct rule *, int);
void Configlist_closure(struct lemon *);
void Configlist_sort(void);
void Configlist_sortbasis(void);
struct config *Configlist_return(void);
struct config *Configlist_basis(void);
void Configlist_eat(struct config *);
void Configlist_reset(void);

#endif //_LEMON_CONFIGLIST_H_
