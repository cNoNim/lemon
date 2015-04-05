#include "config.h"
#include "error.h"
#include "hash_table.h"
#include "rule.h"
#include "state.h"
#include "hash.h"

#include <stdlib.h>

struct hash_table state_hash;

static int compare_config_list(void const *left, void const *right);
static void const *get_key_state(void const *obj);
static unsigned int hash_state(void const *obj, unsigned int size);

struct state **
array_of_state(unsigned int *size) {
    struct state **array;
    unsigned int i = 0;
    if (!state_hash.hash)
        return NULL;
    array = (struct state **)calloc(state_hash.count, sizeof(struct state *));
    MemoryCheck(array);
    unsigned int h;
    for (h = 0, i = 0; h < state_hash.size; h++) {
        if (!state_hash.hash[h])
            continue;
        array[i] = state_hash.hash[h];
        i++;
    }
    if (size)
        *size = i;
    return array;
}

struct state *
lookup_state(struct config_list *key) {
    return (struct state *)lookup_hash(key, get_key_state, compare_config_list, hash_state, &state_hash);
}

struct state *
make_state(struct config_list *key) {
    struct state *obj, *new_obj;
    if (!key)
        return NULL;
    obj = lookup_state(key);
    if (!obj && (new_obj = (struct state *)calloc(1, sizeof(struct state))) != NULL) {
        struct config_list *configs;
        new_obj->basis = key;
        configs = config_list_closure(key); // Compute the configuration closure
        config_list_sort(&configs);         // Sort the configuration closure
        new_obj->configs = configs;         // Remember the configuration closure
        insert_hash(new_obj, hash_state, &state_hash);
        obj = new_obj;
    }
    MemoryCheck(obj);
    return obj;
}

static int
compare_config_list(void const *left, void const *right) {
    struct config_list const *a = (struct config_list const *)left;
    struct config_list const *b = (struct config_list const *)right;
    int rc;
    for (rc = 0; rc == 0 && a && b; a = a->next, b = b->next) {
        rc = a->item->rule->index - b->item->rule->index;
        if (rc == 0)
            rc = a->item->position - b->item->position;
    }
    if (rc == 0) {
        if (a)
            rc = 1;
        if (b)
            rc = -1;
    }
    return rc;
}

static void const *
get_key_state(void const *obj) {
    return ((struct state *)obj)->basis;
}

static unsigned int
hash_state(void const *obj, unsigned int size) {
    return configlisthash(((struct state const *) obj)->basis) & (size - 1);
}
