#include "symbol.h"
#include "hash_table.h"
#include "error.h"
#include "string.h"
#include "hash.h"

#include <ctype.h>
#include <stdlib.h>

struct hash_table symbol_hash;

static void const *get_key_symbol(void const *obj);
static unsigned int hash_symbol(void const *obj, unsigned int size);

struct symbol **
array_of_symbol(unsigned int *size) {
    struct symbol **array;
    unsigned int i = 0;
    if (!symbol_hash.hash)
        return NULL;
    array = (struct symbol **)calloc(symbol_hash.count, sizeof(struct symbol *));
    MemoryCheck(array);
    unsigned int h;
    for (h = 0, i = 0; h < symbol_hash.size; h++) {
        if (!symbol_hash.hash[h])
            continue;
        array[i] = symbol_hash.hash[h];
        i++;
    }
    if (size)
        *size = i;
    return array;
}

struct symbol *
lookup_symbol(char const *key) {
    return (struct symbol *)lookup_hash(key, get_key_symbol, compare_string, hash_symbol, &symbol_hash);
}

struct symbol *
make_symbol(char const *key) {
    struct symbol *obj, *new_obj;
    if (!key)
        return NULL;
    obj = lookup_symbol(key);
    if (!obj && (new_obj = (struct symbol *)calloc(1, sizeof(struct symbol))) != NULL) {
        new_obj->name = make_string(key);
        new_obj->type = isupper(*key) ? TERMINAL : NONTERMINAL;
        new_obj->prec = -1;
        new_obj->assoc = UNKNOWN;
        insert_hash(new_obj, hash_symbol, &symbol_hash);
        obj = new_obj;
    }
    MemoryCheck(obj);
    obj->useCnt++;
    return obj;
}

static void const *
get_key_symbol(void const *obj) {
    return ((struct symbol *)obj)->name;
}

static unsigned int
hash_symbol(void const *obj, unsigned int size) {
    return strhash(((struct symbol const *)obj)->name) & (size - 1);
}
