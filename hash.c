#include "hash.h"
#include "rule.h"
#include "config.h"

#define FNV32_BASE ((unsigned int)0x811c9dc5)
#define FNV32_PRIME ((unsigned int)0x01000193)

unsigned int
ruleposhash(struct rule const *rule, unsigned int pos) {
    unsigned int hash = FNV32_BASE;
    hash = (hash * FNV32_PRIME) ^ rule->index;
    return (hash * FNV32_PRIME) ^ pos;
}

unsigned int
configlisthash(struct config_list const *key) {
    unsigned int hash = FNV32_BASE;
    while (key) {
        hash = (hash * FNV32_PRIME) ^ ruleposhash(key->item->rule, key->item->position);
        key = key->next;
    }
    return hash;
}

unsigned int
strhash(char const *key) {
    unsigned int c, hash = FNV32_BASE;
    while ((c = (unsigned char)*key++))
        hash = (hash * FNV32_PRIME) ^ c;
    return hash;
}
