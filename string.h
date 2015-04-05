#ifndef _LEMON_STRING_H_
#define _LEMON_STRING_H_

char const *make_string(char const *key);

int compare_string(void const *left, void const *right);
unsigned int hash_string(void const *obj, unsigned int size);

#endif //_LEMON_STRING_H_
