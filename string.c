#include "string.h"
#include "hash_table.h"
#include "error.h"
#include "hash.h"

#include <string.h>
#include <stdlib.h>

struct hash_table string_hash;

static void const *get_key_string(void const *obj);
static unsigned int hash_string(void const *obj, unsigned int size);

int
compare_string(void const *left, void const *right) {
    return strcmp(left, right);
}

char const *
make_string(char const *key) {
    char *obj, *new_obj;
    if (!key)
        return NULL;
    obj = (char *)lookup_hash(key, get_key_string, compare_string, hash_string, &string_hash);
    if (!obj && (new_obj = (char *)malloc(strlen(key) + 1)) != NULL) {
        strcpy(new_obj, key);
        insert_hash(new_obj, hash_string, &string_hash);
        obj = new_obj;
    }
    MemoryCheck(obj);
    return obj;
}

static void const *
get_key_string(void const *obj) {
    return obj;
}

static unsigned int
hash_string(void const *obj, unsigned int size) {
    return strhash((char const *)obj) & (size - 1);
}
