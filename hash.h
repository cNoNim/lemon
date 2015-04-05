#ifndef _LEMON_HASH_H_
#define _LEMON_HASH_H_

struct rule;
struct config_list;

unsigned int ruleposhash(struct rule const *rule, unsigned int pos);
unsigned int configlisthash(struct config_list const *key);
unsigned int strhash(char const *key);

#endif //_LEMON_HASH_H_
