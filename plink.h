#ifndef _LEMON_PLINK_H_
#define _LEMON_PLINK_H_

struct config;

/* A followset propagation link indicates that the contents of one
** configuration followset should be propagated to another whenever
** the first changes. */
struct plink {
  struct config *cfp; /* The configuration to which linked */
  struct plink *next; /* The next propagate link */
};

struct plink *Plink_new(void);
void Plink_add(struct plink **, struct config *);
void Plink_copy(struct plink **, struct plink *);
void Plink_delete(struct plink *);

#endif //_LEMON_PLINK_H_
