#ifndef _LEMON_HASH_TABLE_H_
#define _LEMON_HASH_TABLE_H_

struct hash_table {
  unsigned int size;
  unsigned int count;
  void **hash;
};

void *lookup_hash(void const *key, void const *get_key(void const *), int (*compare_fn)(void const *, void const *),
                  unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash);
void insert_hash(void *obj, unsigned int (*hash_fn)(void const *, unsigned int), struct hash_table *hash);

#endif //_LEMON_HASH_TABLE_H_
