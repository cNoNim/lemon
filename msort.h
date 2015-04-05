#ifndef _LEMON_MSORT_H_
#define _LEMON_MSORT_H_

void *mergesort(void *list, void *(*get_next_fn)(void const *), void (*set_next_fn)(void *, void *),
                int (*compare_fn)(void const *, void const *));

#endif //_LEMON_MSORT_H_
