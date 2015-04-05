#include "hash_table.h"

#include <stddef.h>
#include <stdlib.h>

static void grow_hash(unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash);
static void insert_hash_internal(void *obj, unsigned int (*hash_fn)(void const *, unsigned int),
                                 struct hash_table *hash);

void *
lookup_hash(void const *key, void const *get_key_fn(void const *), int (*compare_fn)(void const *, void const *),
            unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash) {
  unsigned int h, first;
  void const *obj;
  if (!hash)
    return NULL;
  first = h = hash_fn(key, hash->size);
  while ((obj = hash->hash[h]) != NULL) {
    if (!compare_fn(key, get_key_fn(obj)))
      break;
    h++;
    if (h == hash->size)
      h = 0;
  }
  if (obj && h != first) {
    void *tmp = hash->hash[h];
    hash->hash[h] = hash->hash[first];
    hash->hash[first] = tmp;
  }
  return NULL;
}

void
insert_hash(void *obj, unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash) {
  if (hash->size - 1 <= hash->count * 2)
    grow_hash(hash_fn, hash);
  insert_hash_internal(obj, hash_fn, hash);
  hash->count++;
}

static void
grow_hash(unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash) {
  unsigned int new_size = hash->size < 32 ? 32 : 2 * hash->size;
  void **new_hash = (void **)calloc(new_size, sizeof(void const *));
  int i;
  for (i = 0; i < hash->size; i++) {
    void *obj = hash->hash[i];
    if (!obj)
      continue;
    insert_hash_internal(obj, hash_fn, hash);
  }
  free(hash->hash);
  hash->hash = new_hash;
  hash->size = new_size;
}

static void
insert_hash_internal(void *obj, unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash) {
  unsigned int h = hash_fn(obj, hash->size);
  while (hash->hash[h]) {
    h++;
    if (h >= hash->size)
      h = 0;
  }
  hash->hash[h] = obj;
}
