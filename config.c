#include "error.h"
#include "hash_table.h"
#include "config.h"
#include "set.h"
#include "msort.h"
#include "rule.h"
#include "hash.h"
#include "symbol.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

struct config_key {
  struct rule *rule;
  unsigned int position;
};
struct hash_table config_hash;

static int compare_config_key(void const *left, void const *right);
static void const *get_key_config(void const *obj);
static unsigned int hash_config(void const *obj, unsigned int size);
static void *config_list_get_next(void const *list);
static void config_list_set_next(void *list, void *next);
static int config_list_compare(void const *left, void const *right);

struct config *
make_config(struct rule *rule, unsigned int position) {
  struct config *obj, *new_obj;
  struct config_key key;
  key.rule = rule;
  key.position = position;
  if (!rule)
    return NULL;
  obj = (struct config *) lookup_hash(&key, get_key_config, compare_config_key, hash_config, &config_hash);
  if (!obj && (new_obj = (struct config *)calloc(1, sizeof(struct config))) != NULL) {
    new_obj->rule = rule;
    new_obj->position = position;
    new_obj->fws = SetNew();
    insert_hash(new_obj, hash_config, &config_hash);
    obj = new_obj;
  }
  MemoryCheck(obj);
  return obj;
}

static int
compare_config_key(void const *left, void const *right) {
  struct config_key const *a = (struct config_key const *)left;
  struct config_key const *b = (struct config_key const *)right;
  int rc = a->rule->index - b->rule->index;
  if (rc == 0)
    rc = a->position - b->position;
  return rc;
}

static void const *
get_key_config(void const *obj) {
  return (struct config_key *)obj;
}

static unsigned int
hash_config(void const *obj, unsigned int size) {
  return ruleposhash(((struct config *)obj)->rule, ((struct config *)obj)->position) & (size - 1);
}



/* Compute the closure of the configuration list */
struct config_list *
config_list_closure(struct config_list *list) {
  struct config_list *new_cl = NULL;
  int i;

  for (; list; list = list->next) {
    struct rule *rule;
    struct symbol *symbol, *x_symbol;
    unsigned int dot;
    rule = list->item->rule;
    dot = list->item->position;
    if (dot >= rule->nrhs)
      continue;
    symbol = rule->rhs[dot];
    if (symbol->type == NONTERMINAL) {
      struct rule_list *rp;
      for (rp = symbol->rules; rp; rp = rp->next) {
        struct config *cfg = make_config(rp->item, 0);
        for (i = dot + 1; i < rule->nrhs; i++) {
          x_symbol = rule->rhs[i];
          if (x_symbol->type == TERMINAL) {
            SetAdd(cfg->fws, x_symbol->index);
            break;
          } else if (x_symbol->type == MULTITERMINAL) {
            int k;
            for (k = 0; k < x_symbol->nsubsym; k++) {
              SetAdd(cfg->fws, x_symbol->subsym[k]->index);
            }
            break;
          } else {
            SetUnion(cfg->fws, x_symbol->firstset);
            if (x_symbol->lambda == false)
              break;
          }
        }
        if (i == rule->nrhs)
          list->item->fplp = config_list_insert(cfg, &list->item->fplp);
        new_cl = config_list_insert(cfg, &new_cl);
      }
    }
  }
  return new_cl;
}

struct config_list *
config_list_insert(struct config *config, struct config_list **list) {
  struct config_list *new_list = (struct config_list *)malloc(sizeof(struct config_list));
  new_list->item = config;
  new_list->next = *list;
  *list = new_list;
  return new_list;
}

struct config_list *
config_list_copy(struct config_list **dest, struct config_list *src) {
  struct config_list *tmp;
  while(src) {
    tmp = src->next;
    src->next = *dest;
    *dest = src;
    src = tmp;
  }
  return *dest;
}

void
clear_config_list(struct config_list *list) {
  struct config_list *tmp;
  while(list) {
    tmp = list->next;
    list->next = NULL;
    free(list);
    list = tmp;
  }
}

void
config_list_sort(struct config_list **list) {
  *list = mergesort(*list, config_list_get_next, config_list_set_next, config_list_compare);
}

static void *
config_list_get_next(void const *list) {
  return ((struct config_list const *)list)->next;
}

static void
config_list_set_next(void *list, void *next) {
  ((struct config_list *)list)->next = next;
}

static int
config_list_compare(void const *left, void const *right) {
  const struct config *a = ((struct config_list *)left)->item;
  const struct config *b = ((struct config_list *)right)->item;
  int x;
  x = a->rule->index - b->rule->index;
  if (x == 0)
    x = a->position - b->position;
  return x;
}
